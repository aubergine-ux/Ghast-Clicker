#include "gui.hpp"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include <GLFW/glfw3.h>
#include <cstdio>
#include <ctime>
#include <chrono>
#include <cstring>
#include <linux/input-event-codes.h>
#include <cfloat>
#include <cstdlib>

// Global accent color state
static ImVec4 g_accent_color(0.24f, 0.49f, 0.78f, 1.0f);  // Default Oxygen blue
static ImVec4 g_accent_hover(0.33f, 0.58f, 0.86f, 1.0f);
static ImVec4 g_accent_active(0.17f, 0.37f, 0.62f, 1.0f);

// Read KDE accent color from kdeglobals
static bool read_kde_accent_color(float &r, float &g, float &b) {
    const char* home = getenv("HOME");
    if (!home) return false;

    std::string path = std::string(home) + "/.config/kdeglobals";
    FILE* f = fopen(path.c_str(), "r");
    if (!f) return false;

    char line[256];
    bool in_colors = false;

    while (fgets(line, sizeof(line), f)) {
        if (strstr(line, "[Colors:Selection]")) {
            in_colors = true;
            continue;
        }
        if (line[0] == '[' && !strstr(line, "[Colors:Selection]")) {
            in_colors = false;
            continue;
        }
        if (in_colors) {
            char* eq = strchr(line, '=');
            if (eq && strstr(line, "BackgroundNormal")) {
                char* val = eq + 1;
                int cr, cg, cb;
                if (sscanf(val, "%d,%d,%d", &cr, &cg, &cb) == 3) {
                    r = cr / 255.0f;
                    g = cg / 255.0f;
                    b = cb / 255.0f;
                    fclose(f);
                    return true;
                }
            }
        }
    }
    fclose(f);
    return false;
}

