#pragma once
#include "clicker_config.hpp"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <algorithm>
#include <string>
#include <vector>
#include <dirent.h>
#include <sys/stat.h>

class ConfigFile {
public:
    static std::string path() {
        const char* home = getenv("HOME");
        if (!home) home = "/tmp";
        return std::string(home) + "/.config/ghast-clicker";
    }

    static void write_profile(FILE* f, const char* prefix, const ClickerProfile& p) {
        fprintf(f, "%s.cps_mean %g\n", prefix, p.cps_mean);
        fprintf(f, "%s.cps_stddev %g\n", prefix, p.cps_stddev);
        fprintf(f, "%s.cps_min %g\n", prefix, p.cps_min);
        fprintf(f, "%s.cps_max %g\n", prefix, p.cps_max);
        fprintf(f, "%s.cps_minmax_fallout %g\n", prefix, p.cps_minmax_fallout);
        fprintf(f, "%s.spike_chance %g\n", prefix, p.spike_chance);
        fprintf(f, "%s.spike_min %g\n", prefix, p.spike_min);
        fprintf(f, "%s.spike_max %g\n", prefix, p.spike_max);
        fprintf(f, "%s.stutter_chance %g\n", prefix, p.stutter_chance);
        fprintf(f, "%s.stutter_min %g\n", prefix, p.stutter_min);
        fprintf(f, "%s.stutter_max %g\n", prefix, p.stutter_max);
        fprintf(f, "%s.hold_ms_mean %g\n", prefix, p.hold_ms_mean);
        fprintf(f, "%s.hold_ms_stddev %g\n", prefix, p.hold_ms_stddev);
        fprintf(f, "%s.hold_ms_min %g\n", prefix, p.hold_ms_min);
        fprintf(f, "%s.hold_ms_max %g\n", prefix, p.hold_ms_max);
        fprintf(f, "%s.hold_ms_heavy_chance %g\n", prefix, p.hold_ms_heavy_chance);
        fprintf(f, "%s.hold_ms_heavy_min %g\n", prefix, p.hold_ms_heavy_min);
        fprintf(f, "%s.hold_ms_heavy_max %g\n", prefix, p.hold_ms_heavy_max);
        fprintf(f, "%s.rhythm_volatility %g\n", prefix, p.rhythm_volatility);
        fprintf(f, "%s.rhythm_tension %g\n", prefix, p.rhythm_tension);
        fprintf(f, "%s.start_delay_ms %g\n", prefix, p.start_delay_ms);
        fprintf(f, "%s.double_click_chance %g\n", prefix, p.double_click_chance);
        fprintf(f, "%s.double_click_delay_min %g\n", prefix, p.double_click_delay_min);
        fprintf(f, "%s.double_click_delay_max %g\n", prefix, p.double_click_delay_max);
        fprintf(f, "%s.breaks_enabled %d\n", prefix, p.breaks_enabled ? 1 : 0);
        fprintf(f, "%s.break_interval_min %g\n", prefix, p.break_interval_min);
        fprintf(f, "%s.break_interval_max %g\n", prefix, p.break_interval_max);
        fprintf(f, "%s.break_duration_min %g\n", prefix, p.break_duration_min);
        fprintf(f, "%s.break_duration_max %g\n", prefix, p.break_duration_max);
        fprintf(f, "%s.distribution %d\n", prefix, static_cast<int>(p.distribution));
    }

    static void read_profile_key(const char* key, double val, const char* prefix, ClickerProfile& p) {
        char buf[96];
        auto match = [&](const char* field) {
            snprintf(buf, sizeof(buf), "%s.%s", prefix, field);
            return strcmp(key, buf) == 0;
        };
        if (match("cps_mean")) p.cps_mean = val;
        else if (match("cps_stddev")) p.cps_stddev = val;
        else if (match("cps_min")) p.cps_min = val;
        else if (match("cps_max")) p.cps_max = val;
        else if (match("cps_minmax_fallout")) p.cps_minmax_fallout = val;
        else if (match("spike_chance")) p.spike_chance = val;
        else if (match("spike_min")) p.spike_min = val;
        else if (match("spike_max")) p.spike_max = val;
        else if (match("stutter_chance")) p.stutter_chance = val;
        else if (match("stutter_min")) p.stutter_min = val;
        else if (match("stutter_max")) p.stutter_max = val;
        else if (match("hold_ms_mean")) p.hold_ms_mean = val;
        else if (match("hold_ms_stddev")) p.hold_ms_stddev = val;
        else if (match("hold_ms_min")) p.hold_ms_min = val;
        else if (match("hold_ms_max")) p.hold_ms_max = val;
        else if (match("hold_ms_heavy_chance")) p.hold_ms_heavy_chance = val;
        else if (match("hold_ms_heavy_min")) p.hold_ms_heavy_min = val;
        else if (match("hold_ms_heavy_max")) p.hold_ms_heavy_max = val;
        else if (match("rhythm_volatility")) p.rhythm_volatility = val;
        else if (match("rhythm_tension")) p.rhythm_tension = val;
        else if (match("start_delay_ms")) p.start_delay_ms = val;
        else if (match("double_click_chance")) p.double_click_chance = val;
        else if (match("double_click_delay_min")) p.double_click_delay_min = val;
        else if (match("double_click_delay_max")) p.double_click_delay_max = val;
        else if (match("breaks_enabled")) p.breaks_enabled = (int)val != 0;
        else if (match("break_interval_min")) p.break_interval_min = val;
        else if (match("break_interval_max")) p.break_interval_max = val;
        else if (match("break_duration_min")) p.break_duration_min = val;
        else if (match("break_duration_max")) p.break_duration_max = val;
        else if (match("distribution")) p.distribution = static_cast<DistributionType>(static_cast<int>(val));
    }

