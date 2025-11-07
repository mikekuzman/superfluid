#pragma once

#include <vector>
#include <algorithm>
#include <cmath>
#include <numeric>

namespace BEC {

/**
 * Statistical utilities for data analysis
 */
class Statistics {
public:
    /**
     * Calculate mean of a dataset
     */
    template<typename T>
    static double mean(const std::vector<T>& data) {
        if (data.empty()) return 0.0;
        double sum = std::accumulate(data.begin(), data.end(), 0.0);
        return sum / data.size();
    }

    /**
     * Calculate standard deviation
     */
    template<typename T>
    static double std_dev(const std::vector<T>& data, double mean_val) {
        if (data.size() < 2) return 0.0;

        double variance = 0.0;
        for (const auto& val : data) {
            double diff = val - mean_val;
            variance += diff * diff;
        }
        variance /= data.size();

        return std::sqrt(variance);
    }

    template<typename T>
    static double std_dev(const std::vector<T>& data) {
        return std_dev(data, mean(data));
    }

    /**
     * Calculate percentile
     */
    template<typename T>
    static double percentile(std::vector<T> data, double p) {
        if (data.empty()) return 0.0;
        if (p <= 0.0) return *std::min_element(data.begin(), data.end());
        if (p >= 100.0) return *std::max_element(data.begin(), data.end());

        // Sort the data
        std::sort(data.begin(), data.end());

        // Calculate index
        double index = (p / 100.0) * (data.size() - 1);
        size_t lower_index = static_cast<size_t>(std::floor(index));
        size_t upper_index = static_cast<size_t>(std::ceil(index));

        if (lower_index == upper_index) {
            return data[lower_index];
        }

        // Linear interpolation
        double weight = index - lower_index;
        return data[lower_index] * (1.0 - weight) + data[upper_index] * weight;
    }

    /**
     * Calculate min and max
     */
    template<typename T>
    static std::pair<T, T> min_max(const std::vector<T>& data) {
        if (data.empty()) return {T(), T()};
        auto result = std::minmax_element(data.begin(), data.end());
        return {*result.first, *result.second};
    }

    /**
     * Calculate median
     */
    template<typename T>
    static double median(std::vector<T> data) {
        return percentile(data, 50.0);
    }

    /**
     * Calculate multiple statistics at once (more efficient)
     */
    template<typename T>
    struct Stats {
        T min_val;
        T max_val;
        double mean_val;
        double std_val;
        double p5;
        double p95;
        size_t count;
    };

    template<typename T>
    static Stats<T> calculate_all(std::vector<T> data) {
        Stats<T> stats;
        stats.count = data.size();

        if (data.empty()) {
            stats.min_val = T();
            stats.max_val = T();
            stats.mean_val = 0.0;
            stats.std_val = 0.0;
            stats.p5 = 0.0;
            stats.p95 = 0.0;
            return stats;
        }

        // Calculate mean
        stats.mean_val = mean(data);

        // Calculate std dev
        stats.std_val = std_dev(data, stats.mean_val);

        // Sort for percentiles and min/max
        std::sort(data.begin(), data.end());

        stats.min_val = data.front();
        stats.max_val = data.back();

        // Calculate percentiles on sorted data
        stats.p5 = percentile_sorted(data, 5.0);
        stats.p95 = percentile_sorted(data, 95.0);

        return stats;
    }

private:
    /**
     * Percentile calculation for already-sorted data
     */
    template<typename T>
    static double percentile_sorted(const std::vector<T>& sorted_data, double p) {
        if (sorted_data.empty()) return 0.0;
        if (p <= 0.0) return sorted_data.front();
        if (p >= 100.0) return sorted_data.back();

        double index = (p / 100.0) * (sorted_data.size() - 1);
        size_t lower_index = static_cast<size_t>(std::floor(index));
        size_t upper_index = static_cast<size_t>(std::ceil(index));

        if (lower_index == upper_index) {
            return sorted_data[lower_index];
        }

        double weight = index - lower_index;
        return sorted_data[lower_index] * (1.0 - weight) + sorted_data[upper_index] * weight;
    }
};

} // namespace BEC