// Apply complete Oxygen-style theme with custom accent
static void setup_oxygen_theme(int preset, float custom_r, float custom_g, float custom_b) {
    ImGuiStyle& style = ImGui::GetStyle();

    // Set base accent color based on preset
    if (preset == 0) {
        g_accent_color = ImVec4(0.24f, 0.49f, 0.78f, 1.0f); // Oxygen Blue
    } else if (preset == 1) {
        g_accent_color = ImVec4(0.80f, 0.20f, 0.20f, 1.0f); // Red
    } else if (preset == 2) {
        g_accent_color = ImVec4(0.20f, 0.80f, 0.30f, 1.0f); // Green
    } else if (preset == 3) {
        g_accent_color = ImVec4(0.60f, 0.30f, 0.90f, 1.0f); // Purple
    } else if (preset == 4) {
        g_accent_color = ImVec4(custom_r, custom_g, custom_b, 1.0f); // Custom
    } else if (preset == 5) {
        float kr, kg, kb;
        if (read_kde_accent_color(kr, kg, kb)) {
            g_accent_color = ImVec4(kr, kg, kb, 1.0f); // Follow KDE
        } else {
            g_accent_color = ImVec4(0.24f, 0.49f, 0.78f, 1.0f); // Fallback
        }
    }

    // Calculate variants (lighter/darker)
    g_accent_hover = ImVec4(
        g_accent_color.x + (1.0f - g_accent_color.x) * 0.15f,
        g_accent_color.y + (1.0f - g_accent_color.y) * 0.15f,
        g_accent_color.z + (1.0f - g_accent_color.z) * 0.15f,
        1.0f
    );
    g_accent_active = ImVec4(
        g_accent_color.x * 0.75f,
        g_accent_color.y * 0.75f,
        g_accent_color.z * 0.75f,
        1.0f
    );

    ImVec4* colors = style.Colors;

    // Base colors (dark theme)
    colors[ImGuiCol_Text]                   = ImVec4(0.90f, 0.95f, 1.00f, 1.00f);
    colors[ImGuiCol_TextDisabled]           = ImVec4(0.50f, 0.55f, 0.60f, 1.00f);
    colors[ImGuiCol_WindowBg]               = ImVec4(0.06f, 0.11f, 0.18f, 0.94f);
    colors[ImGuiCol_ChildBg]                = ImVec4(0.08f, 0.14f, 0.22f, 0.50f);
    colors[ImGuiCol_PopupBg]                = ImVec4(0.08f, 0.14f, 0.22f, 0.98f);
    colors[ImGuiCol_Border]                 = ImVec4(0.20f, 0.28f, 0.38f, 0.50f);
    colors[ImGuiCol_BorderShadow]           = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_FrameBg]                = ImVec4(0.10f, 0.16f, 0.24f, 0.60f);
    colors[ImGuiCol_FrameBgHovered]         = ImVec4(0.15f, 0.22f, 0.32f, 0.70f);
    colors[ImGuiCol_FrameBgActive]          = ImVec4(0.18f, 0.26f, 0.38f, 0.80f);
    colors[ImGuiCol_TitleBg]                = ImVec4(0.08f, 0.14f, 0.22f, 1.00f);
    colors[ImGuiCol_TitleBgActive]          = g_accent_active;
    colors[ImGuiCol_TitleBgCollapsed]       = ImVec4(0.08f, 0.14f, 0.22f, 1.00f);
    colors[ImGuiCol_MenuBarBg]              = ImVec4(0.08f, 0.14f, 0.22f, 0.90f);
    colors[ImGuiCol_ScrollbarBg]            = ImVec4(0.05f, 0.09f, 0.15f, 0.50f);
    colors[ImGuiCol_ScrollbarGrab]          = g_accent_active;
    colors[ImGuiCol_ScrollbarGrabHovered]   = g_accent_color;
    colors[ImGuiCol_ScrollbarGrabActive]    = g_accent_hover;
    colors[ImGuiCol_CheckMark]              = g_accent_hover;
    colors[ImGuiCol_SliderGrab]             = g_accent_color;
    colors[ImGuiCol_SliderGrabActive]       = g_accent_hover;
    colors[ImGuiCol_Button]                 = g_accent_active;
    colors[ImGuiCol_ButtonHovered]          = g_accent_color;
    colors[ImGuiCol_ButtonActive]           = g_accent_hover;
    colors[ImGuiCol_Header]                 = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
    colors[ImGuiCol_HeaderHovered]          = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
    colors[ImGuiCol_HeaderActive]           = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
    colors[ImGuiCol_Separator]              = ImVec4(0.20f, 0.28f, 0.38f, 0.60f);
    colors[ImGuiCol_SeparatorHovered]       = g_accent_color;
    colors[ImGuiCol_SeparatorActive]        = g_accent_hover;
    colors[ImGuiCol_ResizeGrip]             = ImVec4(g_accent_color.x, g_accent_color.y, g_accent_color.z, 0.30f);
    colors[ImGuiCol_ResizeGripHovered]      = g_accent_color;
    colors[ImGuiCol_ResizeGripActive]       = g_accent_hover;
    colors[ImGuiCol_Tab]                    = ImVec4(0.10f, 0.16f, 0.24f, 0.90f);
    colors[ImGuiCol_TabHovered]             = g_accent_color;
    colors[ImGuiCol_TabActive]              = g_accent_active;
    colors[ImGuiCol_TabUnfocused]           = ImVec4(0.08f, 0.14f, 0.22f, 0.90f);
    colors[ImGuiCol_TabUnfocusedActive]     = g_accent_active;
    colors[ImGuiCol_PlotLines]              = g_accent_color;
    colors[ImGuiCol_PlotLinesHovered]       = g_accent_hover;
    colors[ImGuiCol_PlotHistogram]          = g_accent_color;
    colors[ImGuiCol_PlotHistogramHovered]   = g_accent_hover;
    colors[ImGuiCol_TextSelectedBg]         = ImVec4(g_accent_color.x, g_accent_color.y, g_accent_color.z, 0.35f);
    colors[ImGuiCol_DragDropTarget]         = g_accent_hover;
    colors[ImGuiCol_NavHighlight]           = g_accent_color;
    colors[ImGuiCol_NavWindowingHighlight]  = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
    colors[ImGuiCol_NavWindowingDimBg]      = ImVec4(0.00f, 0.00f, 0.00f, 0.20f);
    colors[ImGuiCol_ModalWindowDimBg]       = ImVec4(0.00f, 0.00f, 0.00f, 0.35f);

    // Oxygen-style rounding and sizing
    style.WindowPadding         = ImVec2(12, 10);
    style.FramePadding          = ImVec2(8, 4);
    style.ItemSpacing           = ImVec2(8, 6);
    style.ItemInnerSpacing      = ImVec2(6, 4);
    style.TouchExtraPadding     = ImVec2(0, 0);
    style.IndentSpacing         = 20.0f;
    style.ScrollbarSize         = 14.0f;
    style.GrabMinSize           = 12.0f;

    style.WindowBorderSize      = 0.0f;
    style.ChildBorderSize       = 0.0f;
    style.PopupBorderSize       = 1.0f;
    style.FrameBorderSize       = 0.0f;
    style.TabBorderSize         = 0.0f;

    style.WindowRounding        = 8.0f;
    style.ChildRounding         = 6.0f;
    style.FrameRounding         = 6.0f;
    style.PopupRounding         = 6.0f;
    style.ScrollbarRounding     = 8.0f;
    style.GrabRounding          = 4.0f;
    style.TabRounding           = 6.0f;

    style.WindowTitleAlign      = ImVec2(0.5f, 0.5f);
    style.WindowMenuButtonPosition = ImGuiDir_None;
    style.ColorButtonPosition   = ImGuiDir_Right;
}

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
        case BTN_LEFT: return "Mouse L"; case BTN_RIGHT: return "Mouse R";
        case BTN_MIDDLE: return "Mouse M"; case BTN_SIDE: return "Mouse Side";
        case BTN_EXTRA: return "Mouse Extra";
        case KEY_KP0: return "KP 0"; case KEY_KP1: return "KP 1"; case KEY_KP2: return "KP 2";
        case KEY_KP3: return "KP 3"; case KEY_KP4: return "KP 4"; case KEY_KP5: return "KP 5";
        case KEY_KP6: return "KP 6"; case KEY_KP7: return "KP 7"; case KEY_KP8: return "KP 8";
        case KEY_KP9: return "KP 9"; case KEY_KPDOT: return "KP .";
        case KEY_KPENTER: return "KP Enter"; case KEY_KPPLUS: return "KP +";
        case KEY_KPMINUS: return "KP -"; case KEY_KPASTERISK: return "KP *";
        case KEY_KPSLASH: return "KP /";
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
        case GLFW_KEY_KP_ENTER: return KEY_KPENTER;
        case GLFW_KEY_KP_DECIMAL: return KEY_KPDOT;
        case GLFW_KEY_KP_DIVIDE: return KEY_KPSLASH;
        case GLFW_KEY_KP_MULTIPLY: return KEY_KPASTERISK;
        case GLFW_KEY_KP_SUBTRACT: return KEY_KPMINUS;
        case GLFW_KEY_KP_ADD: return KEY_KPPLUS;
        case GLFW_KEY_KP_EQUAL: return KEY_KPEQUAL;
        case GLFW_KEY_KP_0: return KEY_KP0;
        case GLFW_KEY_KP_1: return KEY_KP1;
        case GLFW_KEY_KP_2: return KEY_KP2;
        case GLFW_KEY_KP_3: return KEY_KP3;
        case GLFW_KEY_KP_4: return KEY_KP4;
        case GLFW_KEY_KP_5: return KEY_KP5;
        case GLFW_KEY_KP_6: return KEY_KP6;
        case GLFW_KEY_KP_7: return KEY_KP7;
        case GLFW_KEY_KP_8: return KEY_KP8;
        case GLFW_KEY_KP_9: return KEY_KP9;
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

