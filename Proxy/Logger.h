//
// Created by maciucateodor on 10/26/24.
//

#ifndef SOFTWARE_ENGINEERING_PRODUCT_TOPTEAM_LOGGER_H
#define SOFTWARE_ENGINEERING_PRODUCT_TOPTEAM_LOGGER_H

#include <fstream>
#include <iostream>
#include <iomanip>
#include <ctime>
#include <mutex>
#include <string>

class Logger {
public:
    explicit Logger(const std::string& filename) : log_file_(filename, std::ios::app) {
        if (!log_file_) {
            std::cerr << "Unable to open log file: " << filename << std::endl;
        }
    }

    void log(const std::string& action) {
        if (log_file_) {
            std::lock_guard<std::mutex> lock(log_mutex_);  // Ensure thread-safe logging
            log_file_ << current_timestamp() << ": " << action << std::endl;
        }
    }

private:
    std::ofstream log_file_;
    std::mutex log_mutex_;

    static std::string current_timestamp() {
        auto now = std::time(nullptr);
        std::tm tm = *std::localtime(&now);

        std::ostringstream oss;
        oss << std::setfill('0')
            << std::setw(2) << tm.tm_hour << ":"
            << std::setw(2) << tm.tm_min << ":"
            << std::setw(2) << tm.tm_sec << "|"
            << std::setw(2) << tm.tm_mday << "/"
            << std::setw(2) << (tm.tm_mon + 1) << "/"
            << (tm.tm_year + 1900);
        return oss.str();
    }
};

#endif //SOFTWARE_ENGINEERING_PRODUCT_TOPTEAM_LOGGER_H
