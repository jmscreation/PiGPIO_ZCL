#ifndef __CLOCK_H__
#define __CLOCK_H__

#include <chrono>
#include <atomic>

typedef std::chrono::time_point<std::chrono::system_clock> timepoint;

class Clock {
    std::atomic<timepoint> start;
public:
    Clock();
    Clock(const Clock&) = default;
    Clock& operator=(const Clock&) = default;
    ~Clock()=default;

    double getSeconds() const;
    double getMilliseconds() const;
    inline void restart() { start = std::chrono::system_clock::now(); }
    void setSeconds(double time);
    void setMilliseconds(double time);
};

#endif // __CLOCK_H__