static void mouse_button_callback(GLFWwindow*, int button, int action, int) {
    if (!g_binding_target || action != GLFW_PRESS) return;
    int evdev = -1;
    switch (button) {
        case GLFW_MOUSE_BUTTON_LEFT: evdev = BTN_LEFT; break;
        case GLFW_MOUSE_BUTTON_RIGHT: evdev = BTN_RIGHT; break;
        case GLFW_MOUSE_BUTTON_MIDDLE: evdev = BTN_MIDDLE; break;
        case GLFW_MOUSE_BUTTON_4: evdev = BTN_SIDE; break;
        case GLFW_MOUSE_BUTTON_5: evdev = BTN_EXTRA; break;
        default:
            if (button >= 6) evdev = BTN_FORWARD + (button - 6);
            break;
    }
    if (evdev >= 0) {
        *g_binding_target = evdev;
    }
    g_binding_target = nullptr;
}

static const float LABEL_W = 175.0f;

static bool oxygen_button(const char* label, ImVec2 size_arg = ImVec2(0, 0)) {
    const float R = 7.0f;
    ImVec2 ls = ImGui::CalcTextSize(label, nullptr, true);
    ImVec2 sz = size_arg;
    if (sz.x <= 0) sz.x = ls.x + ImGui::GetStyle().FramePadding.x * 2.0f;
    if (sz.y <= 0) sz.y = ImGui::GetFrameHeight();
    ImVec2 mn = ImGui::GetCursorScreenPos();
    ImVec2 mx(mn.x + sz.x, mn.y + sz.y);

    bool clicked = ImGui::InvisibleButton(label, sz);
    bool hov = ImGui::IsItemHovered();
    bool act = ImGui::IsItemActive();
    ImDrawList* dl = ImGui::GetWindowDrawList();

    ImVec4 btn_color = act ? g_accent_hover : (hov ? g_accent_color : g_accent_active);
    ImVec4 btn_dark = ImVec4(btn_color.x * 0.5f, btn_color.y * 0.5f, btn_color.z * 0.5f, 1.0f);
    ImVec4 btn_border = ImVec4(
        btn_color.x * 1.4f > 1.0f ? 1.0f : btn_color.x * 1.4f,
        btn_color.y * 1.4f > 1.0f ? 1.0f : btn_color.y * 1.4f,
        btn_color.z * 1.4f > 1.0f ? 1.0f : btn_color.z * 1.4f, 0.6f
    );

    ImU32 bot = IM_COL32((int)(btn_dark.x * 255), (int)(btn_dark.y * 255), (int)(btn_dark.z * 255), 255);

    // Base fill with rounding
    dl->AddRectFilled(mn, mx, bot, R);

    // Highlight inset MORE to hide sharp corners inside the rounded base
    float inset = 3.0f;
    float highlight_h = (sz.y * 0.45f);
    ImVec2 hl_tl(mn.x + inset, mn.y + inset);
    ImVec2 hl_br(mx.x - inset, mn.y + highlight_h);

    dl->AddRectFilledMultiColor(hl_tl, hl_br,
                                IM_COL32(255,255,255,100),
                                IM_COL32(255,255,255,100),
                                IM_COL32(255,255,255,5),
                                IM_COL32(255,255,255,5));

    // Border with rounding
    dl->AddRect(mn, mx, IM_COL32((int)(btn_border.x*255), (int)(btn_border.y*255), (int)(btn_border.z*255), 150), R, ImDrawFlags_None, 1.0f);

    const char* tend = strstr(label, "##");
    dl->AddText(ImVec2(mn.x + (sz.x - ls.x) * 0.5f, mn.y + (sz.y - ls.y) * 0.5f),
                IM_COL32(238, 246, 255, 255), label, tend ? tend : nullptr);
    return clicked;
}

static void keybind_button(const char* label, int& key_code) {
    ImGui::AlignTextToFramePadding();
    char buf[64];
    if (g_binding_target == &key_code)      snprintf(buf, sizeof(buf), "Press a key...##%s", label);
    else if (key_code == 0)                 snprintf(buf, sizeof(buf), "Click to bind##%s", label);
    else                                    snprintf(buf, sizeof(buf), "%s##%s", key_name(key_code), label);
    if (oxygen_button(buf, ImVec2(150, 0))) g_binding_target = &key_code;
    ImGui::SameLine();
    ImGui::Text("%s", label);
}

static void keybind_row(const char* label, int& key_code) {
    ImGui::AlignTextToFramePadding();
    ImGui::TextUnformatted(label);
    ImGui::SameLine(LABEL_W);
    char buf[64];
    if (g_binding_target == &key_code)      snprintf(buf, sizeof(buf), "Press a key...##%s", label);
    else if (key_code == 0)                 snprintf(buf, sizeof(buf), "Click to bind##%s", label);
    else                                    snprintf(buf, sizeof(buf), "%s##%s", key_name(key_code), label);
    if (oxygen_button(buf, ImVec2(140, 0))) g_binding_target = &key_code;
}