    static void write_combat(FILE* f, const Config& cfg) {
        const auto& j = cfg.jump_reset;
        fprintf(f, "jump_reset.chance %g\n", j.chance);
        fprintf(f, "jump_reset.delay_ms_mean %g\n", j.delay_ms_mean);
        fprintf(f, "jump_reset.delay_ms_stddev %g\n", j.delay_ms_stddev);
        fprintf(f, "jump_reset.delay_ms_min %g\n", j.delay_ms_min);
        fprintf(f, "jump_reset.delay_ms_max %g\n", j.delay_ms_max);
        fprintf(f, "jump_reset.hold_ms_mean %g\n", j.hold_ms_mean);
        fprintf(f, "jump_reset.hold_ms_stddev %g\n", j.hold_ms_stddev);
        fprintf(f, "jump_reset.hold_ms_min %g\n", j.hold_ms_min);
        fprintf(f, "jump_reset.hold_ms_max %g\n", j.hold_ms_max);
        fprintf(f, "jump_reset.cooldown_ms %g\n", j.cooldown_ms);
        fprintf(f, "jump_reset.forward_only %d\n", j.forward_only ? 1 : 0);
        fprintf(f, "jump_reset.allow_in_menu %d\n", j.allow_in_menu ? 1 : 0);
        fprintf(f, "jump_reset.use_slot_filter %d\n", j.use_slot_filter ? 1 : 0);

        const auto& r = cfg.sprint_reset;
        fprintf(f, "sprint_reset.mode %d\n", static_cast<int>(r.mode));
        fprintf(f, "sprint_reset.chance %g\n", r.chance);
        fprintf(f, "sprint_reset.delay_ms_min %g\n", r.delay_ms_min);
        fprintf(f, "sprint_reset.delay_ms_max %g\n", r.delay_ms_max);
        fprintf(f, "sprint_reset.duration_ticks_mean %g\n", r.duration_ticks_mean);
        fprintf(f, "sprint_reset.duration_ticks_stddev %g\n", r.duration_ticks_stddev);
        fprintf(f, "sprint_reset.duration_ticks_min %g\n", r.duration_ticks_min);
        fprintf(f, "sprint_reset.duration_ticks_max %g\n", r.duration_ticks_max);
        fprintf(f, "sprint_reset.cooldown_ms %g\n", r.cooldown_ms);
        fprintf(f, "sprint_reset.forward_only %d\n", r.forward_only ? 1 : 0);
        fprintf(f, "sprint_reset.use_slot_filter %d\n", r.use_slot_filter ? 1 : 0);
    }

