#pragma once
#include "clicker_config.hpp"
#include "uinput_device.hpp"
#include "variance.hpp"
#include <algorithm>
#include <atomic>
#include <chrono>
#include <cmath>
#include <condition_variable>
#include <cstdio>
#include <deque>
#include <functional>
#include <mutex>
#include <string>
#include <thread>

// Minecraft samples input once per 50ms client tick.
constexpr double TICK_MS = 50.0;

// Runs one delayed job at a time on its own thread. cancel() drops a pending
// job and wakes any sleep_ms() inside the running one.
class Scheduler {
    using clock = std::chrono::steady_clock;

    std::mutex mu_;
    std::condition_variable cv_;
    std::function<void()> job_;
    clock::time_point at_;
    uint64_t generation_ = 0;
    uint64_t job_generation_ = 0;
    bool busy_ = false;
    bool running_ = true;
    std::thread thread_;

public:
    Scheduler() { thread_ = std::thread(&Scheduler::run, this); }

    ~Scheduler() {
        {
            std::lock_guard<std::mutex> lock(mu_);
            running_ = false;
            generation_++;
        }
        cv_.notify_all();
        if (thread_.joinable()) thread_.join();
    }

    Scheduler(const Scheduler&) = delete;
    Scheduler& operator=(const Scheduler&) = delete;

    // Returns false while another job is pending or running.
    bool schedule(double delay_ms, std::function<void()> job) {
        std::lock_guard<std::mutex> lock(mu_);
        if (job_ || busy_) return false;
        job_ = std::move(job);
        at_ = clock::now() + std::chrono::microseconds(static_cast<int64_t>(delay_ms * 1000));
        cv_.notify_all();
        return true;
    }

    void cancel() {
        {
            std::lock_guard<std::mutex> lock(mu_);
            job_ = nullptr;
            generation_++;
        }
        cv_.notify_all();
    }

    bool idle() {
        std::lock_guard<std::mutex> lock(mu_);
        return !job_ && !busy_;
    }

    // For use inside a job. Returns false if the job was cancelled meanwhile.
    bool sleep_ms(double ms) {
        auto until = clock::now() + std::chrono::microseconds(static_cast<int64_t>(ms * 1000));
        std::unique_lock<std::mutex> lock(mu_);
        return !cv_.wait_until(lock, until, [&] { return generation_ != job_generation_ || !running_; });
    }

private:
    void run() {
        std::unique_lock<std::mutex> lock(mu_);
        while (running_) {
            if (!job_) { cv_.wait(lock); continue; }
            if (clock::now() < at_) { cv_.wait_until(lock, at_); continue; }

            auto job = std::move(job_);
            job_ = nullptr;
            busy_ = true;
            job_generation_ = generation_;
            lock.unlock();
            job();
            lock.lock();
            busy_ = false;
        }
    }
};

// Tracks the physical keyboard and owns the virtual one. While forwarding
// (physical keyboard grabbed), every physical key goes out through the
// virtual device, which is what lets W-Tap release W while you still hold it.
class VirtualKeyboard {
    static constexpr int N = UInputKeyboard::MAX_CODE + 1;

    UInputKeyboard dev_;
    std::mutex mu_;
    bool physical_[N] = {};
    bool virtual_[N] = {};
    bool masked_[N] = {};
    bool forwarding_ = false;

    void set(int code, bool down) {
        if (virtual_[code] == down) return;
        dev_.key(code, down ? 1 : 0);
        virtual_[code] = down;
    }

public:
    void on_physical(int code, int value) {
        if (code <= 0 || code >= N) return;
        std::lock_guard<std::mutex> lock(mu_);
        physical_[code] = value != 0;
        if (!forwarding_ || masked_[code]) return;
        if (value == 2) {
            if (virtual_[code]) dev_.key(code, 2);
        } else {
            set(code, value != 0);
        }
    }

    void sync_physical(const bool* down) {
        std::lock_guard<std::mutex> lock(mu_);
        for (int i = 1; i < N; i++) physical_[i] = down[i];
    }

    bool physical(int code) {
        std::lock_guard<std::mutex> lock(mu_);
        return code > 0 && code < N && physical_[code];
    }

    void set_forwarding(bool v) {
        std::lock_guard<std::mutex> lock(mu_);
        if (!v)
            for (int i = 1; i < N; i++) set(i, false);
        forwarding_ = v;
    }

    bool forwarding() {
        std::lock_guard<std::mutex> lock(mu_);
        return forwarding_;
    }

    void press(int code) {
        std::lock_guard<std::mutex> lock(mu_);
        set(code, true);
    }

