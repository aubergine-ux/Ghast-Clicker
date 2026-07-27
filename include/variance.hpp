#pragma once
#include <random>

class Variance {
public:
    static double gaussian(double mean, double stddev) {
        thread_local std::mt19937 rng{std::random_device{}()};
        std::normal_distribution<double> dist(mean, stddev);
        return dist(rng);
    }

    static bool chance(double probability) {
        thread_local std::mt19937 rng{std::random_device{}()};
        std::uniform_real_distribution<double> dist(0.0, 1.0);
        return dist(rng) < probability;
    }

    static double range(double min, double max) {
        if (min >= max) return min;
        thread_local std::mt19937 rng{std::random_device{}()};
        std::uniform_real_distribution<double> dist(min, max);
        return dist(rng);
    }

    static double poisson(double mean) {
        if (mean <= 0) return 0;
        thread_local std::mt19937 rng{std::random_device{}()};
        std::poisson_distribution<int> dist(mean);
        return static_cast<double>(dist(rng));
    }

    static double trend(double current, double volatility, double tension) {
        double move = range(-volatility, volatility);
        return (current + move) * (1.0 - tension);
    }
};
