#pragma once
#include "clicker.hpp"
#include "clicker_config.hpp"
#include "config_file.hpp"

struct GuiState {
    Config& config;
    Clicker& left;
    Clicker& right;

    bool left_armed = false;
    bool right_armed = false;
    bool in_menu = false;

    bool show_advanced = false;
    bool show_debug = false;
};

void gui_run(GuiState& state);
