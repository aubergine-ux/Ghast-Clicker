#pragma once
#include <linux/input-event-codes.h>

enum class ActivationMode { TOGGLE, HOLD, MOUSE };
enum class DistributionType { GAUSSIAN, POISSON, UNIFORM };
enum class MacroAction { LEFT_CLICK, RIGHT_CLICK, DELAY, SWITCH_SLOT };

struct ClickerProfile {
    double cps_mean = 13.0;
    double cps_stddev = 1.5;
    double cps_min = 8.0;
    double cps_max = 18.0;
    double cps_minmax_fallout = 0.8;

    double spike_chance = 0.04;
    double spike_min = 1.0;
    double spike_max = 3.0;

    double stutter_chance = 0.03;
    double stutter_min = 4.0;
    double stutter_max = 7.0;

    double hold_ms_mean = 38.0;
    double hold_ms_stddev = 6.5;
    double hold_ms_min = 18.0;
    double hold_ms_max = 75.0;

    double hold_ms_heavy_chance = 0.015;
    double hold_ms_heavy_min = 15.0;
    double hold_ms_heavy_max = 35.0;

    double rhythm_volatility = 0.5;
    double rhythm_tension = 0.04;

    double start_delay_ms = 0.0;

    double double_click_chance = 0.0;
    double double_click_delay_min = 20.0;
    double double_click_delay_max = 55.0;

    bool breaks_enabled = false;
    double break_interval_min = 15.0;
    double break_interval_max = 45.0;
    double break_duration_min = 0.3;
    double break_duration_max = 1.5;

    int mouse_button = BTN_LEFT;
    DistributionType distribution = DistributionType::GAUSSIAN;
};

struct ClickerPreset {
    const char* name;
    ClickerProfile profile;
};

inline const ClickerPreset LEFT_PRESETS[] = {
    {"Normal (8-10 CPS)", {
        9.0, 0.8, 7.0, 11.0, 0.5,
        0.02, 0.5, 1.5,
        0.02, 2.0, 4.0,
        45.0, 8.0, 25.0, 85.0,
        0.01, 10.0, 25.0,
        0.3, 0.03, 0.0, 0.0, 20.0, 55.0, false, 15.0, 45.0, 0.3, 1.5, BTN_LEFT
    }},
    {"Jitter (12-16 CPS)", {
        14.0, 1.8, 10.0, 18.0, 1.0,
        0.06, 1.5, 4.0,
        0.05, 3.0, 6.0,
        32.0, 5.0, 15.0, 60.0,
        0.02, 12.0, 30.0,
        0.7, 0.05, 0.0, 0.0, 20.0, 55.0, false, 15.0, 45.0, 0.3, 1.5, BTN_LEFT
    }},
    {"Butterfly (16-22 CPS)", {
        19.0, 2.2, 14.0, 24.0, 1.2,
        0.08, 2.0, 5.0,
        0.04, 5.0, 8.0,
        22.0, 4.0, 10.0, 45.0,
        0.02, 8.0, 20.0,
        1.0, 0.06, 0.0, 0.0, 20.0, 55.0, false, 15.0, 45.0, 0.3, 1.5, BTN_LEFT
    }},
    {"Drag Click (25+ CPS)", {
        28.0, 3.0, 22.0, 35.0, 1.5,
        0.10, 3.0, 6.0,
        0.03, 4.0, 7.0,
        15.0, 3.0, 8.0, 30.0,
        0.01, 5.0, 15.0,
        1.2, 0.08, 0.0, 0.0, 20.0, 55.0, false, 15.0, 45.0, 0.3, 1.5, BTN_LEFT
    }},
};

inline const ClickerPreset RIGHT_PRESETS[] = {
    {"Normal (8-10 CPS)", {
        9.0, 0.8, 7.0, 11.0, 0.5,
        0.02, 0.5, 1.5,
        0.02, 2.0, 4.0,
        45.0, 8.0, 25.0, 85.0,
        0.01, 10.0, 25.0,
        0.3, 0.03, 0.0, 0.0, 20.0, 55.0, false, 15.0, 45.0, 0.3, 1.5, BTN_RIGHT
    }},
    {"Jitter (12-16 CPS)", {
        14.0, 1.8, 10.0, 18.0, 1.0,
        0.06, 1.5, 4.0,
        0.05, 3.0, 6.0,
        32.0, 5.0, 15.0, 60.0,
        0.02, 12.0, 30.0,
        0.7, 0.05, 0.0, 0.0, 20.0, 55.0, false, 15.0, 45.0, 0.3, 1.5, BTN_RIGHT
    }},
    {"Butterfly (16-22 CPS)", {
        19.0, 2.2, 14.0, 24.0, 1.2,
        0.08, 2.0, 5.0,
        0.04, 5.0, 8.0,
        22.0, 4.0, 10.0, 45.0,
        0.02, 8.0, 20.0,
        1.0, 0.06, 0.0, 0.0, 20.0, 55.0, false, 15.0, 45.0, 0.3, 1.5, BTN_RIGHT
    }},
    {"Drag Click (25+ CPS)", {
        28.0, 3.0, 22.0, 35.0, 1.5,
        0.10, 3.0, 6.0,
        0.03, 4.0, 7.0,
        15.0, 3.0, 8.0, 30.0,
        0.01, 5.0, 15.0,
        1.2, 0.08, 0.0, 0.0, 20.0, 55.0, false, 15.0, 45.0, 0.3, 1.5, BTN_RIGHT
    }},
};

constexpr int PRESET_COUNT = sizeof(LEFT_PRESETS) / sizeof(LEFT_PRESETS[0]);

struct MacroStep {
    MacroAction action = MacroAction::LEFT_CLICK;
    int count = 1;
    double delay_ms = 100.0;
    int slot = 1;
};

constexpr int MAX_MACROS = 4;
constexpr int MAX_MACRO_STEPS = 16;

struct Macro {
    char name[32] = "";
    MacroStep steps[MAX_MACRO_STEPS] = {};
    int step_count = 0;
    bool loop = false;
    int key_bind = 0;
};

struct Config {
    ClickerProfile left;
    ClickerProfile right;

    ActivationMode left_mode = ActivationMode::TOGGLE;
    ActivationMode right_mode = ActivationMode::TOGGLE;

    int key_left = KEY_F6;
    int key_right = KEY_F7;
    int key_inventory = KEY_E;
    bool inventory_key_enabled = true;
    int key_escape = KEY_ESC;

    int accent_preset = 0;
    float accent_r = 0.24f;
    float accent_g = 0.49f;
    float accent_b = 0.78f;

    bool slot_filter_enabled = false;
    bool allowed_slots[9] = {true, false, false, false, false, false, false, false, false};

    bool audio_enabled = true;
    Macro macros[MAX_MACROS] = {};

    Config() {
        left.mouse_button = BTN_LEFT;
        right.mouse_button = BTN_RIGHT;
        right.cps_mean = 15.0;
        right.cps_min = 10.0;
        right.cps_max = 20.0;
    }
};
