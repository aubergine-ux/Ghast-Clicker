#include "gui.hpp"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include <GLFW/glfw3.h>
#include <cstdio>
#include <ctime>
#include <linux/input-event-codes.h>

static const char* key_name(int code) {
    static char buf[32];
    switch (code) {
        case KEY_ESC: return "Escape";
        case KEY_1: return "1"; case KEY_2: return "2"; case KEY_3: return "3";
        case KEY_4: return "4"; case KEY_5: return "5"; case KEY_6: return "6";
        case KEY_7: return "7"; case KEY_8: return "8"; case KEY_9: return "9";
        case KEY_0: return "0";
        case KEY_Q: return "Q"; case KEY_W: return "W"; case KEY_E: return "E";
        case KEY_R: return "R"; case KEY_T: return "T"; case KEY_Y: return "Y";
        case KEY_U: return "U"; case KEY_I: return "I"; case KEY_O: return "O";
        case KEY_P: return "P"; case KEY_A: return "A"; case KEY_S: return "S";
        case KEY_D: return "D"; case KEY_F: return "F"; case KEY_G: return "G";
        case KEY_H: return "H"; case KEY_J: return "J"; case KEY_K: return "K";
        case KEY_L: return "L"; case KEY_Z: return "Z"; case KEY_X: return "X";
        case KEY_C: return "C"; case KEY_V: return "V"; case KEY_B: return "B";
        case KEY_N: return "N"; case KEY_M: return "M";
        case KEY_F1: return "F1"; case KEY_F2: return "F2"; case KEY_F3: return "F3";
        case KEY_F4: return "F4"; case KEY_F5: return "F5"; case KEY_F6: return "F6";
        case KEY_F7: return "F7"; case KEY_F8: return "F8"; case KEY_F9: return "F9";
        case KEY_F10: return "F10"; case KEY_F11: return "F11"; case KEY_F12: return "F12";
        case KEY_SPACE: return "Space";
        case KEY_LEFTSHIFT: return "LShift"; case KEY_RIGHTSHIFT: return "RShift";
        case KEY_LEFTCTRL: return "LCtrl"; case KEY_RIGHTCTRL: return "RCtrl";
        case KEY_LEFTALT: return "LAlt"; case KEY_RIGHTALT: return "RAlt";
        case KEY_TAB: return "Tab"; case KEY_CAPSLOCK: return "CapsLock";
        case KEY_ENTER: return "Enter"; case KEY_BACKSPACE: return "Backspace";
        case KEY_DELETE: return "Delete"; case KEY_INSERT: return "Insert";
        case KEY_HOME: return "Home"; case KEY_END: return "End";
        case KEY_PAGEUP: return "PageUp"; case KEY_PAGEDOWN: return "PageDown";
        default:
            snprintf(buf, sizeof(buf), "Key %d", code);
            return buf;
    }
}

