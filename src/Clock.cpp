#include "Clock.h"
#include "mfwlog.h"

namespace mfw {
    Timer::Timer()
    {
        start = Time::GetCurrent();
    }

    Timer::~Timer() {
        duration = getDuration();
    }

    f32 Timer::getDuration() {
        return Time::GetCurrent() - start;
    }

    void Timer::log() {
    }

    Clock::Clock(const std::string& message)
        : message(message)
    {}

    Clock::~Clock() {
        LOG_INFO("[{}][{}ms]\n", message, timer.getDuration() * 1000.0);
    }

}
