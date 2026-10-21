#include "gui.hpp"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include <GLFW/glfw3.h>
#include <cstdio>
#include <ctime>
#include <chrono>
#include <linux/input-event-codes.h>
#include <string>

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

static const ImVec4 COLOR_ON{0.3f, 1.0f, 0.3f, 1.0f};
static const ImVec4 COLOR_ARMED{1.0f, 0.8f, 0.3f, 1.0f};
static const ImVec4 COLOR_WARN{1.0f, 0.6f, 0.2f, 1.0f};
static const ImVec4 COLOR_ERROR{1.0f, 0.35f, 0.35f, 1.0f};

static void tooltip(const char* text) {
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("%s", text);
}

// Grey "(?)" that explains the item before it.
static void help_marker(const char* text) {
    ImGui::SameLine();
    ImGui::TextDisabled("(?)");
    tooltip(text);
}

static void section(const char* title) {
    ImGui::Spacing();
    ImGui::Text("%s", title);
    ImGui::Separator();
}

static bool slider(const char* label, double& v, double lo, double hi, const char* fmt,
                   const char* tip = nullptr) {
    bool changed = ImGui::SliderScalar(label, ImGuiDataType_Double, &v, &lo, &hi, fmt);
    if (tip) tooltip(tip);
    return changed;
}

// Stored as 0..1, shown as a percentage.
static void chance_slider(const char* label, double& v, const char* tip) {
    double pct = v * 100.0;
    if (slider(label, pct, 0.0, 100.0, "%.0f%%", tip))
        v = pct / 100.0;
}

static void keybind_input(const char* id, int& key_code, float width = 150) {
    char buf[64];
    if (g_binding_target == &key_code) {
        snprintf(buf, sizeof(buf), "Press a key...##%s", id);
    } else if (key_code == 0) {
        snprintf(buf, sizeof(buf), "Click to bind##%s", id);
    } else {
        snprintf(buf, sizeof(buf), "%s##%s", key_name(key_code), id);
    }

    if (ImGui::Button(buf, ImVec2(width, 0))) {
        g_binding_target = &key_code;
    }
    tooltip("Click, then press the key you want. Escape clears it.");
}

static void keybind_button(const char* label, int& key_code) {
    keybind_input(label, key_code);
    ImGui::SameLine();
    ImGui::Text("%s", label);
}

static const char* MODE_HELP =
    "Toggle: press the key to start, press again to stop.\n"
    "Hold: clicks only while the key is held.\n"
    "Mouse: the key arms the clicker, then holding your real\n"
    "mouse button auto-clicks. Best for PvP.";

static void mode_combo(const char* label, ActivationMode& mode) {
    int idx = mode == ActivationMode::TOGGLE ? 0 : (mode == ActivationMode::HOLD ? 1 : 2);
    if (ImGui::Combo(label, &idx, "Toggle\0Hold\0Mouse\0")) {
        mode = idx == 0 ? ActivationMode::TOGGLE : (idx == 1 ? ActivationMode::HOLD : ActivationMode::MOUSE);
    }
    tooltip(MODE_HELP);
}

static void clicker_status(const Clicker& clicker, ActivationMode mode, bool armed) {
    if (clicker.is_enabled())
        ImGui::TextColored(COLOR_ON, "CLICKING");
    else if (mode == ActivationMode::MOUSE && armed)
        ImGui::TextColored(COLOR_ARMED, "ARMED");
    else
        ImGui::TextDisabled("Off");
}

template <typename Set>
static void module_checkbox(const char* label, bool on, Set&& set) {
    bool v = on;
    if (ImGui::Checkbox(label, &v)) set(v);
}

static void combat_status(bool enabled, bool busy) {
    if (busy)
        ImGui::TextColored(COLOR_ARMED, "FIRING");
    else if (enabled)
        ImGui::TextColored(COLOR_ON, "ON");
    else
        ImGui::TextDisabled("Off");
}

static void keyboard_grab_status(GuiState& state) {
    if (!state.combat.sprint.needs_keyboard_grab()) return;
    if (state.combat.keyboard.forwarding())
        ImGui::TextColored(COLOR_ON, "Keyboard grabbed - W-Tap ready");
    else
        ImGui::TextColored(COLOR_WARN, "Release all keys to finish grabbing the keyboard for W-Tap");
}

