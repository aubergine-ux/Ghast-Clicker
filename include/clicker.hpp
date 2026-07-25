#pragma once
#include "clicker_config.hpp"
#include "tracker.hpp"
#include "variance.hpp"
#include "uinput_device.hpp"
#include <atomic>
#include <thread>
#include <chrono>

class Clicker {
    ClickerProfile profile_;
    const UInputDevice& device_;
    std::atomic<bool> enabled_{false};
    std::atomic<bool> running_{true};
    std::thread thread_;
    double current_trend_ = 0.0;

    std::atomic<uint32_t> click_count_{0};
    Tracker tracker_;

public:
    Clicker(ClickerProfile profile, const UInputDevice& device)
        : profile_(profile), device_(device) {
        thread_ = std::thread(&Clicker::run, this);
    }

    ~Clicker() {
        running_ = false;
        enabled_ = true;
        if (thread_.joinable()) thread_.join();
    }

    Clicker(const Clicker&) = delete;
    Clicker& operator=(const Clicker&) = delete;

    void set_enabled(bool v) { enabled_ = v; }
    bool is_enabled() const { return enabled_; }
    void toggle() { enabled_ = !enabled_.load(); }
    uint32_t click_count() const { return click_count_.load(); }
    ClickerProfile& profile() { return profile_; }
    const Tracker& tracker() const { return tracker_; }
    Tracker& tracker() { return tracker_; }

private:
    void run() {
        while (running_) {
            while (!enabled_ && running_) {
                std::this_thread::sleep_for(std::chrono::milliseconds(5));
            }
            if (!running_) break;

            current_trend_ = 0.0;
            if (profile_.start_delay_ms > 0) {
                std::this_thread::sleep_for(
                    std::chrono::microseconds(static_cast<int64_t>(profile_.start_delay_ms * 1000)));
            }

            while (enabled_ && running_) {
                int64_t interval_ns = calculate_interval_ns();
                int64_t hold_ns = calculate_hold_ns(interval_ns);

                device_.click(profile_.mouse_button, 1);
                click_count_++;
                wait_ns(hold_ns);
                device_.click(profile_.mouse_button, 0);

                tracker_.record(
                    1'000'000'000.0 / interval_ns,
                    hold_ns, interval_ns, current_trend_);

                wait_ns(interval_ns - hold_ns);
            }
        }
    }

    int64_t calculate_interval_ns() {
        current_trend_ = Variance::trend(current_trend_, profile_.rhythm_volatility, profile_.rhythm_tension);
        double cps = Variance::gaussian(profile_.cps_mean + current_trend_, profile_.cps_stddev);

        if (Variance::chance(profile_.spike_chance))
            cps += Variance::range(profile_.spike_min, profile_.spike_max);

        if (Variance::chance(profile_.stutter_chance))
            cps -= Variance::range(profile_.stutter_min, profile_.stutter_max);

        if (cps > profile_.cps_max)
            cps = profile_.cps_max + Variance::range(-profile_.cps_minmax_fallout, profile_.cps_minmax_fallout);
        if (cps < profile_.cps_min)
            cps = profile_.cps_min + Variance::range(-profile_.cps_minmax_fallout, profile_.cps_minmax_fallout);
        if (cps < 0.5) cps = 0.5;

        return static_cast<int64_t>(1'000'000'000.0 / cps);
    }

    int64_t calculate_hold_ns(int64_t interval_ns) {
        double speed_factor = 1.0 - (1'000'000'000.0 / interval_ns / 25.0);
        double adjusted_mean = profile_.hold_ms_mean * std::max(0.8, speed_factor);

        double hold = Variance::gaussian(adjusted_mean, profile_.hold_ms_stddev);

        if (Variance::chance(profile_.hold_ms_heavy_chance))
            hold += Variance::range(profile_.hold_ms_heavy_min, profile_.hold_ms_heavy_max);

        hold = std::max(profile_.hold_ms_min, std::min(profile_.hold_ms_max, hold));

        int64_t hold_ns = static_cast<int64_t>(hold * 1'000'000);
        int64_t max_hold = static_cast<int64_t>(interval_ns * 0.85);

        return std::min(hold_ns, max_hold);
    }

    void wait_ns(int64_t ns) {
        if (ns <= 0) return;
        auto target = std::chrono::steady_clock::now() + std::chrono::nanoseconds(ns);
        // Sleep most of it, spin the tail for precision
        auto sleep_until = target - std::chrono::microseconds(1500);
        if (std::chrono::steady_clock::now() < sleep_until)
            std::this_thread::sleep_until(sleep_until);
        while (std::chrono::steady_clock::now() < target)
            std::this_thread::yield();
    }
};