// Map GLFW key codes to Linux evdev codes
static int glfw_to_evdev(int glfw_key) {
    if (glfw_key >= GLFW_KEY_A && glfw_key <= GLFW_KEY_Z)
        return KEY_A + (glfw_key - GLFW_KEY_A);
    if (glfw_key >= GLFW_KEY_0 && glfw_key <= GLFW_KEY_9)
        return (glfw_key == GLFW_KEY_0) ? KEY_0 : KEY_1 + (glfw_key - GLFW_KEY_1);
    if (glfw_key >= GLFW_KEY_F1 && glfw_key <= GLFW_KEY_F12)
        return KEY_F1 + (glfw_key - GLFW_KEY_F1);

    switch (glfw_key) {
        case GLFW_KEY_ESCAPE: return KEY_ESC;
        case GLFW_KEY_TAB: return KEY_TAB;
        case GLFW_KEY_CAPS_LOCK: return KEY_CAPSLOCK;
        case GLFW_KEY_LEFT_SHIFT: return KEY_LEFTSHIFT;
        case GLFW_KEY_RIGHT_SHIFT: return KEY_RIGHTSHIFT;
        case GLFW_KEY_LEFT_CONTROL: return KEY_LEFTCTRL;
        case GLFW_KEY_RIGHT_CONTROL: return KEY_RIGHTCTRL;
        case GLFW_KEY_LEFT_ALT: return KEY_LEFTALT;
        case GLFW_KEY_RIGHT_ALT: return KEY_RIGHTALT;
        case GLFW_KEY_SPACE: return KEY_SPACE;
        case GLFW_KEY_ENTER: return KEY_ENTER;
        case GLFW_KEY_BACKSPACE: return KEY_BACKSPACE;
        case GLFW_KEY_DELETE: return KEY_DELETE;
        case GLFW_KEY_INSERT: return KEY_INSERT;
        case GLFW_KEY_HOME: return KEY_HOME;
        case GLFW_KEY_END: return KEY_END;
        case GLFW_KEY_PAGE_UP: return KEY_PAGEUP;
        case GLFW_KEY_PAGE_DOWN: return KEY_PAGEDOWN;
        default: return -1;
    }
}

static int* g_binding_target = nullptr;
static GLFWwindow* g_window = nullptr;

static void key_callback(GLFWwindow*, int key, int, int action, int) {
    if (!g_binding_target || action != GLFW_PRESS) return;
    if (key == GLFW_KEY_ESCAPE) {
        *g_binding_target = 0;
    } else {
        int evdev = glfw_to_evdev(key);
        if (evdev >= 0)
            *g_binding_target = evdev;
    }
    g_binding_target = nullptr;
}

static void keybind_button(const char* label, int& key_code) {
    char buf[64];
    if (g_binding_target == &key_code) {
        snprintf(buf, sizeof(buf), "Press a key...##%s", label);
    } else if (key_code == 0) {
        snprintf(buf, sizeof(buf), "Click to bind##%s", label);
    } else {
        snprintf(buf, sizeof(buf), "%s##%s", key_name(key_code), label);
    }

    if (ImGui::Button(buf, ImVec2(150, 0))) {
        g_binding_target = &key_code;
    }
    ImGui::SameLine();
    ImGui::Text("%s", label);
}

static void mode_combo(const char* label, ActivationMode& mode) {
    int idx = mode == ActivationMode::TOGGLE ? 0 : (mode == ActivationMode::HOLD ? 1 : 2);
    if (ImGui::Combo(label, &idx, "Toggle\0Hold\0Mouse\0")) {
        mode = idx == 0 ? ActivationMode::TOGGLE : (idx == 1 ? ActivationMode::HOLD : ActivationMode::MOUSE);
    }
}

