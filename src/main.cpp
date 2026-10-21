#include "audio.hpp"
#include "clicker.hpp"
#include "clicker_config.hpp"
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

static void input_thread(Config& config, Clicker& left_clicker, Clicker& right_clicker,
                          UInputDevice& device, GuiState& gui_state) {
    int kb_fd = find_device("Keyboard", false);
    if (kb_fd < 0) {
        fprintf(stderr, "  Error: No keyboard found. Need read access to /dev/input/*\n");
        return;
    }

    int mouse_fd = -1;
    bool need_mouse = (config.left_mode == ActivationMode::MOUSE ||
                       config.right_mode == ActivationMode::MOUSE);
    if (need_mouse) {
        mouse_fd = find_device("Mouse", true);
        if (mouse_fd >= 0) {
            ioctl(mouse_fd, EVIOCGRAB, 1);
            printf("  Mouse grabbed for click injection\n");
        }
    }

    bool mouse_left_held = false;
    bool mouse_right_held = false;

    struct pollfd fds[2];
    int nfds = 1;
    fds[0].fd = kb_fd;
    fds[0].events = POLLIN;
    if (mouse_fd >= 0) {
        fds[1].fd = mouse_fd;
        fds[1].events = POLLIN;
        nfds = 2;
    }

    struct input_event ev;

    while (g_running) {
        // Re-check if mouse mode was enabled at runtime
        bool now_need_mouse = (config.left_mode == ActivationMode::MOUSE ||
                               config.right_mode == ActivationMode::MOUSE);
        if (now_need_mouse && mouse_fd < 0) {
            mouse_fd = find_device("Mouse", true);
            if (mouse_fd >= 0) {
                ioctl(mouse_fd, EVIOCGRAB, 1);
                fds[1].fd = mouse_fd;
                fds[1].events = POLLIN;
                nfds = 2;
            }
        } else if (!now_need_mouse && mouse_fd >= 0) {
            ioctl(mouse_fd, EVIOCGRAB, 0);
            close(mouse_fd);
            mouse_fd = -1;
            nfds = 1;
        }

        int ret = poll(fds, nfds, 50);
        if (ret <= 0) continue;

        if (fds[0].revents & POLLIN) {
            while (read(kb_fd, &ev, sizeof(ev)) == sizeof(ev)) {
                if (ev.type != EV_KEY) continue;

                if (ev.value == 1 && ev.code >= KEY_1 && ev.code <= KEY_9) {
                    gui_state.active_slot = ev.code - KEY_1;
                    if (config.slot_filter_enabled && !config.allowed_slots[gui_state.active_slot]) {
                        left_clicker.set_enabled(false);
                        right_clicker.set_enabled(false);
                    }
                }

                if (config.inventory_key_enabled && ev.value == 1 && ev.code == static_cast<unsigned>(config.key_inventory)) {
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
                    if (config.audio_enabled && left_clicker.is_enabled() != was) {
                        if (left_clicker.is_enabled()) Audio::beep_on();
                        else Audio::beep_off();
                    }
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
                    if (config.audio_enabled && right_clicker.is_enabled() != was) {
                        if (right_clicker.is_enabled()) Audio::beep_on();
                        else Audio::beep_off();
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

        if (nfds > 1 && (fds[1].revents & POLLIN)) {
            while (read(mouse_fd, &ev, sizeof(ev)) == sizeof(ev)) {
                if (ev.type == EV_KEY) {
                    bool slot_blocked = config.slot_filter_enabled &&
                                        !config.allowed_slots[gui_state.active_slot];
                    bool is_bound_toggle = false;

                    if (!gui_state.in_menu) {
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
                            if (config.audio_enabled && left_clicker.is_enabled() != was) {
                                if (left_clicker.is_enabled()) Audio::beep_on();
                                else Audio::beep_off();
                            }
                            is_bound_toggle = true;
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
                            if (config.audio_enabled && right_clicker.is_enabled() != was) {
                                if (right_clicker.is_enabled()) Audio::beep_on();
                                else Audio::beep_off();
                            }
                            is_bound_toggle = true;
                        }
                    }

                    if (!is_bound_toggle) {
                        if (ev.code == BTN_LEFT) {
                            mouse_left_held = (ev.value != 0);
                            if (config.left_mode == ActivationMode::MOUSE && gui_state.left_armed && !gui_state.in_menu)
                                left_clicker.set_enabled(mouse_left_held);
                            else
                                device.click(BTN_LEFT, ev.value);
                        }
                        else if (ev.code == BTN_RIGHT) {
                            mouse_right_held = (ev.value != 0);
                            if (config.right_mode == ActivationMode::MOUSE && gui_state.right_armed && !gui_state.in_menu)
                                right_clicker.set_enabled(mouse_right_held);
                            else
                                device.click(BTN_RIGHT, ev.value);
                        }
                        else {
                            device.click(ev.code, ev.value);
                        }
                    } else {
                        // It is a bound toggle key. We must still inject it because the mouse is grabbed.
                        if (ev.code == BTN_LEFT) mouse_left_held = (ev.value != 0);
                        else if (ev.code == BTN_RIGHT) mouse_right_held = (ev.value != 0);
                        device.click(ev.code, ev.value);
                    }
                }
                else if (ev.type == EV_REL || ev.type == EV_SYN) {
                    device.emit_raw(ev.type, ev.code, ev.value);
                }
            }
        }
    }

    if (mouse_fd >= 0) {
        ioctl(mouse_fd, EVIOCGRAB, 0);
        close(mouse_fd);
    }
    close(kb_fd);
}

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
            printf("  --toggle       Toggle mode (default)\n");
            printf("  --hold         Hold mode\n");
            printf("  --mouse        Mouse mode (arm with key, hold mouse to click)\n");
            printf("  --headless     Run without GUI (CLI only)\n");
            printf("  --save         Save current config to ~/.config/ghast-clicker/config\n\n");
            printf("Requires /dev/uinput and /dev/input/* access.\n");
            printf("Run: sudo usermod -aG input $USER  (then re-login)\n");
            return 0;
        }
    }

    printf("\n  Ghast Clicker - Linux/Wayland Port\n");
    printf("  ─────────────────────────────\n");

    UInputDevice device;
    printf("  Virtual mouse device created\n");

    Clicker left_clicker(config.left, device);
    Clicker right_clicker(config.right, device);

    signal(SIGINT, on_signal);
    signal(SIGTERM, on_signal);

    GuiState gui_state(config, left_clicker, right_clicker, device);

    std::thread input_thr(input_thread, std::ref(config), std::ref(left_clicker),
                          std::ref(right_clicker), std::ref(device), std::ref(gui_state));

    if (headless) {
        printf("  Running headless (Ctrl+C to quit)\n\n");
        while (g_running) {
            printf("\r  L: %-3s %5.1f cps (%u)  |  R: %-3s %5.1f cps (%u)   ",
                   left_clicker.is_enabled() ? "ON" : "OFF",
                   left_clicker.tracker().avg_cps(), left_clicker.click_count(),
                   right_clicker.is_enabled() ? "ON" : "OFF",
                   right_clicker.tracker().avg_cps(), right_clicker.click_count());
            fflush(stdout);
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
        }
    } else {
        gui_run(gui_state);
        g_running = false;
    }

    left_clicker.set_enabled(false);
    right_clicker.set_enabled(false);

    if (input_thr.joinable()) input_thr.join();

    printf("\n  Stopped.\n");
    return 0;
}
