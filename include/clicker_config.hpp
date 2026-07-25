#pragma once
#include <linux/input-event-codes.h>

enum class ActivationMode { TOGGLE, HOLD, MOUSE };

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

    int mouse_button = BTN_LEFT;
};

struct Config {
    ClickerProfile left;
    ClickerProfile right;

    ActivationMode left_mode = ActivationMode::TOGGLE;
    ActivationMode right_mode = ActivationMode::TOGGLE;

    int key_left = KEY_F6;
    int key_right = KEY_F7;
    int key_inventory = KEY_E;
    int key_escape = KEY_ESC;

    Config() {
        left.mouse_button = BTN_LEFT;
        right.mouse_button = BTN_RIGHT;
        right.cps_mean = 15.0;
        right.cps_min = 10.0;
        right.cps_max = 20.0;
    }
};