static void clicker_tab(const char* name, ClickerProfile& profile, ActivationMode& mode,
                         Clicker& clicker, int& key, bool& show_advanced) {
    if (!ImGui::BeginTabItem(name)) return;

    ImGui::Spacing();

    keybind_button("Toggle Key", key);

    ImGui::Separator();
    ImGui::Text("Activation");
    mode_combo("##mode", mode);

    ImGui::Separator();
    ImGui::Text("CPS");
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("Ctrl+Click a slider to type a value directly.\nValues between 9.5 - 12.5 recommended for server-side AC bypass.");

    ImGui::SliderScalar("Mean CPS", ImGuiDataType_Double, &profile.cps_mean, &(const double&){1.0}, &(const double&){25.0}, "%.1f");
    ImGui::SliderScalar("Std Dev", ImGuiDataType_Double, &profile.cps_stddev, &(const double&){0.0}, &(const double&){5.0}, "%.2f");
    ImGui::SliderScalar("Min CPS", ImGuiDataType_Double, &profile.cps_min, &(const double&){1.0}, &(const double&){20.0}, "%.1f");
    ImGui::SliderScalar("Max CPS", ImGuiDataType_Double, &profile.cps_max, &(const double&){5.0}, &(const double&){30.0}, "%.1f");

    ImGui::Separator();
    ImGui::Text("Humanization");

    ImGui::SliderScalar("Spike Chance", ImGuiDataType_Double, &profile.spike_chance, &(const double&){0.0}, &(const double&){0.2}, "%.3f");
    if (profile.spike_chance > 0) {
        ImGui::SliderScalar("Spike Min", ImGuiDataType_Double, &profile.spike_min, &(const double&){0.0}, &(const double&){5.0}, "%.1f");
        ImGui::SliderScalar("Spike Max", ImGuiDataType_Double, &profile.spike_max, &(const double&){0.0}, &(const double&){8.0}, "%.1f");
    }

    ImGui::SliderScalar("Stutter Chance", ImGuiDataType_Double, &profile.stutter_chance, &(const double&){0.0}, &(const double&){0.2}, "%.3f");
    if (profile.stutter_chance > 0) {
        ImGui::SliderScalar("Stutter Min", ImGuiDataType_Double, &profile.stutter_min, &(const double&){0.0}, &(const double&){8.0}, "%.1f");
        ImGui::SliderScalar("Stutter Max", ImGuiDataType_Double, &profile.stutter_max, &(const double&){0.0}, &(const double&){10.0}, "%.1f");
    }

    ImGui::Separator();
    ImGui::Text("Hold Timing");

    ImGui::SliderScalar("Hold Mean (ms)", ImGuiDataType_Double, &profile.hold_ms_mean, &(const double&){10.0}, &(const double&){100.0}, "%.1f");
    ImGui::SliderScalar("Hold StdDev (ms)", ImGuiDataType_Double, &profile.hold_ms_stddev, &(const double&){0.0}, &(const double&){20.0}, "%.1f");
    ImGui::SliderScalar("Hold Min (ms)", ImGuiDataType_Double, &profile.hold_ms_min, &(const double&){5.0}, &(const double&){50.0}, "%.1f");
    ImGui::SliderScalar("Hold Max (ms)", ImGuiDataType_Double, &profile.hold_ms_max, &(const double&){20.0}, &(const double&){150.0}, "%.1f");

    ImGui::Checkbox("Advanced", &show_advanced);
    if (show_advanced) {
        ImGui::SliderScalar("Heavy Hold Chance", ImGuiDataType_Double, &profile.hold_ms_heavy_chance, &(const double&){0.0}, &(const double&){0.1}, "%.3f");
        ImGui::SliderScalar("Heavy Hold Min", ImGuiDataType_Double, &profile.hold_ms_heavy_min, &(const double&){5.0}, &(const double&){50.0}, "%.1f");
        ImGui::SliderScalar("Heavy Hold Max", ImGuiDataType_Double, &profile.hold_ms_heavy_max, &(const double&){10.0}, &(const double&){80.0}, "%.1f");
        ImGui::SliderScalar("Rhythm Volatility", ImGuiDataType_Double, &profile.rhythm_volatility, &(const double&){0.0}, &(const double&){2.0}, "%.2f");
        ImGui::SliderScalar("Rhythm Tension", ImGuiDataType_Double, &profile.rhythm_tension, &(const double&){0.0}, &(const double&){0.2}, "%.3f");
        ImGui::SliderScalar("CPS Min/Max Fallout", ImGuiDataType_Double, &profile.cps_minmax_fallout, &(const double&){0.0}, &(const double&){3.0}, "%.2f");
        ImGui::SliderScalar("Start Delay (ms)", ImGuiDataType_Double, &profile.start_delay_ms, &(const double&){0.0}, &(const double&){500.0}, "%.0f");
    }

    ImGui::Separator();
    const auto& t = clicker.tracker();
    ImGui::Text("Status: %s", clicker.is_enabled() ? "ACTIVE" : "Idle");
    ImGui::Text("Avg CPS: %.1f  |  Avg Hold: %.1f ms  |  Clicks: %u",
                t.avg_cps(), t.avg_hold_ms(), clicker.click_count());

    ImGui::EndTabItem();
}

