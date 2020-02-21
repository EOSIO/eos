#pragma once

#include <algorithm> // std::max_element|min_element|sort
#include <fstream>   // std::ofstream
#include <iomanip>   // std::setw
#include <iostream>  // std::cout|flush
#include <numeric>   // std::accumulate
#include <utility>   // std::pair
#include <vector>    // std::vector

#include <boost/filesystem.hpp> // boost::filesystem::unique_path|remove_all

namespace jd {
    /// Each latency test should be tested 10000 times.
    class logger {
    public:
        /// Save the file in the current directory.
        logger(std::string name) : _metrics_file{ (boost::filesystem::current_path() /= std::string{'/' + name + ".csv"}).string() } {
            _latency.reserve(10000);
        }

        void flush_all() {
            _flush_metrics();
        }

        void log_latency(const double& n) {
            _latency.push_back(n);
        }
    
    private:
        std::vector<double> _latency;
        std::ofstream _metrics_file;

        void _flush_metrics() {
            std::sort(_latency.begin(), _latency.end());
    
            ptrdiff_t quartile;
            double latency_min;
            double latency_max;
            double latency_median;
            double latency_25th_percentile;
            double latency_75th_percentile;
            double latency_mean;

            quartile = _latency.size()/4;
            latency_min = *std::min_element(_latency.cbegin(), _latency.cend());
            latency_max = *std::max_element(_latency.cbegin(), _latency.cend());
            latency_median = *(_latency.cbegin() + (_latency.size()) / 2);
            latency_25th_percentile = *(_latency.cbegin()+quartile);
            latency_75th_percentile = *(_latency.crbegin()+quartile);
            latency_mean = std::accumulate(_latency.cbegin(), _latency.cend(), 0) / static_cast<double>(_latency.size());

            _metrics_file << std::setw(10) << latency_min             << "\t"; // whisker_min
            _metrics_file << std::setw(10) << latency_25th_percentile << "\t"; // box_min
            _metrics_file << std::setw(10) << latency_median          << "\t"; // median
            _metrics_file << std::setw(10) << latency_75th_percentile << "\t"; // box_max
            _metrics_file << std::setw(10) << latency_max             << "\t"; // whisker_max
            _metrics_file << std::setw(10) << latency_mean            << "\n"; // mean
        }
    };
}