// Clean slider with dynamic accent thumb (no double overlays)
static bool row_slider(const char* label, double* v, double lo, double hi, const char* fmt) {
    ImGui::AlignTextToFramePadding();
    ImGui::TextUnformatted(label);
    ImGui::SameLine(LABEL_W);
    ImGui::SetNextItemWidth(-FLT_MIN);
    char id[96]; snprintf(id, sizeof(id), "##%s", label);
    bool changed = ImGui::SliderScalar(id, ImGuiDataType_Double, v, &lo, &hi, fmt);

    // Get slider rect
    ImVec2 r0 = ImGui::GetItemRectMin(), r1 = ImGui::GetItemRectMax();
    float grab_w = ImGui::GetStyle().GrabMinSize;
    float t = (float)((*v - lo) / (hi - lo));
    if (t < 0) t = 0; if (t > 1) t = 1;
    float gx = r0.x + (r1.x - r0.x - grab_w) * t;
    ImVec2 g0(gx, r0.y + 3), g1(gx + grab_w, r1.y - 3);

    bool hov = ImGui::IsItemHovered();
    bool act = ImGui::IsItemActive();
    ImDrawList* dl = ImGui::GetWindowDrawList();

    // Choose color based on state
    ImVec4 thumb_color = act ? g_accent_hover : (hov ? g_accent_color : g_accent_active);
    float gr = grab_w * 0.5f;

    // Draw thumb background (darker base)
    dl->AddRectFilled(g0, g1, IM_COL32(
        (int)(thumb_color.x * 0.6f * 255),
        (int)(thumb_color.y * 0.6f * 255),
        (int)(thumb_color.z * 0.6f * 255),
        255
    ), gr);

    // Draw thumb highlight (top gradient)
    float mid_y = g0.y + (g1.y - g0.y) * 0.45f;
    dl->AddRectFilledMultiColor(
        ImVec2(g0.x + 2, g0.y + 2),
        ImVec2(g1.x - 2, mid_y),
        IM_COL32(255, 255, 255, 100),
        IM_COL32(255, 255, 255, 100),
        IM_COL32(255, 255, 255, 20),
        IM_COL32(255, 255, 255, 20)
    );

    // Draw thumb border
    dl->AddRect(
        g0, g1,
        IM_COL32(
            (int)(thumb_color.x * 0.9f * 255),
            (int)(thumb_color.y * 0.9f * 255),
            (int)(thumb_color.z * 0.9f * 255),
            180
        ),
        gr,
        ImDrawFlags_None,
        1.0f
    );

    return changed;
}

static bool combo_framed(const char* id, int* idx, const char* items) {
    ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 0.0f);
    ImGui::PushStyleColor(ImGuiCol_Button,        ImGui::GetStyleColorVec4(ImGuiCol_FrameBg));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImGui::GetStyleColorVec4(ImGuiCol_FrameBgHovered));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive,  ImGui::GetStyleColorVec4(ImGuiCol_FrameBgActive));
    bool changed = ImGui::Combo(id, idx, items);
    ImGui::PopStyleColor(3);
    ImGui::PopStyleVar();
    ImGui::GetWindowDrawList()->AddRect(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(),
                                        ImGui::GetColorU32(ImGuiCol_Border), 1.0f,
                                        ImGui::GetStyle().FrameRounding);
    return changed;
}

static bool row_combo(const char* label, int* idx, const char* items) {
    ImGui::AlignTextToFramePadding();
    ImGui::TextUnformatted(label);
    ImGui::SameLine(LABEL_W);
    ImGui::SetNextItemWidth(-FLT_MIN);
    char id[96]; snprintf(id, sizeof(id), "##%s", label);
    return combo_framed(id, idx, items);
}

static void mode_combo(const char* label, ActivationMode& mode) {
    int idx = mode == ActivationMode::TOGGLE ? 0 : (mode == ActivationMode::HOLD ? 1 : 2);
    if (combo_framed(label, &idx, "Toggle\0Hold\0Mouse\0")) {
        mode = idx == 0 ? ActivationMode::TOGGLE : (idx == 1 ? ActivationMode::HOLD : ActivationMode::MOUSE);
    }
}

static bool row_check(const char* label, bool* v) {
    ImGui::AlignTextToFramePadding();
    ImGui::TextUnformatted(label);
    ImGui::SameLine(LABEL_W);
    char id[96]; snprintf(id, sizeof(id), "##%s", label);
    return ImGui::Checkbox(id, v);
}

static bool row_section(const char* title) {
    ImGui::Spacing();
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.72f, 0.60f, 0.95f, 1.0f));
    bool open = ImGui::CollapsingHeader(title, ImGuiTreeNodeFlags_DefaultOpen);
    ImGui::PopStyleColor();
    return open;
}

