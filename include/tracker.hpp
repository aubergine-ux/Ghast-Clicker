#pragma once
#include <cstdint>
#include <cstdio>
#include <chrono>
#include <deque>
#include <mutex>
#include <string>

struct ClickData {
    int64_t timestamp_ms;
    double target_cps;
    double actual_cps;
    int64_t hold_ns;
    int64_t interval_ns;
    double trend;
};

class Tracker {
    static constexpr int WINDOW = 20;
    static constexpr int MAX_HISTORY = 2000;
    static constexpr int GRAPH_SIZE = 120;

    double cps_samples_[WINDOW] = {};
    double hold_samples_[WINDOW] = {};
    int pos_ = 0;
    int count_ = 0;
    uint64_t total_clicks_ = 0;

    float cps_graph_[GRAPH_SIZE] = {};
    int graph_pos_ = 0;
    int graph_count_ = 0;

    std::deque<ClickData> history_;
    mutable std::mutex mu_;
    bool recording_ = false;

    static int64_t now_ms() {
        return std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
    }

public:
    void record(double cps, double hold_ns, double interval_ns, double trend) {
        cps_samples_[pos_] = cps;
        hold_samples_[pos_] = hold_ns / 1'000'000.0;
        pos_ = (pos_ + 1) % WINDOW;
        if (count_ < WINDOW) count_++;
        total_clicks_++;

        cps_graph_[graph_pos_] = static_cast<float>(cps);
        graph_pos_ = (graph_pos_ + 1) % GRAPH_SIZE;
        if (graph_count_ < GRAPH_SIZE) graph_count_++;

        if (recording_) {
            std::lock_guard<std::mutex> lock(mu_);
            if (history_.size() >= MAX_HISTORY)
                history_.pop_front();
            history_.push_back({now_ms(), cps, cps, static_cast<int64_t>(hold_ns),
                                static_cast<int64_t>(interval_ns), trend});
        }
    }

    double avg_cps() const {
        if (count_ == 0) return 0;
        double sum = 0;
        for (int i = 0; i < count_; i++) sum += cps_samples_[i];
        return sum / count_;
    }

    double avg_hold_ms() const {
        if (count_ == 0) return 0;
        double sum = 0;
        for (int i = 0; i < count_; i++) sum += hold_samples_[i];
        return sum / count_;
    }

    uint64_t total() const { return total_clicks_; }

    const float* graph_data() const { return cps_graph_; }
    int graph_offset() const { return graph_pos_; }
    int graph_count() const { return graph_count_; }
    static constexpr int graph_size() { return GRAPH_SIZE; }

    void set_recording(bool v) { recording_ = v; }
    bool is_recording() const { return recording_; }

    int history_size() const {
        std::lock_guard<std::mutex> lock(mu_);
        return static_cast<int>(history_.size());
    }

    void clear_history() {
        std::lock_guard<std::mutex> lock(mu_);
        history_.clear();
    }

    bool export_csv(const std::string& filepath) const {
        std::lock_guard<std::mutex> lock(mu_);
        FILE* f = fopen(filepath.c_str(), "w");
        if (!f) return false;

        fprintf(f, "Timestamp,TargetCPS,ActualCPS,HoldMS,IntervalMS,Trend\n");
        for (const auto& d : history_) {
            fprintf(f, "%ld,%.2f,%.2f,%.2f,%.2f,%.2f\n",
                    (long)d.timestamp_ms, d.target_cps, d.actual_cps,
                    d.hold_ns / 1'000'000.0, d.interval_ns / 1'000'000.0, d.trend);
        }
        fclose(f);
        return true;
    }

    void reset() {
        pos_ = 0;
        count_ = 0;
        total_clicks_ = 0;
        clear_history();
    }
};
