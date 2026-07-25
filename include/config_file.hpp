#pragma once
#include "clicker_config.hpp"
#include <cstdio>
#include <cstdlib>
#include <cstring>
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

            if (!strcmp(key, "key_left")) cfg.key_left = (int)val;
            else if (!strcmp(key, "key_right")) cfg.key_right = (int)val;
            else if (!strcmp(key, "key_inventory")) cfg.key_inventory = (int)val;
            else if (!strcmp(key, "left_mode")) cfg.left_mode = (int)val == 0 ? ActivationMode::TOGGLE : ((int)val == 1 ? ActivationMode::HOLD : ActivationMode::MOUSE);
            else if (!strcmp(key, "right_mode")) cfg.right_mode = (int)val == 0 ? ActivationMode::TOGGLE : ((int)val == 1 ? ActivationMode::HOLD : ActivationMode::MOUSE);
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
        fprintf(f, "left_mode %d\n", cfg.left_mode == ActivationMode::TOGGLE ? 0 : (cfg.left_mode == ActivationMode::HOLD ? 1 : 2));
        fprintf(f, "right_mode %d\n", cfg.right_mode == ActivationMode::TOGGLE ? 0 : (cfg.right_mode == ActivationMode::HOLD ? 1 : 2));

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