// Dynamic accent section header
static bool oxygen_section(const char* label, bool default_open) {
    const float R = 7.0f;
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(10, 6));
    ImDrawList* dl = ImGui::GetWindowDrawList();
    ImVec2 mn = ImGui::GetCursorScreenPos();
    float w = ImGui::GetContentRegionAvail().x;
    float h = ImGui::GetFrameHeight();
    ImVec2 mx(mn.x + w, mn.y + h);

    ImVec4 sec_color = g_accent_active;
    ImVec4 sec_light = ImVec4(
        sec_color.x * 1.3f > 1.0f ? 1.0f : sec_color.x * 1.3f,
        sec_color.y * 1.3f > 1.0f ? 1.0f : sec_color.y * 1.3f,
        sec_color.z * 1.3f > 1.0f ? 1.0f : sec_color.z * 1.3f, 1.0f
    );

    // Base fill with rounding
    dl->AddRectFilled(mn, mx, IM_COL32((int)(sec_color.x*255), (int)(sec_color.y*255), (int)(sec_color.z*255), 255), R);

    // Highlight inset to hide sharp corners
    float inset = 3.0f;
    float highlight_h = h * 0.50f;
    ImVec2 hl_tl(mn.x + inset, mn.y + inset);
    ImVec2 hl_br(mx.x - inset, mn.y + highlight_h);

    dl->AddRectFilledMultiColor(hl_tl, hl_br,
                                IM_COL32((int)(sec_light.x*255), (int)(sec_light.y*255), (int)(sec_light.z*255), 130),
                                IM_COL32((int)(sec_light.x*255), (int)(sec_light.y*255), (int)(sec_light.z*255), 130),
                                IM_COL32(0,0,0,0),
                                IM_COL32(0,0,0,0));

    // Border with rounding
    dl->AddRect(mn, mx, IM_COL32((int)(sec_light.x*255), (int)(sec_light.y*255), (int)(sec_light.z*255), 180), R, ImDrawFlags_None, 1.0f);

    ImGui::SetCursorScreenPos(mn);
    bool open = ImGui::CollapsingHeader(label, default_open ? ImGuiTreeNodeFlags_DefaultOpen : 0);
    ImGui::PopStyleVar();
    ImGui::Dummy(ImVec2(0, 3));
    return open;
}

static void status_chip(const char* label, bool on) {
    ImVec2 pad(9, 4);
    ImVec2 ls = ImGui::CalcTextSize(label);
    ImVec2 mn = ImGui::GetCursorScreenPos();
    ImVec2 sz(ls.x + pad.x * 2, ls.y + pad.y * 2);
    ImVec2 mx(mn.x + sz.x, mn.y + sz.y);
    float r = sz.y * 0.5f;
    ImDrawList* dl = ImGui::GetWindowDrawList();

    if (on) {
        ImVec4 c = g_accent_color;
        ImVec4 dark = ImVec4(c.x * 0.5f, c.y * 0.5f, c.z * 0.5f, 1.0f);
        ImVec4 light = ImVec4(
            c.x * 1.3f > 1.0f ? 1.0f : c.x * 1.3f,
            c.y * 1.3f > 1.0f ? 1.0f : c.y * 1.3f,
            c.z * 1.3f > 1.0f ? 1.0f : c.z * 1.3f, 1.0f
        );

        dl->AddRectFilled(mn, mx, IM_COL32((int)(dark.x*255), (int)(dark.y*255), (int)(dark.z*255), 255), r);

        // Highlight inset to hide sharp corners
        float inset = 3.0f;
        ImVec2 hl_tl(mn.x + inset, mn.y + inset);
        ImVec2 hl_br(mx.x - inset, mn.y + sz.y * 0.5f);

        dl->AddRectFilledMultiColor(hl_tl, hl_br,
                                    IM_COL32((int)(light.x*255), (int)(light.y*255), (int)(light.z*255), 180),
                                    IM_COL32((int)(light.x*255), (int)(light.y*255), (int)(light.z*255), 180),
                                    IM_COL32(0,0,0,0),
                                    IM_COL32(0,0,0,0));

        dl->AddRect(mn, mx, IM_COL32((int)(light.x*255), (int)(light.y*255), (int)(light.z*255), 200), r, ImDrawFlags_None, 1.0f);
        dl->AddText(ImVec2(mn.x + pad.x, mn.y + pad.y), IM_COL32(232, 255, 242, 255), label);
    } else {
        dl->AddRectFilled(mn, mx, IM_COL32(40, 50, 60, 200), r);
        dl->AddRect(mn, mx, IM_COL32(100, 120, 140, 100), r, ImDrawFlags_None, 1.0f);
        dl->AddText(ImVec2(mn.x + pad.x, mn.y + pad.y), IM_COL32(180, 190, 200, 200), label);
    }
    ImGui::Dummy(sz);
}

// Segmented control: rounded track + inset rounded pill for selection.
static int g_tab = 0;
static void segmented_tabs() {
    static const char* names[] = { "Left", "Right", "General", "Macros", "Config", "Analytics" };
    const int count = 6;
    const float R = 7.0f;
    const float pad_x = 14.0f;
    const float h = ImGui::GetFrameHeight();

    ImVec2 ls[count]; float w[count], total = 0.0f;
    for (int i = 0; i < count; i++) {
        ls[i] = ImGui::CalcTextSize(names[i]);
        w[i] = ls[i].x + pad_x * 2.0f;
        total += w[i];
    }

    ImVec2 mn = ImGui::GetCursorScreenPos();
    ImVec2 mx(mn.x + total, mn.y + h);
    ImDrawList* dl = ImGui::GetWindowDrawList();

    dl->AddRectFilled(mn, mx, IM_COL32(22, 36, 56, 210), R);
    dl->AddRect(mn, mx, IM_COL32(150, 200, 250, 60), 1.0f, R);

    float x = mn.x;
    for (int i = 0; i < count; i++) {
        ImGui::SetCursorScreenPos(ImVec2(x, mn.y));
        bool sel = (g_tab == i);
        ImVec2 s0(x, mn.y), s1(x + w[i], mn.y + h);
        bool clicked = ImGui::InvisibleButton(names[i], ImVec2(w[i], h));
        bool hov = ImGui::IsItemHovered();

        ImVec2 i0(s0.x + 2, s0.y + 2), i1(s1.x - 2, s1.y - 2);
        if (sel) {
            dl->AddRectFilled(i0, i1, IM_COL32((int)(g_accent_color.x*255), (int)(g_accent_color.y*255), (int)(g_accent_color.z*255), 255), R - 2);
            dl->AddRectFilledMultiColor(ImVec2(i0.x + 3, i0.y + 2), ImVec2(i1.x - 3, i0.y + (i1.y - i0.y) * 0.58f),
                                        IM_COL32(255,255,255,80), IM_COL32(255,255,255,80), IM_COL32(255,255,255,0), IM_COL32(255,255,255,0));
        } else if (hov) {
            dl->AddRectFilled(i0, i1, IM_COL32(255, 255, 255, 18), R - 2);
        }
        if (i > 0 && !sel && g_tab != i - 1)
            dl->AddLine(ImVec2(s0.x, s0.y + 5), ImVec2(s0.x, s1.y - 5), IM_COL32(255, 255, 255, 26), 1.0f);

        dl->AddText(ImVec2(s0.x + (w[i] - ls[i].x) * 0.5f, s0.y + (h - ls[i].y) * 0.5f),
                    sel ? IM_COL32(242, 249, 255, 255) : IM_COL32(202, 218, 238, 215), names[i]);
        if (clicked) g_tab = i;
        x += w[i];
    }
    ImGui::SetCursorScreenPos(ImVec2(mn.x, mx.y));
    ImGui::Spacing();
}