void gui_run(GuiState& state) {
    if (!glfwInit()) {
        fprintf(stderr, "Failed to initialize GLFW\n");
        return;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
    glfwWindowHint(GLFW_DECORATED, GLFW_FALSE);

    GLFWwindow* window = glfwCreateWindow(620, 520, "GhostTap", nullptr, nullptr);
    if (!window) {
        fprintf(stderr, "Failed to create GLFW window\n");
        glfwTerminate();
        return;
    }
    g_window = window;

    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);
    glfwSetKeyCallback(window, key_callback);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.IniFilename = nullptr;

    ImGui::StyleColorsDark();
    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding = 0.0f;
    style.FrameRounding = 3.0f;
    style.GrabRounding = 5.0f;
    style.GrabMinSize = 10.0f;
    style.ScrollbarSize = 10.0f;
    style.TabRounding = 3.0f;
    style.WindowBorderSize = 0.0f;

    auto* colors = style.Colors;
    ImVec4 accent{0.45f, 0.28f, 0.62f, 1.0f};
    ImVec4 accent_hover{0.55f, 0.38f, 0.72f, 1.0f};
    ImVec4 accent_active{0.65f, 0.48f, 0.82f, 1.0f};

    colors[ImGuiCol_WindowBg]       = ImVec4(0.08f, 0.08f, 0.08f, 0.96f);
    colors[ImGuiCol_PopupBg]        = ImVec4(0.08f, 0.08f, 0.08f, 0.96f);
    colors[ImGuiCol_Border]         = accent;
    colors[ImGuiCol_FrameBg]        = ImVec4(0.14f, 0.14f, 0.14f, 0.54f);
    colors[ImGuiCol_FrameBgHovered] = ImVec4(0.19f, 0.19f, 0.19f, 0.54f);
    colors[ImGuiCol_FrameBgActive]  = ImVec4(0.26f, 0.26f, 0.26f, 0.54f);
    colors[ImGuiCol_TitleBg]        = ImVec4(0.06f, 0.06f, 0.06f, 1.0f);
    colors[ImGuiCol_TitleBgActive]  = accent;
    colors[ImGuiCol_ScrollbarBg]    = ImVec4(0.08f, 0.08f, 0.08f, 0.94f);
    colors[ImGuiCol_ScrollbarGrab]  = accent;
    colors[ImGuiCol_CheckMark]      = accent;
    colors[ImGuiCol_SliderGrab]     = accent;
    colors[ImGuiCol_SliderGrabActive] = accent_active;
    colors[ImGuiCol_Button]         = accent;
    colors[ImGuiCol_ButtonHovered]  = accent_hover;
    colors[ImGuiCol_ButtonActive]   = accent_active;
    colors[ImGuiCol_Header]         = accent;
    colors[ImGuiCol_HeaderHovered]  = accent_hover;
    colors[ImGuiCol_HeaderActive]   = accent_active;
    colors[ImGuiCol_Separator]      = accent;
    colors[ImGuiCol_Tab]            = accent;
    colors[ImGuiCol_TabHovered]     = accent_hover;
    colors[ImGuiCol_TabActive]      = accent_active;
    colors[ImGuiCol_TextSelectedBg] = accent;

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 130");

    bool show_advanced_left = false;
    bool show_advanced_right = false;

    // Config profile state
    static char profile_name_buf[64] = "";
    int selected_profile = -1;
    auto profiles = ConfigFile::list_profiles();
    char status_msg[128] = "";
    float status_timer = 0;

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);

        ImGui::SetNextWindowPos(ImVec2(0, 0));
        ImGui::SetNextWindowSize(ImVec2((float)display_w, (float)display_h));

        ImGuiWindowFlags wf = ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
                              ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar;

        if (ImGui::Begin("GhostTap", nullptr, wf)) {
            ImGui::Text("GhostTap");
            ImGui::SameLine(ImGui::GetWindowWidth() - 260);

            if (state.left.is_enabled())
                ImGui::TextColored(ImVec4(0.3f, 1.0f, 0.3f, 1.0f), "L: ACTIVE");
            else
                ImGui::TextDisabled("L: ---");
            ImGui::SameLine();
            if (state.right.is_enabled())
                ImGui::TextColored(ImVec4(0.3f, 1.0f, 0.3f, 1.0f), "R: ACTIVE");
            else
                ImGui::TextDisabled("R: ---");
            ImGui::SameLine();
            if (state.in_menu)
                ImGui::TextColored(ImVec4(1.0f, 0.6f, 0.2f, 1.0f), "[INV]");

            ImGui::Separator();

            if (ImGui::BeginTabBar("##tabs")) {
                clicker_tab("Left Clicker", state.config.left, state.config.left_mode,
                            state.left, state.config.key_left, show_advanced_left);

                clicker_tab("Right Clicker", state.config.right, state.config.right_mode,
                            state.right, state.config.key_right, show_advanced_right);

                if (ImGui::BeginTabItem("General")) {
                    ImGui::Spacing();

                    ImGui::Text("Keybinds");
                    ImGui::Separator();
                    keybind_button("Inventory Key", state.config.key_inventory);
                    ImGui::TextDisabled("Pauses clicking while inventory is open");

                    ImGui::Spacing();
                    ImGui::Separator();
                    ImGui::Text("Statistics");
                    const auto& lt = state.left.tracker();
                    const auto& rt = state.right.tracker();
                    ImGui::Text("Left  - CPS: %.1f  Hold: %.1f ms  Total: %u",
                                lt.avg_cps(), lt.avg_hold_ms(), state.left.click_count());
                    ImGui::Text("Right - CPS: %.1f  Hold: %.1f ms  Total: %u",
                                rt.avg_cps(), rt.avg_hold_ms(), state.right.click_count());

                    ImGui::Spacing();
                    ImGui::Separator();
                    ImGui::Checkbox("Show Debug", &state.show_debug);
                    if (state.show_debug) {
                        ImGui::Text("FPS: %.1f (%.1f ms)", io.Framerate, 1000.0f / io.Framerate);
                        ImGui::Text("Menu paused: %s", state.in_menu ? "Yes" : "No");
                        ImGui::Text("Left armed: %s", state.left_armed ? "Yes" : "No");
                        ImGui::Text("Right armed: %s", state.right_armed ? "Yes" : "No");
                    }

                    ImGui::Spacing();
                    ImGui::Separator();
                    ImGui::TextDisabled("GhostTap - Linux/Wayland Port");

                    ImGui::EndTabItem();
                }

                if (ImGui::BeginTabItem("Config")) {
                    ImGui::Spacing();
                    ImGui::Text("Config Profiles");
                    ImGui::Separator();

                    if (ImGui::BeginListBox("##profiles", ImVec2(-1, 100))) {
                        for (int i = 0; i < (int)profiles.size(); i++) {
                            bool selected = (selected_profile == i);
                            if (ImGui::Selectable(profiles[i].c_str(), selected))
                                selected_profile = i;
                        }
                        ImGui::EndListBox();
                    }

                    ImGui::InputText("##profile_name", profile_name_buf, sizeof(profile_name_buf));

                    if (ImGui::Button("Save", ImVec2(80, 0))) {
                        std::string name = profile_name_buf[0] ? profile_name_buf : "config";
                        if (ConfigFile::save(state.config, name)) {
                            snprintf(status_msg, sizeof(status_msg), "Saved: %s", name.c_str());
                            status_timer = 3.0f;
                            profiles = ConfigFile::list_profiles();
                        }
                    }
                    ImGui::SameLine();
                    if (ImGui::Button("Load", ImVec2(80, 0)) && selected_profile >= 0 && selected_profile < (int)profiles.size()) {
                        if (ConfigFile::load(state.config, profiles[selected_profile])) {
                            snprintf(status_msg, sizeof(status_msg), "Loaded: %s", profiles[selected_profile].c_str());
                            status_timer = 3.0f;
                        }
                    }
                    ImGui::SameLine();
                    if (ImGui::Button("Delete", ImVec2(80, 0)) && selected_profile >= 0 && selected_profile < (int)profiles.size()) {
                        ConfigFile::remove_profile(profiles[selected_profile]);
                        snprintf(status_msg, sizeof(status_msg), "Deleted: %s", profiles[selected_profile].c_str());
                        status_timer = 3.0f;
                        profiles = ConfigFile::list_profiles();
                        selected_profile = -1;
                    }
                    ImGui::SameLine();
                    if (ImGui::Button("Reset", ImVec2(80, 0))) {
                        state.config = Config();
                        snprintf(status_msg, sizeof(status_msg), "Reset to defaults");
                        status_timer = 3.0f;
                    }

                    if (status_timer > 0) {
                        ImGui::TextColored(ImVec4(0.3f, 1.0f, 0.3f, 1.0f), "%s", status_msg);
                        status_timer -= io.DeltaTime;
                    }

                    ImGui::EndTabItem();
                }

                if (ImGui::BeginTabItem("Analytics")) {
                    ImGui::Spacing();
                    ImGui::Text("Click Analytics");
                    ImGui::Separator();

                    bool left_rec = state.left.tracker().is_recording();
                    bool right_rec = state.right.tracker().is_recording();

                    if (ImGui::Checkbox("Record Left", &left_rec))
                        state.left.tracker().set_recording(left_rec);
                    ImGui::SameLine();
                    if (ImGui::Checkbox("Record Right", &right_rec))
                        state.right.tracker().set_recording(right_rec);

                    ImGui::Text("Left: %d samples  |  Right: %d samples",
                                state.left.tracker().history_size(),
                                state.right.tracker().history_size());

                    if (ImGui::Button("Export CSV", ImVec2(120, 0))) {
                        std::string dir = ConfigFile::path();
                        time_t now = time(nullptr);
                        char ts[32];
                        strftime(ts, sizeof(ts), "%Y%m%d_%H%M%S", localtime(&now));

                        bool any = false;
                        if (state.left.tracker().history_size() > 0) {
                            std::string lpath = dir + "/analytics_left_" + ts + ".csv";
                            state.left.tracker().export_csv(lpath);
                            any = true;
                        }
                        if (state.right.tracker().history_size() > 0) {
                            std::string rpath = dir + "/analytics_right_" + ts + ".csv";
                            state.right.tracker().export_csv(rpath);
                            any = true;
                        }
                        if (any) {
                            snprintf(status_msg, sizeof(status_msg), "Exported to %s/", dir.c_str());
                            status_timer = 5.0f;
                        } else {
                            snprintf(status_msg, sizeof(status_msg), "No data to export. Enable recording first.");
                            status_timer = 3.0f;
                        }
                    }
                    ImGui::SameLine();
                    if (ImGui::Button("Clear Data", ImVec2(120, 0))) {
                        state.left.tracker().clear_history();
                        state.right.tracker().clear_history();
                        snprintf(status_msg, sizeof(status_msg), "Analytics data cleared");
                        status_timer = 3.0f;
                    }

                    if (status_timer > 0) {
                        ImGui::TextColored(ImVec4(0.3f, 1.0f, 0.3f, 1.0f), "%s", status_msg);
                    }

                    ImGui::EndTabItem();
                }

                ImGui::EndTabBar();
            }
        }
        ImGui::End();

        ImGui::Render();
        glViewport(0, 0, display_w, display_h);
        glClearColor(0.06f, 0.06f, 0.06f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
    }

    ConfigFile::save(state.config);

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwDestroyWindow(window);
    glfwTerminate();
}
