#include "audio.hpp"
#include "clicker.hpp"
#include "clicker_config.hpp"
#include "combat.hpp"
#include "config_file.hpp"
#include "gui.hpp"
#include "tracker.hpp"
#include "uinput_device.hpp"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <csignal>
#include <atomic>
#include <chrono>
#include <memory>
#include <thread>
#include <dirent.h>
#include <fcntl.h>
#include <poll.h>
#include <unistd.h>
#include <linux/input.h>

static std::atomic<bool> g_running{true};

static void on_signal(int) { g_running = false; }

static bool is_physical_device(int fd) {
    char phys[256] = "";
    if (ioctl(fd, EVIOCGPHYS(sizeof(phys)), phys) < 0 || phys[0] == '\0')
        return false;

    char name[256] = "";
    ioctl(fd, EVIOCGNAME(sizeof(name)), name);
    if (strcasestr(name, "Ghast Clicker")) return false;
    if (strcasestr(name, "Virtual")) return false;

    return true;
}

static int find_device(const char* label, bool want_mouse) {
    DIR* dir = opendir("/dev/input");
    if (!dir) return -1;

    int fallback_fd = -1;
    char fallback_name[256] = "";
    char fallback_path[128] = "";

    struct dirent* entry;
    while ((entry = readdir(dir))) {
        if (strncmp(entry->d_name, "event", 5) != 0) continue;

        char path[128];
        snprintf(path, sizeof(path), "/dev/input/%s", entry->d_name);

        int fd = open(path, O_RDONLY | O_NONBLOCK);
        if (fd < 0) continue;

        unsigned long evbits = 0;
        ioctl(fd, EVIOCGBIT(0, sizeof(evbits)), &evbits);

        if (evbits & (1 << EV_KEY)) {
            unsigned long keybits[KEY_MAX / (8 * sizeof(unsigned long)) + 1] = {};
            ioctl(fd, EVIOCGBIT(EV_KEY, sizeof(keybits)), keybits);

            auto has_key = [&](int code) {
                return keybits[code / (8 * sizeof(unsigned long))] &
                       (1UL << (code % (8 * sizeof(unsigned long))));
            };

            // A real keyboard has the full alphanumeric set plus modifiers.
            // Gaming mice (G305) register as kbd but lack the number row / F-row.
            bool is_keyboard = has_key(KEY_A) && has_key(KEY_Z) &&
                               has_key(KEY_1) && has_key(KEY_0) &&
                               has_key(KEY_F1) && has_key(KEY_F12) &&
                               has_key(KEY_SPACE) && has_key(KEY_ENTER) &&
                               has_key(KEY_LEFTSHIFT) && has_key(KEY_BACKSPACE) &&
                               !has_key(BTN_LEFT);
            bool is_mouse = has_key(BTN_LEFT) && has_key(BTN_RIGHT);

            if ((want_mouse && is_mouse) || (!want_mouse && is_keyboard)) {
                char name[256] = "Unknown";
                ioctl(fd, EVIOCGNAME(sizeof(name)), name);

                // Never read our own virtual devices, even as a fallback:
                // forwarding would loop back into itself.
                if (strcasestr(name, "Ghast Clicker")) {
                    close(fd);
                    continue;
                }

                if (is_physical_device(fd)) {
                    printf("  %s: %s (%s)\n", label, name, path);
                    if (fallback_fd >= 0) close(fallback_fd);
                    closedir(dir);
                    return fd;
                }

                // Keep as fallback if no physical device found
                if (fallback_fd < 0) {
                    fallback_fd = fd;
                    strncpy(fallback_name, name, sizeof(fallback_name) - 1);
                    strncpy(fallback_path, path, sizeof(fallback_path) - 1);
                    continue;
                }
            }
        }
        close(fd);
    }
    closedir(dir);

    if (fallback_fd >= 0) {
        printf("  %s (virtual): %s (%s)\n", label, fallback_name, fallback_path);
        return fallback_fd;
    }
    return -1;
}

static bool any_key_down(int fd) {
    unsigned char keys[KEY_MAX / 8 + 1] = {};
    if (ioctl(fd, EVIOCGKEY(sizeof(keys)), keys) < 0) return false;
    for (unsigned char b : keys)
        if (b) return true;
    return false;
}