static void draw_clicker_tab(ClickerProfile& profile, ActivationMode& mode,
                             Clicker& clicker, int& key,
                             const ClickerPreset* presets) {
    if (clicker.is_enabled())
        ImGui::TextColored(ImVec4(0.55f, 0.90f, 0.65f, 1.0f),
                           "ACTIVE  -  %.1f avg CPS  -  %u clicks",
                           clicker.tracker().avg_cps(), clicker.click_count());
    else
        ImGui::TextDisabled("Idle  -  press your toggle key to start");
    ImGui::Spacing();

    if (oxygen_section("Quick Setup", true)) {
        keybind_row("Toggle Key", key);

        ImGui::TextUnformatted("Preset");
        const float sp = ImGui::GetStyle().ItemSpacing.x;
        const float row_x = ImGui::GetCursorPosX();
        const float content_w = ImGui::GetContentRegionAvail().x;
        const float btn_h = ImGui::GetFrameHeight();
        float line_y = ImGui::GetCursorPosY();
        float x_used = 0.0f;
        for (int i = 0; i < PRESET_COUNT; i++) {
            ImVec2 pls = ImGui::CalcTextSize(presets[i].name, nullptr, true);
            float bw = pls.x + ImGui::GetStyle().FramePadding.x * 2.0f;
            if (i > 0) {
                if (x_used + sp + bw > content_w) {
                    line_y += btn_h + 4.0f;
                    ImGui::SetCursorPos(ImVec2(row_x, line_y));
                    x_used = 0.0f;
                } else {
                    ImGui::SameLine(); x_used += sp;
                }
            }
            if (oxygen_button(presets[i].name)) {
                int saved_button = profile.mouse_button;
                profile = presets[i].profile;
                profile.mouse_button = saved_button;
            }
            x_used += bw;
        }
        ImGui::SetCursorPosY(line_y + btn_h + ImGui::GetStyle().ItemSpacing.y);

        int idx = mode == ActivationMode::TOGGLE ? 0 : (mode == ActivationMode::HOLD ? 1 : 2);
        if (row_combo("Activation", &idx, "Toggle\0Hold\0Mouse\0"))
            mode = idx == 0 ? ActivationMode::TOGGLE : (idx == 1 ? ActivationMode::HOLD : ActivationMode::MOUSE);

        row_slider("Speed (CPS)", &profile.cps_mean, 1.0, 25.0, "%.1f");
    }

    if (oxygen_section("Advanced Tuning", false)) {
        row_slider("CPS Variation", &profile.cps_stddev, 0.0, 5.0, "%.2f");
        row_slider("Min CPS", &profile.cps_min, 1.0, 20.0, "%.1f");
        row_slider("Max CPS", &profile.cps_max, 5.0, 30.0, "%.1f");
        int dist_idx = (int)profile.distribution;
        if (row_combo("Distribution", &dist_idx, "Gaussian\0Poisson\0Uniform\0"))
            profile.distribution = (DistributionType)dist_idx;

        ImGui::Dummy(ImVec2(0, 4)); ImGui::TextDisabled("HUMANIZATION");
        row_slider("Spike Chance", &profile.spike_chance, 0.0, 0.2, "%.3f");
        if (profile.spike_chance > 0) {
            row_slider("Spike Min", &profile.spike_min, 0.0, 5.0, "%.1f");
            row_slider("Spike Max", &profile.spike_max, 0.0, 8.0, "%.1f");
        }
        row_slider("Stutter Chance", &profile.stutter_chance, 0.0, 0.2, "%.3f");
        if (profile.stutter_chance > 0) {
            row_slider("Stutter Min", &profile.stutter_min, 0.0, 8.0, "%.1f");
            row_slider("Stutter Max", &profile.stutter_max, 0.0, 10.0, "%.1f");
        }

        ImGui::Dummy(ImVec2(0, 4)); ImGui::TextDisabled("HOLD TIMING");
        row_slider("Hold Mean (ms)",   &profile.hold_ms_mean,   10.0, 100.0, "%.1f");
        row_slider("Hold StdDev (ms)", &profile.hold_ms_stddev,  0.0,  20.0, "%.1f");
        row_slider("Hold Min (ms)",    &profile.hold_ms_min,     5.0,  50.0, "%.1f");
        row_slider("Hold Max (ms)",    &profile.hold_ms_max,    20.0, 150.0, "%.1f");
        row_slider("Heavy Hold Chance", &profile.hold_ms_heavy_chance, 0.0, 0.1, "%.3f");
        if (profile.hold_ms_heavy_chance > 0) {
            row_slider("Heavy Hold Min", &profile.hold_ms_heavy_min, 5.0, 50.0, "%.1f");
            row_slider("Heavy Hold Max", &profile.hold_ms_heavy_max, 10.0, 80.0, "%.1f");
        }
        row_slider("Rhythm Volatility", &profile.rhythm_volatility, 0.0, 2.0, "%.2f");
        row_slider("Rhythm Tension",    &profile.rhythm_tension,    0.0, 0.2, "%.3f");
        row_slider("CPS Fallout",       &profile.cps_minmax_fallout,0.0, 3.0, "%.2f");
        row_slider("Start Delay (ms)",  &profile.start_delay_ms,    0.0, 500.0, "%.0f");

        ImGui::Dummy(ImVec2(0, 4)); ImGui::TextDisabled("DOUBLE CLICK");
        row_slider("Chance", &profile.double_click_chance, 0.0, 0.3, "%.3f");
        if (profile.double_click_chance > 0) {
            row_slider("Delay Min (ms)", &profile.double_click_delay_min,  5.0,  80.0, "%.0f");
            row_slider("Delay Max (ms)", &profile.double_click_delay_max, 10.0, 120.0, "%.0f");
        }

        ImGui::Dummy(ImVec2(0, 4)); ImGui::TextDisabled("FATIGUE BREAKS");
        row_check("Enable Breaks", &profile.breaks_enabled);
        if (profile.breaks_enabled) {
            row_slider("Break Every Min (s)",    &profile.break_interval_min,  5.0, 120.0, "%.1f");
            row_slider("Break Every Max (s)",    &profile.break_interval_max,  5.0, 120.0, "%.1f");
            row_slider("Break Duration Min (s)", &profile.break_duration_min,  0.1,   5.0, "%.1f");
            row_slider("Break Duration Max (s)", &profile.break_duration_max,  0.1,   5.0, "%.1f");
        }
    }
}