    void release(int code) {
        std::lock_guard<std::mutex> lock(mu_);
        set(code, false);
    }

    // Hide a held key from the game until unmask(); only works while forwarding.
    bool mask(int code) {
        std::lock_guard<std::mutex> lock(mu_);
        if (!forwarding_) return false;
        masked_[code] = true;
        set(code, false);
        return true;
    }

    void unmask(int code) {
        std::lock_guard<std::mutex> lock(mu_);
        masked_[code] = false;
        if (forwarding_) set(code, physical_[code]);
    }
};

enum class ResetOutcome {
    TRIGGERED, SKIP_CHANCE, SKIP_MENU, SKIP_SLOT, SKIP_NOT_FORWARD,
    SKIP_KEY_HELD, SKIP_NO_GRAB, CANCELLED, COUNT
};

inline const char* outcome_name(ResetOutcome o) {
    switch (o) {
        case ResetOutcome::TRIGGERED:        return "TRIGGERED";
        case ResetOutcome::SKIP_CHANCE:      return "SKIP_CHANCE";
        case ResetOutcome::SKIP_MENU:        return "SKIP_MENU";
        case ResetOutcome::SKIP_SLOT:        return "SKIP_SLOT";
        case ResetOutcome::SKIP_NOT_FORWARD: return "SKIP_NOT_FORWARD";
        case ResetOutcome::SKIP_KEY_HELD:    return "SKIP_KEY_HELD";
        case ResetOutcome::SKIP_NO_GRAB:     return "SKIP_NO_GRAB";
        case ResetOutcome::CANCELLED:        return "CANCELLED";
        default:                             return "UNKNOWN";
    }
}

struct ResetEvent {
    int64_t timestamp_ms;
    ResetOutcome outcome;
    double delay_ms;
    double hold_ms;
    const char* action;
};

// Decision log for one module. Counters always run; history only while recording.
class ResetLog {
    static constexpr int MAX_HISTORY = 2000;

    mutable std::mutex mu_;
    std::deque<ResetEvent> history_;
    uint32_t counts_[static_cast<int>(ResetOutcome::COUNT)] = {};
    double delay_sum_ = 0;
    double hold_sum_ = 0;
    bool recording_ = false;

public:
    void record(ResetOutcome outcome, double delay_ms = -1, double hold_ms = -1, const char* action = "") {
        std::lock_guard<std::mutex> lock(mu_);
        counts_[static_cast<int>(outcome)]++;
        if (outcome == ResetOutcome::TRIGGERED) {
            delay_sum_ += delay_ms;
            hold_sum_ += hold_ms;
        }
        if (recording_) {
            if (history_.size() >= MAX_HISTORY) history_.pop_front();
            int64_t now = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::system_clock::now().time_since_epoch()).count();
            history_.push_back({now, outcome, delay_ms, hold_ms, action});
        }
    }

    uint32_t count(ResetOutcome o) const {
        std::lock_guard<std::mutex> lock(mu_);
        return counts_[static_cast<int>(o)];
    }

    uint32_t decisions() const {
        std::lock_guard<std::mutex> lock(mu_);
        uint32_t total = 0;
        for (auto c : counts_) total += c;
        return total;
    }

    double avg_delay_ms() const {
        std::lock_guard<std::mutex> lock(mu_);
        uint32_t n = counts_[static_cast<int>(ResetOutcome::TRIGGERED)];
        return n ? delay_sum_ / n : 0;
    }

    double avg_hold_ms() const {
        std::lock_guard<std::mutex> lock(mu_);
        uint32_t n = counts_[static_cast<int>(ResetOutcome::TRIGGERED)];
        return n ? hold_sum_ / n : 0;
    }

    void set_recording(bool v) { std::lock_guard<std::mutex> lock(mu_); recording_ = v; }
    bool is_recording() const { std::lock_guard<std::mutex> lock(mu_); return recording_; }

    int history_size() const {
        std::lock_guard<std::mutex> lock(mu_);
        return static_cast<int>(history_.size());
    }

    void clear() {
        std::lock_guard<std::mutex> lock(mu_);
        history_.clear();
        for (auto& c : counts_) c = 0;
        delay_sum_ = hold_sum_ = 0;
    }

    bool export_csv(const std::string& filepath) const {
        std::lock_guard<std::mutex> lock(mu_);
        FILE* f = fopen(filepath.c_str(), "w");
        if (!f) return false;

        fprintf(f, "Timestamp,Outcome,Action,DelayMS,HoldMS\n");
        for (const auto& e : history_) {
            fprintf(f, "%ld,%s,%s,%.2f,%.2f\n", (long)e.timestamp_ms,
                    outcome_name(e.outcome), e.action, e.delay_ms, e.hold_ms);
        }
        fclose(f);
        return true;
    }
};