    static bool read_combat_key(const char* key, double val, Config& cfg) {
        auto& j = cfg.jump_reset;
        auto& r = cfg.sprint_reset;
        if (!strcmp(key, "jump_reset.chance")) j.chance = val;
        else if (!strcmp(key, "jump_reset.delay_ms_mean")) j.delay_ms_mean = val;
        else if (!strcmp(key, "jump_reset.delay_ms_stddev")) j.delay_ms_stddev = val;
        else if (!strcmp(key, "jump_reset.delay_ms_min")) j.delay_ms_min = val;
        else if (!strcmp(key, "jump_reset.delay_ms_max")) j.delay_ms_max = val;
        else if (!strcmp(key, "jump_reset.hold_ms_mean")) j.hold_ms_mean = val;
        else if (!strcmp(key, "jump_reset.hold_ms_stddev")) j.hold_ms_stddev = val;
        else if (!strcmp(key, "jump_reset.hold_ms_min")) j.hold_ms_min = val;
        else if (!strcmp(key, "jump_reset.hold_ms_max")) j.hold_ms_max = val;
        else if (!strcmp(key, "jump_reset.cooldown_ms")) j.cooldown_ms = val;
        else if (!strcmp(key, "jump_reset.forward_only")) j.forward_only = (int)val != 0;
        else if (!strcmp(key, "jump_reset.allow_in_menu")) j.allow_in_menu = (int)val != 0;
        else if (!strcmp(key, "jump_reset.use_slot_filter")) j.use_slot_filter = (int)val != 0;
        else if (!strcmp(key, "sprint_reset.mode")) r.mode = static_cast<SprintResetMode>(std::max(0, std::min(2, (int)val)));
        else if (!strcmp(key, "sprint_reset.chance")) r.chance = val;
        else if (!strcmp(key, "sprint_reset.delay_ms_min")) r.delay_ms_min = val;
        else if (!strcmp(key, "sprint_reset.delay_ms_max")) r.delay_ms_max = val;
        else if (!strcmp(key, "sprint_reset.duration_ticks_mean")) r.duration_ticks_mean = val;
        else if (!strcmp(key, "sprint_reset.duration_ticks_stddev")) r.duration_ticks_stddev = val;
        else if (!strcmp(key, "sprint_reset.duration_ticks_min")) r.duration_ticks_min = val;
        else if (!strcmp(key, "sprint_reset.duration_ticks_max")) r.duration_ticks_max = val;
        else if (!strcmp(key, "sprint_reset.cooldown_ms")) r.cooldown_ms = val;
        else if (!strcmp(key, "sprint_reset.forward_only")) r.forward_only = (int)val != 0;
        else if (!strcmp(key, "sprint_reset.use_slot_filter")) r.use_slot_filter = (int)val != 0;
        else return false;
        return true;
    }

    static bool load(Config& cfg, const std::string& filename = "config") {
        std::string p = path() + "/" + filename;
        FILE* f = fopen(p.c_str(), "r");
        if (!f) return false;

        char line[256];
        while (fgets(line, sizeof(line), f)) {
            char key[96];
            double val;
            if (sscanf(line, "%95s %lf", key, &val) != 2) continue;

            read_profile_key(key, val, "left", cfg.left);
            read_profile_key(key, val, "right", cfg.right);
            if (read_combat_key(key, val, cfg)) continue;

            if (!strcmp(key, "key_left")) cfg.key_left = (int)val;
            else if (!strcmp(key, "key_right")) cfg.key_right = (int)val;
            else if (!strcmp(key, "key_inventory")) cfg.key_inventory = (int)val;
            else if (!strcmp(key, "key_jump_reset")) cfg.key_jump_reset = (int)val;
            else if (!strcmp(key, "key_sprint_reset")) cfg.key_sprint_reset = (int)val;
            else if (!strcmp(key, "left_mode")) cfg.left_mode = (int)val == 0 ? ActivationMode::TOGGLE : ((int)val == 1 ? ActivationMode::HOLD : ActivationMode::MOUSE);
            else if (!strcmp(key, "right_mode")) cfg.right_mode = (int)val == 0 ? ActivationMode::TOGGLE : ((int)val == 1 ? ActivationMode::HOLD : ActivationMode::MOUSE);
            else if (!strcmp(key, "slot_filter_enabled")) cfg.slot_filter_enabled = (int)val != 0;
            else if (!strcmp(key, "audio_enabled")) cfg.audio_enabled = (int)val != 0;
            else if (!strcmp(key, "macro_count")) { /* counted implicitly */ }
            else {
                bool handled = false;
                for (int i = 0; i < 9 && !handled; i++) {
                    char sbuf[32];
                    snprintf(sbuf, sizeof(sbuf), "allowed_slot_%d", i);
                    if (!strcmp(key, sbuf)) { cfg.allowed_slots[i] = (int)val != 0; handled = true; }
                }
                for (int m = 0; m < MAX_MACROS && !handled; m++) {
                    char buf[64];
                    snprintf(buf, sizeof(buf), "macro_%d.name", m);
                    if (!strcmp(key, buf)) {
                        const char* name_start = strstr(line, buf);
                        if (name_start) {
                            name_start += strlen(buf);
                            while (*name_start == ' ') name_start++;
                            strncpy(cfg.macros[m].name, name_start, 31);
                            cfg.macros[m].name[31] = '\0';
                            char* nl = strchr(cfg.macros[m].name, '\n');
                            if (nl) *nl = '\0';
                        }
                        handled = true; break;
                    }
                    snprintf(buf, sizeof(buf), "macro_%d.loop", m);
                    if (!strcmp(key, buf)) { cfg.macros[m].loop = (int)val != 0; handled = true; break; }
                    snprintf(buf, sizeof(buf), "macro_%d.key_bind", m);
                    if (!strcmp(key, buf)) { cfg.macros[m].key_bind = (int)val; handled = true; break; }
                    snprintf(buf, sizeof(buf), "macro_%d.step_count", m);
                    if (!strcmp(key, buf)) { cfg.macros[m].step_count = std::min((int)val, MAX_MACRO_STEPS); handled = true; break; }
                    for (int s = 0; s < MAX_MACRO_STEPS && !handled; s++) {
                        snprintf(buf, sizeof(buf), "macro_%d.step_%d.action", m, s);
                        if (!strcmp(key, buf)) { cfg.macros[m].steps[s].action = static_cast<MacroAction>((int)val); handled = true; break; }
                        snprintf(buf, sizeof(buf), "macro_%d.step_%d.count", m, s);
                        if (!strcmp(key, buf)) { cfg.macros[m].steps[s].count = (int)val; handled = true; break; }
                        snprintf(buf, sizeof(buf), "macro_%d.step_%d.delay_ms", m, s);
                        if (!strcmp(key, buf)) { cfg.macros[m].steps[s].delay_ms = val; handled = true; break; }
                        snprintf(buf, sizeof(buf), "macro_%d.step_%d.slot", m, s);
                        if (!strcmp(key, buf)) { cfg.macros[m].steps[s].slot = (int)val; handled = true; break; }
                    }
                }
            }
        }
        fclose(f);
        return true;
    }

