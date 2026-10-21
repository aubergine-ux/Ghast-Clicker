#pragma once
#include "clicker.hpp"
#include "clicker_config.hpp"
#include "combat.hpp"
#include "config_file.hpp"
#include "macro.hpp"
#include <atomic>
#include <chrono>
#include <mutex>
#include <string>

struct GuiState {
    Config& config;
    Clicker& left;
    Clicker& right;
    Combat& combat;
    MacroRunner macro_runner;

    bool left_armed = false;
    bool right_armed = false;
    bool in_menu = false;
    int active_slot = 0;

    // Set while Ghast Clicker's own window has focus, so resets never fire into it.
    std::atomic<bool> gui_focused{false};

    // Shown as a banner when the input thread can't read the keyboard.
    std::mutex error_mu;
    std::string input_error;
    void set_input_error(const std::string& e) { std::lock_guard<std::mutex> l(error_mu); input_error = e; }
    std::string get_input_error() { std::lock_guard<std::mutex> l(error_mu); return input_error; }

    bool show_advanced = false;
    bool show_debug = false;

    int selected_macro = 0;

    std::chrono::steady_clock::time_point session_start = std::chrono::steady_clock::now();
    double active_time_s = 0.0;
    double peak_cps_left = 0.0;
    double peak_cps_right = 0.0;

    GuiState(Config& c, Clicker& l, Clicker& r, Combat& cb, const UInputDevice& dev)
        : config(c), left(l), right(r), combat(cb), macro_runner(dev) {}
};

void gui_run(GuiState& state);

// Minimal window shown instead of the main GUI when startup fails
// (usually missing /dev/uinput permissions).
void gui_show_setup_error(const char* error);