static void setup_kde_cursor() {
    const char* home = getenv("HOME");
    if (!home) return;
    std::string path = std::string(home) + "/.config/kcminputrc";
    FILE* f = fopen(path.c_str(), "r");
    if (!f) return;
    char line[256];
    bool in_mouse = false;
    char theme[128] = "";
    char size[32] = "";
    while (fgets(line, sizeof(line), f)) {
        if (strncmp(line, "[Mouse]", 7) == 0) {
            in_mouse = true;
            continue;
        }
        if (line[0] == '[') {
            in_mouse = false;
            continue;
        }
        if (in_mouse) {
            char* eq = strchr(line, '=');
            if (eq) {
                *eq = '\0';
                char* key = line;
                char* val = eq + 1;
                while (*key == ' ' || *key == '\t') key++;
                char* end = key + strlen(key) - 1;
                while (end > key && (*end == ' ' || *end == '\t' || *end == '\n' || *end == '\r')) *end-- = '\0';
                while (*val == ' ' || *val == '\t') val++;
                end = val + strlen(val) - 1;
                while (end > val && (*end == ' ' || *end == '\t' || *end == '\n' || *end == '\r')) *end-- = '\0';
                if (strcmp(key, "cursorTheme") == 0) strncpy(theme, val, sizeof(theme)-1);
                else if (strcmp(key, "cursorSize") == 0) strncpy(size, val, sizeof(size)-1);
            }
        }
    }
    fclose(f);
    if (theme[0]) setenv("XCURSOR_THEME", theme, 1);
    if (size[0]) setenv("XCURSOR_SIZE", size, 1);
}

