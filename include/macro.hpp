#pragma once
#include "clicker_config.hpp"
#include "uinput_device.hpp"
#include <atomic>
#include <chrono>
#include <thread>
#include <linux/input-event-codes.h>

class MacroRunner {
    const UInputDevice& device_;
    std::atomic<bool> running_{false};
    std::thread thread_;
    std::atomic<int> current_step_{-1};
    std::atomic<int> loop_count_{0};
    int* active_slot_ = nullptr;

public:
    MacroRunner(const UInputDevice& device) : device_(device) {}

    ~MacroRunner() { stop(); }

    MacroRunner(const MacroRunner&) = delete;
    MacroRunner& operator=(const MacroRunner&) = delete;

    bool is_running() const { return running_; }
    int current_step() const { return current_step_; }
    int loop_count() const { return loop_count_; }

    void start(const Macro& macro, int* active_slot = nullptr) {
        stop();
        active_slot_ = active_slot;
        running_ = true;
        current_step_ = 0;
        loop_count_ = 0;
        thread_ = std::thread(&MacroRunner::run, this, macro);
    }

    void stop() {
        running_ = false;
        if (thread_.joinable()) thread_.join();
        current_step_ = -1;
    }

    void toggle(const Macro& macro, int* active_slot = nullptr) {
        if (running_) stop();
        else start(macro, active_slot);
    }

private:
    void run(Macro macro) {
        do {
            for (int i = 0; i < macro.step_count && running_; i++) {
                current_step_ = i;
                execute_step(macro.steps[i]);
            }
            loop_count_++;
        } while (macro.loop && running_);
        running_ = false;
    }

    void execute_step(const MacroStep& step) {
        switch (step.action) {
            case MacroAction::LEFT_CLICK:
            case MacroAction::RIGHT_CLICK: {
                int btn = (step.action == MacroAction::LEFT_CLICK) ? BTN_LEFT : BTN_RIGHT;
                for (int i = 0; i < step.count && running_; i++) {
                    device_.click(btn, 1);
                    wait_ms(30);
                    device_.click(btn, 0);
                    if (i < step.count - 1)
                        wait_ms(step.delay_ms);
                }
                break;
            }
            case MacroAction::DELAY:
                wait_ms(step.delay_ms);
                break;
            case MacroAction::SWITCH_SLOT:
                if (step.slot >= 1 && step.slot <= 9) {
                    int key = KEY_1 + (step.slot - 1);
                    device_.click(key, 1);
                    wait_ms(20);
                    device_.click(key, 0);
                    if (active_slot_) *active_slot_ = step.slot - 1;
                }
                break;
        }
    }

    void wait_ms(double ms) {
        auto target = std::chrono::steady_clock::now() +
                      std::chrono::microseconds(static_cast<int64_t>(ms * 1000));
        while (running_ && std::chrono::steady_clock::now() < target)
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
};
