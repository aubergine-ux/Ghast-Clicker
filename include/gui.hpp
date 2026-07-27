#pragma once
#include "clicker.hpp"
#include "clicker_config.hpp"
#include "config_file.hpp"
#include "macro.hpp"
#include <chrono>

struct GuiState {
    Config& config;
    Clicker& left;
    Clicker& right;
    MacroRunner macro_runner;

    bool left_armed = false;
    bool right_armed = false;
    bool in_menu = false;
    int active_slot = 0;

    bool show_advanced = false;
    bool show_debug = false;

    int selected_macro = 0;

    std::chrono::steady_clock::time_point session_start = std::chrono::steady_clock::now();
    double active_time_s = 0.0;
    double peak_cps_left = 0.0;
    double peak_cps_right = 0.0;

    GuiState(Config& c, Clicker& l, Clicker& r, const UInputDevice& dev)
        : config(c), left(l), right(r), macro_runner(dev) {}
};

void gui_run(GuiState& state);