// What the caller knows about the game at the moment of the attack.
struct AttackContext {
    bool in_menu;
    bool slot_blocked;
};

inline double clamp(double v, double lo, double hi) { return std::max(lo, std::min(hi, v)); }

inline double sample_gaussian(double mean, double stddev, double lo, double hi) {
    double v = stddev > 0 ? Variance::gaussian(mean, stddev) : mean;
    return clamp(v, std::min(lo, hi), std::max(lo, hi));
}

// Shared cooldown/chance gate. The cooldown stands in for upstream's hit
// confirmation: Minecraft only lets a player take a hit every 10 ticks, so one
// decision per window matches how often a real hit lands.
class AttackGate {
    using clock = std::chrono::steady_clock;
    std::mutex mu_;
    clock::time_point last_{};

public:
    // True if this attack starts a new decision window.
    bool claim(double cooldown_ms) {
        std::lock_guard<std::mutex> lock(mu_);
        auto now = clock::now();
        if (now - last_ < std::chrono::microseconds(static_cast<int64_t>(cooldown_ms * 1000)))
            return false;
        last_ = now;
        return true;
    }
};

// Presses Space shortly after your attack so the jump lines up with the
// knockback you take in a trade.
class JumpReset {
    const JumpResetProfile& p_;
    VirtualKeyboard& kb_;
    Scheduler sched_;
    AttackGate gate_;
    std::atomic<bool> enabled_{false};
    ResetLog log_;

public:
    JumpReset(const JumpResetProfile& p, VirtualKeyboard& kb) : p_(p), kb_(kb) {}

    bool is_enabled() const { return enabled_; }
    void set_enabled(bool v) {
        enabled_ = v;
        if (!v) sched_.cancel();
    }
    void toggle() { set_enabled(!enabled_.load()); }
    bool busy() { return !sched_.idle(); }
    ResetLog& log() { return log_; }
    const ResetLog& log() const { return log_; }

    void on_attack(const AttackContext& ctx) {
        if (!enabled_ || !sched_.idle() || !gate_.claim(p_.cooldown_ms)) return;
        if (ctx.in_menu && !p_.allow_in_menu) return log_.record(ResetOutcome::SKIP_MENU);
        if (ctx.slot_blocked && p_.use_slot_filter) return log_.record(ResetOutcome::SKIP_SLOT);
        if (p_.forward_only && !kb_.physical(KEY_W)) return log_.record(ResetOutcome::SKIP_NOT_FORWARD);

        double probability = clamp(p_.chance, 0, 1);
        if (probability <= 0 || (probability < 1 && !Variance::chance(probability)))
            return log_.record(ResetOutcome::SKIP_CHANCE);

        double delay = std::round(sample_gaussian(p_.delay_ms_mean, p_.delay_ms_stddev,
                                                  p_.delay_ms_min, p_.delay_ms_max));
        sched_.schedule(std::max(0.0, delay), [this, delay] { press(delay); });
    }

private:
    void press(double delay) {
        if (!enabled_) return log_.record(ResetOutcome::CANCELLED, delay);
        if (kb_.physical(KEY_SPACE)) return log_.record(ResetOutcome::SKIP_KEY_HELD, delay);

        double hold = std::round(sample_gaussian(p_.hold_ms_mean, p_.hold_ms_stddev,
                                                 p_.hold_ms_min, p_.hold_ms_max));
        auto start = std::chrono::steady_clock::now();
        kb_.press(KEY_SPACE);
        bool finished = sched_.sleep_ms(hold);
        kb_.release(KEY_SPACE);
        double held = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start).count();
        log_.record(finished ? ResetOutcome::TRIGGERED : ResetOutcome::CANCELLED, delay, held, "JUMP");
    }
};

inline const char* sprint_mode_name(SprintResetMode m) {
    switch (m) {
        case SprintResetMode::S_TAP:     return "S-Tap";
        case SprintResetMode::SHIFT_TAP: return "Shift-Tap";
        default:                         return "W-Tap";
    }
}

// Briefly breaks sprint after your attack (W release, S press or Shift
// press) for whole 50ms ticks, so your next hit deals full sprint knockback.
class SprintReset {
    const SprintResetProfile& p_;
    VirtualKeyboard& kb_;
    Scheduler sched_;
    AttackGate gate_;
    std::atomic<bool> enabled_{false};
    ResetLog log_;

public:
    SprintReset(const SprintResetProfile& p, VirtualKeyboard& kb) : p_(p), kb_(kb) {}