static void clicker_tab(const char* name, ClickerProfile& profile, ActivationMode& mode,
                         Clicker& clicker, int& key, bool& show_advanced, bool armed,
                         const ClickerPreset* presets) {
    if (!ImGui::BeginTabItem(name)) return;

    ImGui::Spacing();

    keybind_button("Toggle Key", key);

    ImGui::SameLine();
    clicker_status(clicker, mode, armed);

    ImGui::Separator();
    ImGui::Text("Presets");
    help_marker("One click sets every slider below. Fine-tune afterwards if you like.");
    for (int i = 0; i < PRESET_COUNT; i++) {
        if (i > 0) ImGui::SameLine();
        if (ImGui::Button(presets[i].name)) {
            int saved_button = profile.mouse_button;
            profile = presets[i].profile;
            profile.mouse_button = saved_button;
        }
    }

    ImGui::Separator();
    ImGui::Text("Activation");
    help_marker(MODE_HELP);
    mode_combo("##mode", mode);

    ImGui::Separator();
    ImGui::Text("CPS");
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("Ctrl+Click a slider to type a value directly.\nValues between 9.5 - 12.5 recommended for server-side AC bypass.");

    ImGui::SliderScalar("Mean CPS", ImGuiDataType_Double, &profile.cps_mean, &(const double&){1.0}, &(const double&){25.0}, "%.1f");
    ImGui::SliderScalar("Std Dev", ImGuiDataType_Double, &profile.cps_stddev, &(const double&){0.0}, &(const double&){5.0}, "%.2f");
    ImGui::SliderScalar("Min CPS", ImGuiDataType_Double, &profile.cps_min, &(const double&){1.0}, &(const double&){20.0}, "%.1f");
    ImGui::SliderScalar("Max CPS", ImGuiDataType_Double, &profile.cps_max, &(const double&){5.0}, &(const double&){30.0}, "%.1f");

    {
        int dist_idx = static_cast<int>(profile.distribution);
        if (ImGui::Combo("Distribution", &dist_idx, "Gaussian\0Poisson\0Uniform\0"))
            profile.distribution = static_cast<DistributionType>(dist_idx);
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Gaussian: bell curve around mean (default)\n"
                              "Poisson: natural integer variance, stddev ignored\n"
                              "Uniform: flat random between min/max CPS");
        }
    }

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
    ImGui::Text("Double Click");
    ImGui::SliderScalar("Chance##dbl", ImGuiDataType_Double, &profile.double_click_chance, &(const double&){0.0}, &(const double&){0.3}, "%.3f");
    if (profile.double_click_chance > 0) {
        ImGui::SliderScalar("Delay Min (ms)##dbl", ImGuiDataType_Double, &profile.double_click_delay_min, &(const double&){5.0}, &(const double&){80.0}, "%.0f");
        ImGui::SliderScalar("Delay Max (ms)##dbl", ImGuiDataType_Double, &profile.double_click_delay_max, &(const double&){10.0}, &(const double&){120.0}, "%.0f");
    }

    ImGui::Separator();
    ImGui::Text("Fatigue Breaks");
    ImGui::Checkbox("Enable Breaks", &profile.breaks_enabled);
    if (profile.breaks_enabled) {
        ImGui::SliderScalar("Break Every Min (s)", ImGuiDataType_Double, &profile.break_interval_min, &(const double&){5.0}, &(const double&){120.0}, "%.0f");
        ImGui::SliderScalar("Break Every Max (s)", ImGuiDataType_Double, &profile.break_interval_max, &(const double&){5.0}, &(const double&){120.0}, "%.0f");
        ImGui::SliderScalar("Break Duration Min (s)", ImGuiDataType_Double, &profile.break_duration_min, &(const double&){0.1}, &(const double&){5.0}, "%.1f");
        ImGui::SliderScalar("Break Duration Max (s)", ImGuiDataType_Double, &profile.break_duration_max, &(const double&){0.1}, &(const double&){5.0}, "%.1f");
    }

    ImGui::Separator();
    const auto& t = clicker.tracker();
    ImGui::Text("Status: %s", clicker.is_enabled() ? "ACTIVE" : "Idle");
    ImGui::Text("Avg CPS: %.1f  |  Avg Hold: %.1f ms  |  Clicks: %u",
                t.avg_cps(), t.avg_hold_ms(), clicker.click_count());

    ImGui::EndTabItem();
}