    static bool save(const Config& cfg, const std::string& filename = "config") {
        std::string dir = path();
        mkdir(dir.c_str(), 0755);

        std::string p = dir + "/" + filename;
        FILE* f = fopen(p.c_str(), "w");
        if (!f) return false;

        write_profile(f, "left", cfg.left);
        write_profile(f, "right", cfg.right);

        fprintf(f, "key_left %d\n", cfg.key_left);
        fprintf(f, "key_right %d\n", cfg.key_right);
        fprintf(f, "key_inventory %d\n", cfg.key_inventory);
        fprintf(f, "key_jump_reset %d\n", cfg.key_jump_reset);
        fprintf(f, "key_sprint_reset %d\n", cfg.key_sprint_reset);
        write_combat(f, cfg);
        fprintf(f, "left_mode %d\n", cfg.left_mode == ActivationMode::TOGGLE ? 0 : (cfg.left_mode == ActivationMode::HOLD ? 1 : 2));
        fprintf(f, "right_mode %d\n", cfg.right_mode == ActivationMode::TOGGLE ? 0 : (cfg.right_mode == ActivationMode::HOLD ? 1 : 2));
        fprintf(f, "slot_filter_enabled %d\n", cfg.slot_filter_enabled ? 1 : 0);
        for (int i = 0; i < 9; i++)
            fprintf(f, "allowed_slot_%d %d\n", i, cfg.allowed_slots[i] ? 1 : 0);

        fprintf(f, "audio_enabled %d\n", cfg.audio_enabled ? 1 : 0);

        int macro_count = 0;
        for (int m = 0; m < MAX_MACROS; m++)
            if (cfg.macros[m].step_count > 0 || cfg.macros[m].name[0]) macro_count = m + 1;
        fprintf(f, "macro_count %d\n", macro_count);
        for (int m = 0; m < macro_count; m++) {
            const auto& mac = cfg.macros[m];
            fprintf(f, "macro_%d.name %s\n", m, mac.name[0] ? mac.name : "Untitled");
            fprintf(f, "macro_%d.loop %d\n", m, mac.loop ? 1 : 0);
            fprintf(f, "macro_%d.key_bind %d\n", m, mac.key_bind);
            fprintf(f, "macro_%d.step_count %d\n", m, mac.step_count);
            for (int s = 0; s < mac.step_count; s++) {
                const auto& step = mac.steps[s];
                fprintf(f, "macro_%d.step_%d.action %d\n", m, s, static_cast<int>(step.action));
                fprintf(f, "macro_%d.step_%d.count %d\n", m, s, step.count);
                fprintf(f, "macro_%d.step_%d.delay_ms %g\n", m, s, step.delay_ms);
                fprintf(f, "macro_%d.step_%d.slot %d\n", m, s, step.slot);
            }
        }

        fclose(f);
        return true;
    }

    static std::vector<std::string> list_profiles() {
        std::vector<std::string> result;
        std::string dir = path();
        DIR* d = opendir(dir.c_str());
        if (!d) return result;

        struct dirent* entry;
        while ((entry = readdir(d))) {
            if (entry->d_name[0] == '.') continue;
            result.push_back(entry->d_name);
        }
        closedir(d);
        return result;
    }

    static bool remove_profile(const std::string& filename) {
        std::string p = path() + "/" + filename;
        return ::remove(p.c_str()) == 0;
    }
};