    bool is_enabled() const { return enabled_; }
    void set_enabled(bool v) {
        enabled_ = v;
        if (!v) sched_.cancel();
    }
    void toggle() { set_enabled(!enabled_.load()); }
    bool busy() { return !sched_.idle(); }
    ResetLog& log() { return log_; }
    const ResetLog& log() const { return log_; }

    // W-Tap is the only mode that has to hide a key you're physically holding.
    bool needs_keyboard_grab() const { return enabled_ && p_.mode == SprintResetMode::W_TAP; }

    void on_attack(const AttackContext& ctx) {
        if (!enabled_ || !sched_.idle() || !gate_.claim(p_.cooldown_ms)) return;
        if (ctx.in_menu) return log_.record(ResetOutcome::SKIP_MENU);
        if (ctx.slot_blocked && p_.use_slot_filter) return log_.record(ResetOutcome::SKIP_SLOT);
        bool need_w = p_.forward_only || p_.mode == SprintResetMode::W_TAP;
        if (need_w && !kb_.physical(KEY_W)) return log_.record(ResetOutcome::SKIP_NOT_FORWARD);

        double probability = clamp(p_.chance, 0, 1);
        if (probability <= 0 || (probability < 1 && !Variance::chance(probability)))
            return log_.record(ResetOutcome::SKIP_CHANCE);

        double lo = clamp(std::min(p_.delay_ms_min, p_.delay_ms_max), 0, 500);
        double hi = clamp(std::max(p_.delay_ms_min, p_.delay_ms_max), lo, 500);
        double delay = std::round(Variance::range(lo, hi));
        SprintResetMode mode = p_.mode;
        sched_.schedule(delay, [this, delay, mode] { tap(delay, mode); });
    }

private:
    int sample_ticks() const {
        double lo = clamp(p_.duration_ticks_min, 1, 3);
        double hi = clamp(p_.duration_ticks_max, lo, 3);
        double base = sample_gaussian(clamp(p_.duration_ticks_mean, lo, hi),
                                      clamp(p_.duration_ticks_stddev, 0, 1), lo, hi);
        // Stochastic rounding keeps the configured mean instead of always
        // snapping to the nearest whole tick.
        int ticks = static_cast<int>(std::floor(base));
        double frac = base - ticks;
        if (frac > 0 && Variance::chance(frac)) ticks++;
        return std::max(static_cast<int>(std::round(lo)), std::min(static_cast<int>(std::round(hi)), ticks));
    }

    void tap(double delay, SprintResetMode mode) {
        const char* action = sprint_mode_name(mode);
        if (!enabled_) return log_.record(ResetOutcome::CANCELLED, delay, -1, action);

        double ms = sample_ticks() * TICK_MS;
        auto start = std::chrono::steady_clock::now();
        bool finished;

        if (mode == SprintResetMode::W_TAP) {
            if (!kb_.physical(KEY_W)) return log_.record(ResetOutcome::SKIP_NOT_FORWARD, delay, -1, action);
            if (!kb_.mask(KEY_W)) return log_.record(ResetOutcome::SKIP_NO_GRAB, delay, -1, action);
            finished = sched_.sleep_ms(ms);
            kb_.unmask(KEY_W);
        } else {
            int key = mode == SprintResetMode::S_TAP ? KEY_S : KEY_LEFTSHIFT;
            if (kb_.physical(key)) return log_.record(ResetOutcome::SKIP_KEY_HELD, delay, -1, action);
            kb_.press(key);
            finished = sched_.sleep_ms(ms);
            kb_.release(key);
        }

        double held = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start).count();
        log_.record(finished ? ResetOutcome::TRIGGERED : ResetOutcome::CANCELLED, delay, held, action);
    }
};

// Owns the virtual keyboard and both reset modules. Constructing it creates
// the uinput keyboard, so it throws on missing /dev/uinput access.
class Combat {
public:
    VirtualKeyboard keyboard;
    JumpReset jump;
    SprintReset sprint;

    explicit Combat(Config& cfg)
        : jump(cfg.jump_reset, keyboard), sprint(cfg.sprint_reset, keyboard) {}

    bool any_enabled() const { return jump.is_enabled() || sprint.is_enabled(); }

    void on_attack(const AttackContext& ctx) {
        jump.on_attack(ctx);
        sprint.on_attack(ctx);
    }
};
