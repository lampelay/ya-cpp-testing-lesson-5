#pragma once

#include <string>
#include <chrono>
#include <iostream>

#define PROFILE_CONCAT_INTERNAL(X, Y) X##Y
#define PROFILE_CONCAT(X, Y) PROFILE_CONCAT_INTERNAL(X, Y)
#define UNIQUE_VAR_NAME_PROFILE PROFILE_CONCAT(profileGuard, __LINE__)
#define LOG_DURATION(x) LogDuration UNIQUE_VAR_NAME_PROFILE(x)
#define LOG_DURATION_STREAM(x, os) LogDuration UNIQUE_VAR_NAME_PROFILE(x, os)

class LogDuration
{
    using Clock = std::chrono::steady_clock;

public:
    LogDuration(const std::string &id, std::ostream &os = std::cerr)
        : id_(id), os_(os)
    {
    }

    ~LogDuration()
    {
        const auto dur = Clock::now() - start_;
        os_ << id_ << ": "
            << std::chrono::duration_cast<std::chrono::microseconds>(dur).count()
            << " ms" << std::endl;
    }

private:
    const std::string id_;
    std::ostream &os_;
    const Clock::time_point start_ = Clock::now();
};