static void sync_key_state(int fd, VirtualKeyboard& kb) {
    unsigned char keys[KEY_MAX / 8 + 1] = {};
    ioctl(fd, EVIOCGKEY(sizeof(keys)), keys);
    bool down[UInputKeyboard::MAX_CODE + 1] = {};
    for (int i = 0; i <= UInputKeyboard::MAX_CODE; i++)
        down[i] = keys[i / 8] & (1 << (i % 8));
    kb.sync_physical(down);
}

static void notify_attack(Config& config, GuiState& gui_state, Combat& combat) {
    if (gui_state.gui_focused) return;
    bool slot_blocked = config.slot_filter_enabled && !config.allowed_slots[gui_state.active_slot];
    combat.on_attack({gui_state.in_menu, slot_blocked});
}

static void beep(bool on) {
    if (on) Audio::beep_on();
    else Audio::beep_off();
}

static void input_thread(Config& config, Clicker& left_clicker, Clicker& right_clicker,
                          UInputDevice& device, Combat& combat, GuiState& gui_state) {
    int kb_fd = find_device("Keyboard", false);
    if (kb_fd < 0) {
        fprintf(stderr, "  Error: No keyboard found. Need read access to /dev/input/*\n");
        gui_state.set_input_error("No keyboard found. Ghast Clicker needs read access to /dev/input/* "
                                  "(add yourself to the 'input' group, then log out and back in).");
        return;
    }
    sync_key_state(kb_fd, combat.keyboard);

    int mouse_fd = -1;
    bool mouse_grabbed = false;
    bool kb_grabbed = false;
    auto last_mouse_search = std::chrono::steady_clock::time_point{};

    bool mouse_left_held = false;
    bool mouse_right_held = false;

    struct pollfd fds[2];
    fds[0].fd = kb_fd;
    fds[0].events = POLLIN;

    struct input_event ev;

    while (g_running) {
        // The mouse is read whenever mouse mode or a combat module needs it,
        // but only grabbed (and re-emitted) for mouse mode.
        bool want_mouse_grab = (config.left_mode == ActivationMode::MOUSE ||
                                config.right_mode == ActivationMode::MOUSE);
        bool want_mouse = want_mouse_grab || combat.any_enabled();
        auto now = std::chrono::steady_clock::now();

        if (want_mouse && mouse_fd < 0 && now - last_mouse_search > std::chrono::seconds(2)) {
            last_mouse_search = now;
            mouse_fd = find_device("Mouse", true);
        } else if (!want_mouse && mouse_fd >= 0) {
            if (mouse_grabbed) ioctl(mouse_fd, EVIOCGRAB, 0);
            close(mouse_fd);
            mouse_fd = -1;
            mouse_grabbed = false;
        }
        if (mouse_fd >= 0 && want_mouse_grab != mouse_grabbed) {
            if (ioctl(mouse_fd, EVIOCGRAB, want_mouse_grab ? 1 : 0) == 0) {
                mouse_grabbed = want_mouse_grab;
                if (mouse_grabbed) printf("  Mouse grabbed for click injection\n");
            }
        }

        // W-Tap needs the keyboard grabbed. Only switch while nothing is held:
        // a key pressed on one side of the grab and released on the other
        // would stay stuck down in the compositor.
        bool want_kb_grab = combat.sprint.needs_keyboard_grab();
        if (want_kb_grab != kb_grabbed && !any_key_down(kb_fd)) {
            if (want_kb_grab) {
                if (ioctl(kb_fd, EVIOCGRAB, 1) == 0) {
                    if (any_key_down(kb_fd)) {
                        ioctl(kb_fd, EVIOCGRAB, 0);
                    } else {
                        kb_grabbed = true;
                        combat.keyboard.set_forwarding(true);
                        printf("  Keyboard grabbed for W-Tap\n");
                    }
                }
            } else {
                combat.keyboard.set_forwarding(false);
                ioctl(kb_fd, EVIOCGRAB, 0);
                kb_grabbed = false;
                printf("  Keyboard released\n");
            }
        }

        int nfds = 1;
        if (mouse_fd >= 0) {
            fds[1].fd = mouse_fd;
            fds[1].events = POLLIN;
            nfds = 2;
        }

        int ret = poll(fds, nfds, 50);
        if (ret <= 0) continue;

        if (fds[0].revents & POLLIN) {
            while (read(kb_fd, &ev, sizeof(ev)) == sizeof(ev)) {
                if (ev.type != EV_KEY) continue;

                combat.keyboard.on_physical(ev.code, ev.value);

                if (ev.value == 1 && ev.code >= KEY_1 && ev.code <= KEY_9) {
                    gui_state.active_slot = ev.code - KEY_1;
                    if (config.slot_filter_enabled && !config.allowed_slots[gui_state.active_slot]) {
                        left_clicker.set_enabled(false);
                        right_clicker.set_enabled(false);
                    }
                }

                if (ev.value == 1 && ev.code == static_cast<unsigned>(config.key_inventory)) {
                    if (!gui_state.in_menu) {
                        gui_state.in_menu = true;
                        left_clicker.set_enabled(false);
                        right_clicker.set_enabled(false);
                    } else {
                        gui_state.in_menu = false;
                    }
                    continue;
                }
                if (ev.value == 1 && ev.code == static_cast<unsigned>(config.key_escape) && gui_state.in_menu) {
                    gui_state.in_menu = false;
                    continue;
                }

                if (gui_state.in_menu) continue;

                bool slot_blocked = config.slot_filter_enabled &&
                                    !config.allowed_slots[gui_state.active_slot];

                if (ev.code == static_cast<unsigned>(config.key_left)) {
                    bool was = left_clicker.is_enabled();
                    if (config.left_mode == ActivationMode::TOGGLE) {
                        if (ev.value == 1) {
                            if (slot_blocked) left_clicker.set_enabled(false);
                            else left_clicker.toggle();
                        }
                    } else if (config.left_mode == ActivationMode::HOLD) {
                        left_clicker.set_enabled(ev.value != 0 && !slot_blocked);
                    } else {
                        if (ev.value == 1) {
                            gui_state.left_armed = !gui_state.left_armed;
                            if (!gui_state.left_armed) left_clicker.set_enabled(false);
                            else if (mouse_left_held) left_clicker.set_enabled(true);
                        }
                    }
                    if (config.audio_enabled && left_clicker.is_enabled() != was)
                        beep(left_clicker.is_enabled());
                }
                else if (ev.code == static_cast<unsigned>(config.key_right)) {
                    bool was = right_clicker.is_enabled();
                    if (config.right_mode == ActivationMode::TOGGLE) {
                        if (ev.value == 1) {
                            if (slot_blocked) right_clicker.set_enabled(false);
                            else right_clicker.toggle();
                        }
                    } else if (config.right_mode == ActivationMode::HOLD) {
                        right_clicker.set_enabled(ev.value != 0 && !slot_blocked);
                    } else {
                        if (ev.value == 1) {
                            gui_state.right_armed = !gui_state.right_armed;
                            if (!gui_state.right_armed) right_clicker.set_enabled(false);
                            else if (mouse_right_held) right_clicker.set_enabled(true);
                        }
                    }
                    if (config.audio_enabled && right_clicker.is_enabled() != was)
                        beep(right_clicker.is_enabled());
                }
                else if (ev.code == static_cast<unsigned>(config.key_jump_reset)) {
                    if (ev.value == 1) {
                        combat.jump.toggle();
                        if (config.audio_enabled) beep(combat.jump.is_enabled());
                    }
                }
                else if (ev.code == static_cast<unsigned>(config.key_sprint_reset)) {
                    if (ev.value == 1) {
                        combat.sprint.toggle();
                        if (config.audio_enabled) beep(combat.sprint.is_enabled());
                    }
                }
                else if (ev.value == 1) {
                    for (int m = 0; m < MAX_MACROS; m++) {
                        if (config.macros[m].key_bind > 0 &&
                            ev.code == static_cast<unsigned>(config.macros[m].key_bind) &&
                            config.macros[m].step_count > 0) {
                            gui_state.macro_runner.toggle(config.macros[m], &gui_state.active_slot);
                            break;
                        }
                    }
                }
            }
        }

        if (nfds > 1 && (fds[1].revents & (POLLERR | POLLHUP | POLLNVAL))) {
            // Mouse unplugged; search again on the next pass.
            close(mouse_fd);
            mouse_fd = -1;
            mouse_grabbed = false;
            continue;
        }

        if (nfds > 1 && (fds[1].revents & POLLIN)) {
            while (read(mouse_fd, &ev, sizeof(ev)) == sizeof(ev)) {
                if (ev.type == EV_KEY) {
                    if (ev.code == BTN_LEFT) {
                        mouse_left_held = (ev.value != 0);
                        if (mouse_grabbed && config.left_mode == ActivationMode::MOUSE &&
                            gui_state.left_armed && !gui_state.in_menu) {
                            // The clicker's own presses report the attacks.
                            left_clicker.set_enabled(mouse_left_held);
                        } else {
                            if (mouse_grabbed) device.click(BTN_LEFT, ev.value);
                            if (ev.value == 1) notify_attack(config, gui_state, combat);
                        }
                    }
                    else if (ev.code == BTN_RIGHT) {
                        mouse_right_held = (ev.value != 0);
                        if (mouse_grabbed && config.right_mode == ActivationMode::MOUSE &&
                            gui_state.right_armed && !gui_state.in_menu)
                            right_clicker.set_enabled(mouse_right_held);
                        else if (mouse_grabbed)
                            device.click(BTN_RIGHT, ev.value);
                    }
                    else if (mouse_grabbed) {
                        device.click(ev.code, ev.value);
                    }
                }
                else if (mouse_grabbed && (ev.type == EV_REL || ev.type == EV_SYN)) {
                    device.emit_raw(ev.type, ev.code, ev.value);
                }
            }
        }
    }

    if (mouse_fd >= 0) {
        if (mouse_grabbed) ioctl(mouse_fd, EVIOCGRAB, 0);
        close(mouse_fd);
    }
    if (kb_grabbed) {
        combat.keyboard.set_forwarding(false);
        ioctl(kb_fd, EVIOCGRAB, 0);
    }
    close(kb_fd);
}