void gui_run(GuiState& state) {
    setup_kde_cursor();
    if (!glfwInit()) {
        fprintf(stderr, "Failed to initialize GLFW\n");
        return;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
    glfwWindowHint(GLFW_DECORATED, GLFW_FALSE);

    GLFWwindow* window = glfwCreateWindow(700, 600, "Ghast Clicker", nullptr, nullptr);
    if (!window) {
        fprintf(stderr, "Failed to create GLFW window\n");
        glfwTerminate();
        return;
    }
    g_window = window;
    glfwSetWindowSizeLimits(window, 520, 420, GLFW_DONT_CARE, GLFW_DONT_CARE);

    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);
    glfwSetKeyCallback(window, key_callback);
    glfwSetMouseButtonCallback(window, mouse_button_callback);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.IniFilename = nullptr;

    // Real font instead of the pixelated embedded one
    {
        const char* candidates[] = {
            "/usr/share/fonts/inter/Inter-Regular.otf",
            "/usr/share/fonts/truetype/inter/Inter-Regular.otf",
            "/usr/share/fonts/roboto/Roboto-Regular.ttf",
            "/usr/share/fonts/truetype/roboto/Roboto-Regular.ttf",
            "/usr/share/fonts/noto/NotoSans-Regular.ttf",
            "/usr/share/fonts/truetype/noto/NotoSans-Regular.ttf",
            "/usr/share/fonts/truetype/ubuntu/Ubuntu-R.ttf",
            "/usr/share/fonts/cantarell/Cantarell-VF.otf",
            "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
        };
        ImFont* font = nullptr;
        for (const char* p : candidates) {
            if (FILE* f = fopen(p, "rb")) { fclose(f); font = io.Fonts->AddFontFromFileTTF(p, 16.0f); break; }
        }
        if (!font) io.Fonts->AddFontDefault();
    }

    // Setup Oxygen-style theme (replaces manual StyleColorsDark and color assignments)
    setup_oxygen_theme(state.config.accent_preset, state.config.accent_r, state.config.accent_g, state.config.accent_b);

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 130");

    // Config profile state
    static char profile_name_buf[64] = "";
    int selected_profile = -1;
    auto profiles = ConfigFile::list_profiles();
    char status_msg[128] = "";
    float status_timer = 0;

    bool always_on_top = false;
    bool dragging = false;
    double drag_start_x = 0, drag_start_y = 0;
    int win_start_x = 0, win_start_y = 0;

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        // Window dragging via title bar
        if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
            double mx, my;
            glfwGetCursorPos(window, &mx, &my);
            if (!dragging && my < 30.0 && !ImGui::IsAnyItemActive()) {
                dragging = true;
                drag_start_x = mx;
                drag_start_y = my;
                glfwGetWindowPos(window, &win_start_x, &win_start_y);
            }
            if (dragging) {
                int dx = (int)(mx - drag_start_x);
                int dy = (int)(my - drag_start_y);
                glfwSetWindowPos(window, win_start_x + dx, win_start_y + dy);
            }
        } else {
            dragging = false;
        }

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);

        ImGui::SetNextWindowPos(ImVec2(0, 0));
        ImGui::SetNextWindowSize(ImVec2((float)display_w, (float)display_h));

        ImGuiWindowFlags wf = ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
                              ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar |
                              ImGuiWindowFlags_NoBackground; // Ensures our custom background shows through rounded corners

        if (ImGui::Begin("Ghast Clicker", nullptr, wf)) {
            // Draw theme-aware gradient background INSIDE the window to perfectly match rounded corners
            ImDrawList* draw_list = ImGui::GetWindowDrawList();
            ImVec2 w_min = ImVec2(0, 0);
            ImVec2 w_max = ImVec2((float)display_w, (float)display_h);

            // Use a nicer gradient based on the active theme (more visible than before)
            ImVec4 bg_top = ImVec4(g_accent_color.x * 0.25f, g_accent_color.y * 0.25f, g_accent_color.z * 0.25f, 1.0f);
            ImVec4 bg_bot = ImVec4(g_accent_color.x * 0.08f, g_accent_color.y * 0.08f, g_accent_color.z * 0.08f, 1.0f);

            draw_list->AddRectFilledMultiColor(w_min, w_max,
                IM_COL32((int)(bg_top.x*255), (int)(bg_top.y*255), (int)(bg_top.z*255), 255),
                IM_COL32((int)(bg_top.x*255), (int)(bg_top.y*255), (int)(bg_top.z*255), 255),
                IM_COL32((int)(bg_bot.x*255), (int)(bg_bot.y*255), (int)(bg_bot.z*255), 255),
                IM_COL32((int)(bg_bot.x*255), (int)(bg_bot.y*255), (int)(bg_bot.z*255), 255));

            bool l_on = state.left.is_enabled() || state.left_armed;
            bool r_on = state.right.is_enabled() || state.right_armed;
            status_chip(l_on ? "L: ON" : "L: off", l_on);
            ImGui::SameLine(0, 6);
            status_chip(r_on ? "R: ON" : "R: off", r_on);
            ImGui::SameLine();
            ImGui::SetCursorPosX(ImGui::GetCursorPosX() + ImGui::GetContentRegionAvail().x - 70.0f);
            if (oxygen_button(always_on_top ? "pinned" : "pin", ImVec2(70, 0))) {
                always_on_top = !always_on_top;
                glfwSetWindowAttrib(window, GLFW_FLOATING, always_on_top ? GLFW_TRUE : GLFW_FALSE);
            }
            ImGui::Spacing();
            segmented_tabs();
            ImGui::Separator();

            switch (g_tab) {
                case 0:
                    draw_clicker_tab(state.config.left, state.config.left_mode,
                                     state.left, state.config.key_left, LEFT_PRESETS);
                    break;

                case 1:
                    draw_clicker_tab(state.config.right, state.config.right_mode,
                                     state.right, state.config.key_right, RIGHT_PRESETS);
                    break;

                case 2: {
                    ImGui::Spacing();

                    ImGui::Text("Keybinds");
                    ImGui::Separator();
                    ImGui::BeginDisabled(!state.config.inventory_key_enabled);
                    keybind_button("Inventory Key", state.config.key_inventory);
                    ImGui::EndDisabled();
                    ImGui::SameLine();
                    ImGui::Checkbox("Enabled##inv", &state.config.inventory_key_enabled);
                    ImGui::TextDisabled("Pauses clicking while inventory is open");

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
                    }

                    ImGui::Spacing();
                    ImGui::Separator();
                    ImGui::TextDisabled("Ghast Clicker - Linux/Wayland Port");
                    break;
                }

                case 3: {
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
                        if (combo_framed("##action", &action_idx, "Left Click\0Right Click\0Delay\0Switch Slot\0"))
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
                    break;
                }

                case 4: {
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
                    ImGui::Spacing();
                    ImGui::Separator();
                    ImGui::Text("Appearance");

                    bool theme_changed = false;
                    if (combo_framed("Accent Preset", &state.config.accent_preset, "Oxygen Blue\0Red\0Green\0Purple\0Custom\0Follow KDE\0"))
                        theme_changed = true;

                    if (state.config.accent_preset == 4) { // Custom
                        if (ImGui::ColorEdit3("Custom Color", &state.config.accent_r))
                            theme_changed = true;
                    }

                    if (theme_changed) {
                        setup_oxygen_theme(state.config.accent_preset, state.config.accent_r, state.config.accent_g, state.config.accent_b);
                    }
                    break;
                }

                case 5: {
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
                    break;
                }
            }
        }
        ImGui::End();

        ImGui::Render();
        glViewport(0, 0, display_w, display_h);
        glClearColor(0.055f, 0.086f, 0.125f, 1.0f);
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