static void home_tab(GuiState& state) {
    if (!ImGui::BeginTabItem("Home")) return;
    ImGui::Spacing();

    auto& cfg = state.config;
    auto& combat = state.combat;

    ImGui::TextWrapped("Hotkeys work anywhere: switch to your game and press them. "
                       "Click a key button below to rebind it.");
    ImGui::Spacing();

    ImGuiTableFlags tf = ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersInnerH |
                         ImGuiTableFlags_SizingStretchProp;
    if (ImGui::BeginTable("##modules", 4, tf)) {
        ImGui::TableSetupColumn("Module", ImGuiTableColumnFlags_WidthStretch, 1.2f);
        ImGui::TableSetupColumn("Hotkey", ImGuiTableColumnFlags_WidthFixed, 130.0f);
        ImGui::TableSetupColumn("Mode", ImGuiTableColumnFlags_WidthStretch, 1.2f);
        ImGui::TableSetupColumn("Status", ImGuiTableColumnFlags_WidthStretch, 0.8f);
        ImGui::TableHeadersRow();

        auto clicker_row = [&](const char* name, const char* id, Clicker& c, int& key,
                               ActivationMode& mode, bool armed) {
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::AlignTextToFramePadding();
            ImGui::Text("%s", name);
            ImGui::TableNextColumn();
            keybind_input(id, key, -1);
            ImGui::TableNextColumn();
            ImGui::SetNextItemWidth(-1);
            char combo_id[32];
            snprintf(combo_id, sizeof(combo_id), "##mode_%s", id);
            mode_combo(combo_id, mode);
            ImGui::TableNextColumn();
            ImGui::AlignTextToFramePadding();
            clicker_status(c, mode, armed);
        };

        clicker_row("Left Clicker", "home_left", state.left, cfg.key_left, cfg.left_mode, state.left_armed);
        clicker_row("Right Clicker", "home_right", state.right, cfg.key_right, cfg.right_mode, state.right_armed);

        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        module_checkbox("Jump Reset##home", combat.jump.is_enabled(),
                        [&](bool v) { combat.jump.set_enabled(v); });
        ImGui::TableNextColumn();
        keybind_input("home_jump", cfg.key_jump_reset, -1);
        ImGui::TableNextColumn();
        ImGui::AlignTextToFramePadding();
        ImGui::TextDisabled("Jumps after you hit");
        ImGui::TableNextColumn();
        ImGui::AlignTextToFramePadding();
        combat_status(combat.jump.is_enabled(), combat.jump.busy());

        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        module_checkbox("Sprint Reset##home", combat.sprint.is_enabled(),
                        [&](bool v) { combat.sprint.set_enabled(v); });
        ImGui::TableNextColumn();
        keybind_input("home_sprint", cfg.key_sprint_reset, -1);
        ImGui::TableNextColumn();
        {
            int m = static_cast<int>(cfg.sprint_reset.mode);
            ImGui::SetNextItemWidth(-1);
            if (ImGui::Combo("##home_sprint_mode", &m, "W-Tap\0S-Tap\0Shift-Tap\0"))
                cfg.sprint_reset.mode = static_cast<SprintResetMode>(m);
        }
        ImGui::TableNextColumn();
        ImGui::AlignTextToFramePadding();
        combat_status(combat.sprint.is_enabled(), combat.sprint.busy());

        ImGui::EndTable();
    }
    keyboard_grab_status(state);

    ImGui::Spacing();
    if (ImGui::CollapsingHeader("Quick start", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::BulletText("Pick a preset on the Left / Right tab (or keep the defaults).");
        ImGui::BulletText("Choose a mode. Mouse mode is the most natural for PvP:");
        ImGui::Indent();
        ImGui::TextDisabled("press the hotkey once to arm, then just hold your mouse button.");
        ImGui::Unindent();
        ImGui::BulletText("Turn on Jump Reset / Sprint Reset here or with their hotkeys.");
        ImGui::BulletText("Inventory key (%s) pauses everything while your inventory is open.",
                          key_name(cfg.key_inventory));
        ImGui::BulletText("Settings save automatically when you close the window.");
    }

    ImGui::EndTabItem();
}

static void combat_tab(GuiState& state) {
    if (!ImGui::BeginTabItem("Combat")) return;
    ImGui::Spacing();

    auto& combat = state.combat;
    auto& jr = state.config.jump_reset;
    auto& sr = state.config.sprint_reset;

    ImGui::TextWrapped("Both modules react to your own left clicks, real or auto-clicked. "
                       "Ghast Clicker can't see game packets, so a cooldown stands in for "
                       "hit detection: at most one reset per cooldown window.");
    ImGui::Spacing();

    if (ImGui::CollapsingHeader("Jump Reset", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::PushID("jr");
        module_checkbox("Enabled", combat.jump.is_enabled(),
                        [&](bool v) { combat.jump.set_enabled(v); });
        ImGui::SameLine(160);
        keybind_input("jr_key", state.config.key_jump_reset, 120);
        ImGui::SameLine();
        combat_status(combat.jump.is_enabled(), combat.jump.busy());
        ImGui::TextDisabled("Taps Space right after you attack, so you jump as the return hit lands "
                            "and take less knockback.");

        section("Timing");
        chance_slider("Chance", jr.chance, "How often a hit window triggers a jump.");
        slider("Delay Mean (ms)", jr.delay_ms_mean, 0, 150, "%.0f", "Average reaction time after your click.");
        slider("Delay StdDev (ms)", jr.delay_ms_stddev, 0, 50, "%.1f");
        slider("Delay Min (ms)", jr.delay_ms_min, 0, 150, "%.0f");
        slider("Delay Max (ms)", jr.delay_ms_max, 0, 250, "%.0f");
        slider("Hold Mean (ms)", jr.hold_ms_mean, 30, 200, "%.0f", "How long Space stays down.");
        slider("Hold StdDev (ms)", jr.hold_ms_stddev, 0, 50, "%.1f");
        slider("Hold Min (ms)", jr.hold_ms_min, 30, 200, "%.0f");
        slider("Hold Max (ms)", jr.hold_ms_max, 30, 250, "%.0f");
        slider("Cooldown (ms)", jr.cooldown_ms, 200, 1500, "%.0f",
               "Minimum time between jumps. A jump lasts ~600ms, so lower values can\n"
               "fire while you're still airborne.");

        section("Filters");
        ImGui::Checkbox("Only while holding W", &jr.forward_only);
        ImGui::Checkbox("Allow while inventory is open", &jr.allow_in_menu);
        ImGui::Checkbox("Respect hotbar slot filter", &jr.use_slot_filter);
        tooltip("Uses the slot filter from the General tab (when enabled there).");

        const auto& log = combat.jump.log();
        ImGui::Text("Jumps: %u / %u decisions  |  Avg delay: %.1f ms  |  Avg hold: %.1f ms",
                    log.count(ResetOutcome::TRIGGERED), log.decisions(),
                    log.avg_delay_ms(), log.avg_hold_ms());
        if (ImGui::SmallButton("Reset to defaults")) jr = JumpResetProfile{};
        ImGui::PopID();
    }

    ImGui::Spacing();
    if (ImGui::CollapsingHeader("Sprint Reset", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::PushID("sr");
        module_checkbox("Enabled", combat.sprint.is_enabled(),
                        [&](bool v) { combat.sprint.set_enabled(v); });
        ImGui::SameLine(160);
        keybind_input("sr_key", state.config.key_sprint_reset, 120);
        ImGui::SameLine();
        combat_status(combat.sprint.is_enabled(), combat.sprint.busy());
        ImGui::TextDisabled("Breaks sprint for a tick or two after you attack so your next hit "
                            "deals full sprint knockback.");

        int m = static_cast<int>(sr.mode);
        if (ImGui::Combo("Mode", &m, "W-Tap\0S-Tap\0Shift-Tap\0"))
            sr.mode = static_cast<SprintResetMode>(m);
        tooltip("W-Tap: briefly releases W (grabs your keyboard while on).\n"
                "S-Tap: briefly presses S.\n"
                "Shift-Tap: briefly presses Shift.");
        keyboard_grab_status(state);

        section("Timing");
        chance_slider("Chance", sr.chance, "How often a hit window triggers a reset.");
        slider("Delay Min (ms)", sr.delay_ms_min, 0, 500, "%.0f", "Random wait after your click before the tap.");
        slider("Delay Max (ms)", sr.delay_ms_max, 0, 500, "%.0f");
        slider("Duration Mean (ticks)", sr.duration_ticks_mean, 1, 3, "%.2f",
               "1 tick = 50ms. Fractional means mix whole-tick taps to hit the average.");
        slider("Duration StdDev (ticks)", sr.duration_ticks_stddev, 0, 1, "%.2f");
        slider("Duration Min (ticks)", sr.duration_ticks_min, 1, 3, "%.0f");
        slider("Duration Max (ticks)", sr.duration_ticks_max, 1, 3, "%.0f");
        slider("Cooldown (ms)", sr.cooldown_ms, 200, 1500, "%.0f",
               "Minimum time between resets. Players can only take a hit every 500ms.");

        section("Filters");
        if (sr.mode == SprintResetMode::W_TAP) {
            ImGui::BeginDisabled();
            bool always = true;
            ImGui::Checkbox("Only while holding W", &always);
            ImGui::EndDisabled();
            tooltip("W-Tap always needs W held.");
        } else {
            ImGui::Checkbox("Only while holding W", &sr.forward_only);
        }
        ImGui::Checkbox("Respect hotbar slot filter", &sr.use_slot_filter);
        tooltip("Uses the slot filter from the General tab (when enabled there).");

        const auto& log = combat.sprint.log();
        ImGui::Text("Resets: %u / %u decisions  |  Avg delay: %.1f ms  |  Avg length: %.1f ms",
                    log.count(ResetOutcome::TRIGGERED), log.decisions(),
                    log.avg_delay_ms(), log.avg_hold_ms());
        if (ImGui::SmallButton("Reset to defaults")) sr = SprintResetProfile{};
        ImGui::PopID();
    }

    ImGui::EndTabItem();
}

static GLFWwindow* create_window(int w, int h) {
    if (!glfwInit()) {
        fprintf(stderr, "Failed to initialize GLFW\n");
        return nullptr;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
    glfwWindowHint(GLFW_DECORATED, GLFW_FALSE);

    GLFWwindow* window = glfwCreateWindow(w, h, "Ghast Clicker", nullptr, nullptr);
    if (!window) {
        fprintf(stderr, "Failed to create GLFW window\n");
        glfwTerminate();
        return nullptr;
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
    return window;
}

static void destroy_window(GLFWwindow* window) {
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwDestroyWindow(window);
    glfwTerminate();
}

// Drag the undecorated window by its top 30px.
struct WindowDrag {
    bool dragging = false;
    double start_x = 0, start_y = 0;
    int win_x = 0, win_y = 0;

    void update(GLFWwindow* window) {
        if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
            double mx, my;
            glfwGetCursorPos(window, &mx, &my);
            if (!dragging && my < 30.0 && !ImGui::IsAnyItemActive()) {
                dragging = true;
                start_x = mx;
                start_y = my;
                glfwGetWindowPos(window, &win_x, &win_y);
            }
            if (dragging)
                glfwSetWindowPos(window, win_x + (int)(mx - start_x), win_y + (int)(my - start_y));
        } else {
            dragging = false;
        }
    }
};

static void begin_frame(GLFWwindow* window, int& display_w, int& display_h) {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    glfwGetFramebufferSize(window, &display_w, &display_h);
    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(ImVec2((float)display_w, (float)display_h));
}

static void end_frame(GLFWwindow* window, int display_w, int display_h) {
    ImGui::Render();
    glViewport(0, 0, display_w, display_h);
    glClearColor(0.06f, 0.06f, 0.06f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    glfwSwapBuffers(window);
}

static const ImGuiWindowFlags MAIN_WINDOW_FLAGS =
    ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
    ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar;

void gui_show_setup_error(const char* error) {
    GLFWwindow* window = create_window(560, 360);
    if (!window) return;

    WindowDrag drag;
    bool copied = false;
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        drag.update(window);

        int w, h;
        begin_frame(window, w, h);
        if (ImGui::Begin("Ghast Clicker", nullptr, MAIN_WINDOW_FLAGS)) {
            ImGui::Text("Ghast Clicker");
            ImGui::Separator();
            ImGui::TextColored(COLOR_ERROR, "Setup needed");
            ImGui::Spacing();
            ImGui::TextWrapped("%s", error);
            ImGui::Spacing();
            if (ImGui::Button("Copy fix commands", ImVec2(160, 0))) {
                ImGui::SetClipboardText(
                    "sudo usermod -aG input $USER\n"
                    "sudo cp 99-ghast-clicker.rules /etc/udev/rules.d/\n"
                    "sudo udevadm control --reload-rules && sudo udevadm trigger\n"
                    "sudo modprobe uinput\n");
                copied = true;
            }
            ImGui::SameLine();
            if (ImGui::Button("Quit", ImVec2(80, 0)))
                glfwSetWindowShouldClose(window, GLFW_TRUE);
            if (copied) {
                ImGui::SameLine();
                ImGui::TextColored(COLOR_ON, "Copied!");
            }
        }
        ImGui::End();
        end_frame(window, w, h);
    }

    destroy_window(window);
}

void gui_run(GuiState& state) {
    GLFWwindow* window = create_window(680, 600);
    if (!window) return;
    ImGuiIO& io = ImGui::GetIO();

    bool show_advanced_left = false;
    bool show_advanced_right = false;

    // Config profile state
    static char profile_name_buf[64] = "";
    int selected_profile = -1;
    auto profiles = ConfigFile::list_profiles();
    char status_msg[128] = "";
    float status_timer = 0;

    bool always_on_top = false;
    WindowDrag drag;

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        drag.update(window);
        state.gui_focused = glfwGetWindowAttrib(window, GLFW_FOCUSED) == GLFW_TRUE;

        int display_w, display_h;
        begin_frame(window, display_w, display_h);

        if (ImGui::Begin("Ghast Clicker", nullptr, MAIN_WINDOW_FLAGS)) {
            ImGui::Text("Ghast Clicker");
            ImGui::SameLine(ImGui::GetWindowWidth() - 400);

            if (ImGui::SmallButton(always_on_top ? "[pinned]" : "[pin]")) {
                always_on_top = !always_on_top;
                glfwSetWindowAttrib(window, GLFW_FLOATING, always_on_top ? GLFW_TRUE : GLFW_FALSE);
            }
            ImGui::SameLine();

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
            if (state.combat.jump.is_enabled())
                ImGui::TextColored(COLOR_ON, "JR: ON");
            else
                ImGui::TextDisabled("JR: ---");
            ImGui::SameLine();
            if (state.combat.sprint.is_enabled())
                ImGui::TextColored(COLOR_ON, "SR: ON");
            else
                ImGui::TextDisabled("SR: ---");
            ImGui::SameLine();
            if (state.in_menu)
                ImGui::TextColored(COLOR_WARN, "[INV]");

            std::string input_error = state.get_input_error();
            if (!input_error.empty())
                ImGui::TextColored(COLOR_ERROR, "%s", input_error.c_str());

            ImGui::Separator();

            if (ImGui::BeginTabBar("##tabs", ImGuiTabBarFlags_FittingPolicyScroll)) {
                home_tab(state);

                clicker_tab("Left", state.config.left, state.config.left_mode,
                            state.left, state.config.key_left, show_advanced_left,
                            state.left_armed, LEFT_PRESETS);

                clicker_tab("Right", state.config.right, state.config.right_mode,
                            state.right, state.config.key_right, show_advanced_right,
                            state.right_armed, RIGHT_PRESETS);

                combat_tab(state);

                if (ImGui::BeginTabItem("General")) {
                    ImGui::Spacing();

                    ImGui::Text("Keybinds");
                    ImGui::Separator();
                    keybind_button("Inventory Key", state.config.key_inventory);
                    ImGui::TextDisabled("Pauses clicking while inventory is open");
                    keybind_button("Jump Reset Key", state.config.key_jump_reset);
                    keybind_button("Sprint Reset Key", state.config.key_sprint_reset);

                    ImGui::Spacing();
                    ImGui::Separator();
                    ImGui::Text("Hotbar Slot Filter");
                    ImGui::TextDisabled("Only click when holding certain slots (e.g. sword slot)");
                    ImGui::Checkbox("Enable Slot Filter", &state.config.slot_filter_enabled);
                    if (state.config.slot_filter_enabled) {
                        for (int i = 0; i < 9; i++) {
                            if (i > 0) ImGui::SameLine();
                            char slabel[4];
                            snprintf(slabel, sizeof(slabel), "%d", i + 1);
                            ImGui::Checkbox(slabel, &state.config.allowed_slots[i]);
                        }
                        ImGui::Text("Active slot: %d", state.active_slot + 1);
                    }

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
                    ImGui::Text("Audio");
                    ImGui::Checkbox("Toggle Sound", &state.config.audio_enabled);
                    if (ImGui::IsItemHovered())
                        ImGui::SetTooltip("Play a beep when clickers toggle on/off");

                    ImGui::Spacing();
                    ImGui::Separator();
                    ImGui::Text("Session Stats");
                    {
                        bool any_active = state.left.is_enabled() || state.right.is_enabled();
                        if (any_active)
                            state.active_time_s += io.DeltaTime;

                        if (state.left.is_enabled()) {
                            double cps = lt.avg_cps();
                            if (cps > state.peak_cps_left) state.peak_cps_left = cps;
                        }
                        if (state.right.is_enabled()) {
                            double cps = rt.avg_cps();
                            if (cps > state.peak_cps_right) state.peak_cps_right = cps;
                        }

                        auto elapsed = std::chrono::steady_clock::now() - state.session_start;
                        double session_secs = std::chrono::duration<double>(elapsed).count();
                        int s_min = (int)session_secs / 60;
                        int s_sec = (int)session_secs % 60;
                        int a_min = (int)state.active_time_s / 60;
                        int a_sec = (int)state.active_time_s % 60;
                        double pct = session_secs > 0 ? state.active_time_s / session_secs * 100.0 : 0;
                        uint32_t total = state.left.click_count() + state.right.click_count();
                        double avg = state.active_time_s > 0 ? total / state.active_time_s : 0;

                        ImGui::Text("Duration: %dm %02ds", s_min, s_sec);
                        ImGui::Text("Active: %dm %02ds (%.0f%%)", a_min, a_sec, pct);
                        ImGui::Text("Total Clicks: %u (L: %u | R: %u)",
                                    total, state.left.click_count(), state.right.click_count());
                        ImGui::Text("Session Avg CPS: %.1f", avg);
                        ImGui::Text("Peak CPS - L: %.1f | R: %.1f",
                                    state.peak_cps_left, state.peak_cps_right);
                    }

                    ImGui::Spacing();
                    ImGui::Separator();
                    ImGui::Checkbox("Show Debug", &state.show_debug);
                    if (state.show_debug) {
                        ImGui::Text("FPS: %.1f (%.1f ms)", io.Framerate, 1000.0f / io.Framerate);
                        ImGui::Text("Menu paused: %s", state.in_menu ? "Yes" : "No");
                        ImGui::Text("Left armed: %s", state.left_armed ? "Yes" : "No");
                        ImGui::Text("Right armed: %s", state.right_armed ? "Yes" : "No");
                        ImGui::Text("Keyboard grabbed: %s", state.combat.keyboard.forwarding() ? "Yes" : "No");
                        ImGui::Text("Window focused: %s", state.gui_focused ? "Yes" : "No");
                    }

                    ImGui::Spacing();
                    ImGui::Separator();
                    ImGui::TextDisabled("Ghast Clicker - Linux/Wayland Port");

                    ImGui::EndTabItem();
                }

                if (ImGui::BeginTabItem("Macros")) {
                    ImGui::Spacing();
                    ImGui::Text("Macro Sequences");
                    ImGui::Separator();

                    for (int m = 0; m < MAX_MACROS; m++) {
                        if (m > 0) ImGui::SameLine();
                        char mlabel[16];
                        snprintf(mlabel, sizeof(mlabel), "%d##mtab", m + 1);
                        bool active = (state.selected_macro == m);
                        if (active) ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.65f, 0.48f, 0.82f, 1.0f));
                        if (ImGui::Button(mlabel, ImVec2(40, 0)))
                            state.selected_macro = m;
                        if (active) ImGui::PopStyleColor();
                    }

                    auto& mac = state.config.macros[state.selected_macro];

                    ImGui::InputText("Name", mac.name, sizeof(mac.name));
                    ImGui::Checkbox("Loop", &mac.loop);
                    ImGui::SameLine();
                    keybind_button("Macro Key", mac.key_bind);

                    ImGui::Separator();
                    ImGui::Text("Steps (%d/%d)", mac.step_count, MAX_MACRO_STEPS);

                    int remove_idx = -1;
                    for (int s = 0; s < mac.step_count; s++) {
                        ImGui::PushID(s);
                        auto& step = mac.steps[s];

                        int action_idx = static_cast<int>(step.action);
                        ImGui::SetNextItemWidth(120);
                        if (ImGui::Combo("##action", &action_idx, "Left Click\0Right Click\0Delay\0Switch Slot\0"))
                            step.action = static_cast<MacroAction>(action_idx);

                        ImGui::SameLine();
                        switch (step.action) {
                            case MacroAction::LEFT_CLICK:
                            case MacroAction::RIGHT_CLICK:
                                ImGui::SetNextItemWidth(60);
                                ImGui::InputInt("##cnt", &step.count, 0);
                                if (step.count < 1) step.count = 1;
                                ImGui::SameLine();
                                ImGui::SetNextItemWidth(80);
                                ImGui::InputDouble("ms##intv", &step.delay_ms, 0, 0, "%.0f");
                                if (step.delay_ms < 1) step.delay_ms = 1;
                                break;
                            case MacroAction::DELAY:
                                ImGui::SetNextItemWidth(100);
                                ImGui::InputDouble("ms##del", &step.delay_ms, 0, 0, "%.0f");
                                if (step.delay_ms < 1) step.delay_ms = 1;
                                break;
                            case MacroAction::SWITCH_SLOT:
                                ImGui::SetNextItemWidth(60);
                                ImGui::InputInt("Slot##sw", &step.slot, 0);
                                if (step.slot < 1) step.slot = 1;
                                if (step.slot > 9) step.slot = 9;
                                break;
                        }

                        ImGui::SameLine();
                        if (ImGui::SmallButton("X"))
                            remove_idx = s;

                        ImGui::PopID();
                    }

                    if (remove_idx >= 0) {
                        for (int s = remove_idx; s < mac.step_count - 1; s++)
                            mac.steps[s] = mac.steps[s + 1];
                        mac.step_count--;
                    }

                    if (mac.step_count < MAX_MACRO_STEPS) {
                        if (ImGui::Button("+ Add Step"))
                            mac.steps[mac.step_count++] = MacroStep{};
                    }

                    ImGui::Separator();
                    if (state.macro_runner.is_running()) {
                        ImGui::TextColored(ImVec4(0.3f, 1.0f, 0.3f, 1.0f),
                            "Running (step %d/%d, loop %d)",
                            state.macro_runner.current_step() + 1,
                            mac.step_count, state.macro_runner.loop_count());
                        if (ImGui::Button("Stop", ImVec2(100, 0)))
                            state.macro_runner.stop();
                    } else {
                        ImGui::TextDisabled("Idle");
                        if (mac.step_count > 0) {
                            if (ImGui::Button("Run", ImVec2(100, 0)))
                                state.macro_runner.start(mac, &state.active_slot);
                        }
                    }

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

                    ImGui::Text("Left CPS");
                    {
                        const auto& t = state.left.tracker();
                        char overlay[32];
                        snprintf(overlay, sizeof(overlay), "%.1f CPS", t.avg_cps());
                        ImGui::PlotLines("##lcps", t.graph_data(), t.graph_size(),
                                         t.graph_offset(), overlay, 0.0f, 35.0f, ImVec2(-1, 60));
                    }
                    ImGui::Text("Right CPS");
                    {
                        const auto& t = state.right.tracker();
                        char overlay[32];
                        snprintf(overlay, sizeof(overlay), "%.1f CPS", t.avg_cps());
                        ImGui::PlotLines("##rcps", t.graph_data(), t.graph_size(),
                                         t.graph_offset(), overlay, 0.0f, 35.0f, ImVec2(-1, 60));
                    }

                    ImGui::Separator();

                    bool left_rec = state.left.tracker().is_recording();
                    bool right_rec = state.right.tracker().is_recording();

                    if (ImGui::Checkbox("Record Left", &left_rec))
                        state.left.tracker().set_recording(left_rec);
                    ImGui::SameLine();
                    if (ImGui::Checkbox("Record Right", &right_rec))
                        state.right.tracker().set_recording(right_rec);

                    ImGui::SameLine();
                    bool combat_rec = state.combat.jump.log().is_recording();
                    if (ImGui::Checkbox("Record Combat", &combat_rec)) {
                        state.combat.jump.log().set_recording(combat_rec);
                        state.combat.sprint.log().set_recording(combat_rec);
                    }
                    tooltip("Logs every Jump Reset / Sprint Reset decision with its delay and length.");

                    ImGui::Text("Left: %d  |  Right: %d  |  Jump Reset: %d  |  Sprint Reset: %d samples",
                                state.left.tracker().history_size(),
                                state.right.tracker().history_size(),
                                state.combat.jump.log().history_size(),
                                state.combat.sprint.log().history_size());

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
                        if (state.combat.jump.log().history_size() > 0) {
                            state.combat.jump.log().export_csv(dir + "/analytics_jump_reset_" + ts + ".csv");
                            any = true;
                        }
                        if (state.combat.sprint.log().history_size() > 0) {
                            state.combat.sprint.log().export_csv(dir + "/analytics_sprint_reset_" + ts + ".csv");
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
                        state.combat.jump.log().clear();
                        state.combat.sprint.log().clear();
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
        end_frame(window, display_w, display_h);
    }

    ConfigFile::save(state.config);
    destroy_window(window);
}