static const char* SETUP_HELP =
    "Ghast Clicker needs access to /dev/uinput and /dev/input/*.\n\n"
    "Easiest fix, from the project folder:\n"
    "  ./install.sh\n\n"
    "Or by hand:\n"
    "  sudo usermod -aG input $USER\n"
    "  sudo cp 99-ghast-clicker.rules /etc/udev/rules.d/\n"
    "  sudo udevadm control --reload-rules && sudo udevadm trigger\n"
    "  sudo modprobe uinput\n"
    "Then log out and back in.\n";

int main(int argc, char* argv[]) {
    // Under sudo -E, GTK finds the user's broken icon theme via HOME and crashes.
    // Redirect HOME so GTK can't find ~/.local/share/icons/kora.
    if (getuid() == 0) {
        const char* real_home = getenv("HOME");
        char saved_home[256] = "";
        if (real_home) strncpy(saved_home, real_home, sizeof(saved_home) - 1);

        setenv("HOME", "/tmp", 1);
        setenv("XDG_DATA_HOME", "/tmp", 1);
        setenv("XDG_CACHE_HOME", "/tmp", 1);
    }

    Config config;
    ConfigFile::load(config);

    bool headless = false;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--lcps") == 0 && i + 1 < argc)
            config.left.cps_mean = atof(argv[++i]);
        else if (strcmp(argv[i], "--rcps") == 0 && i + 1 < argc)
            config.right.cps_mean = atof(argv[++i]);
        else if (strcmp(argv[i], "--lkey") == 0 && i + 1 < argc)
            config.key_left = atoi(argv[++i]);
        else if (strcmp(argv[i], "--rkey") == 0 && i + 1 < argc)
            config.key_right = atoi(argv[++i]);
        else if (strcmp(argv[i], "--invkey") == 0 && i + 1 < argc)
            config.key_inventory = atoi(argv[++i]);
        else if (strcmp(argv[i], "--jkey") == 0 && i + 1 < argc)
            config.key_jump_reset = atoi(argv[++i]);
        else if (strcmp(argv[i], "--skey") == 0 && i + 1 < argc)
            config.key_sprint_reset = atoi(argv[++i]);
        else if (strcmp(argv[i], "--hold") == 0) {
            config.left_mode = ActivationMode::HOLD;
            config.right_mode = ActivationMode::HOLD;
        }
        else if (strcmp(argv[i], "--toggle") == 0) {
            config.left_mode = ActivationMode::TOGGLE;
            config.right_mode = ActivationMode::TOGGLE;
        }
        else if (strcmp(argv[i], "--mouse") == 0) {
            config.left_mode = ActivationMode::MOUSE;
            config.right_mode = ActivationMode::MOUSE;
        }
        else if (strcmp(argv[i], "--headless") == 0) {
            headless = true;
        }
        else if (strcmp(argv[i], "--save") == 0) {
            ConfigFile::save(config);
            printf("  Config saved to %s/config\n", ConfigFile::path().c_str());
            return 0;
        }
        else if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            printf("Ghast Clicker - Humanized auto-clicker for Linux/Wayland\n\n");
            printf("Usage: ghast-clicker [options]\n\n");
            printf("Options:\n");
            printf("  --lcps <n>     Left CPS mean (default: %.1f)\n", config.left.cps_mean);
            printf("  --rcps <n>     Right CPS mean (default: %.1f)\n", config.right.cps_mean);
            printf("  --lkey <code>  Left toggle key (default: %d)\n", config.key_left);
            printf("  --rkey <code>  Right toggle key (default: %d)\n", config.key_right);
            printf("  --invkey <code> Inventory key (default: %d)\n", config.key_inventory);
            printf("  --jkey <code>  Jump Reset toggle key (default: %d)\n", config.key_jump_reset);
            printf("  --skey <code>  Sprint Reset toggle key (default: %d)\n", config.key_sprint_reset);
            printf("  --toggle       Toggle mode (default)\n");
            printf("  --hold         Hold mode\n");
            printf("  --mouse        Mouse mode (arm with key, hold mouse to click)\n");
            printf("  --headless     Run without GUI (CLI only)\n");
            printf("  --save         Save current config to ~/.config/ghast-clicker/config\n\n");
            printf("Requires /dev/uinput and /dev/input/* access.\n");
            printf("Run ./install.sh once, or: sudo usermod -aG input $USER  (then re-login)\n");
            return 0;
        }
    }

    printf("\n  Ghast Clicker - Linux/Wayland Port\n");
    printf("  ─────────────────────────────\n");

    std::unique_ptr<UInputDevice> device_ptr;
    std::unique_ptr<Combat> combat_ptr;
    try {
        device_ptr = std::make_unique<UInputDevice>();
        combat_ptr = std::make_unique<Combat>(config);
    } catch (const std::exception& e) {
        fprintf(stderr, "  Error: %s\n\n%s", e.what(), SETUP_HELP);
        if (!headless) {
            char msg[1024];
            snprintf(msg, sizeof(msg), "%s\n\n%s", e.what(), SETUP_HELP);
            gui_show_setup_error(msg);
        }
        return 1;
    }
    UInputDevice& device = *device_ptr;
    Combat& combat = *combat_ptr;
    printf("  Virtual mouse and keyboard created\n");

    Clicker left_clicker(config.left, device);
    Clicker right_clicker(config.right, device);

    signal(SIGINT, on_signal);
    signal(SIGTERM, on_signal);

    GuiState gui_state(config, left_clicker, right_clicker, combat, device);
    left_clicker.set_on_press([&] { notify_attack(config, gui_state, combat); });

    std::thread input_thr(input_thread, std::ref(config), std::ref(left_clicker),
                          std::ref(right_clicker), std::ref(device), std::ref(combat),
                          std::ref(gui_state));

    if (headless) {
        printf("  Running headless (Ctrl+C to quit)\n\n");
        while (g_running) {
            printf("\r  L: %-3s %5.1f cps (%u)  |  R: %-3s %5.1f cps (%u)  |  JR: %-3s  SR: %-3s   ",
                   left_clicker.is_enabled() ? "ON" : "OFF",
                   left_clicker.tracker().avg_cps(), left_clicker.click_count(),
                   right_clicker.is_enabled() ? "ON" : "OFF",
                   right_clicker.tracker().avg_cps(), right_clicker.click_count(),
                   combat.jump.is_enabled() ? "ON" : "OFF",
                   combat.sprint.is_enabled() ? "ON" : "OFF");
            fflush(stdout);
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
        }
    } else {
        gui_run(gui_state);
        g_running = false;
    }

    left_clicker.set_enabled(false);
    right_clicker.set_enabled(false);
    combat.jump.set_enabled(false);
    combat.sprint.set_enabled(false);

    if (input_thr.joinable()) input_thr.join();

    printf("\n  Stopped.\n");
    return 0;
}
