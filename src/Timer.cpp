/**
 * Source code for timer calculations and management
 */

#include "Timer.h"
#include <Arduino.h>  // For millis()

Timer::Timer()
    : _duration_ms(0),
      _start_time_ms(0),
      _stop_time_ms(0),
      _elapsed_ms(0),
      _is_running(false),
      _is_idle(true)
{
}

void Timer::setDuration(Components duration)
{
    _duration_ms = componentsToMilliseconds(duration);
}

void Timer::start()
{
    if (!_is_running)
    {
        _is_idle = false;  // No longer idle once started
        
        // If resuming from a stop/pause, adjust start time
        if (_elapsed_ms > 0)
        {
            _start_time_ms = millis() - _elapsed_ms;
        }
        else
        {
            _start_time_ms = millis();
        }
        _is_running = true;
    }
}

void Timer::stop()
{
    if (_is_running)
    {
        _stop_time_ms = millis();
        _elapsed_ms = _stop_time_ms - _start_time_ms;
        _is_running = false;
    }
}

void Timer::reset()
{
    _is_running = false;
    _is_idle = true;  // Back to idle state
    _start_time_ms = 0;
    _stop_time_ms = 0;
    _elapsed_ms = 0;
}

Timer::Components Timer::getElapsedTime()
{
    unsigned long current_elapsed;
    
    if (_is_running)
    {
        current_elapsed = millis() - _start_time_ms;
    }
    else
    {
        current_elapsed = _elapsed_ms;
    }
    
    return millisecondsToComponents(current_elapsed);
}

Timer::Components Timer::getRemainingTime()
{
    unsigned long current_elapsed;
    
    if (_is_running)
    {
        current_elapsed = millis() - _start_time_ms;
    }
    else
    {
        current_elapsed = _elapsed_ms;
    }
    
    // If elapsed exceeds duration, remaining is 0
    if (current_elapsed >= _duration_ms)
    {
        return {0, 0, 0};
    }
    
    unsigned long remaining = _duration_ms - current_elapsed;
    return millisecondsToComponents(remaining);
}

Timer::Components Timer::getDuration()
{
    return millisecondsToComponents(_duration_ms);
}

bool Timer::isRunning()
{
    return _is_running;
}

bool Timer::isPaused()
{
    return !_is_running && !_is_idle;
}

bool Timer::isIdle()
{
    return _is_idle;
}

bool Timer::isExpired()
{
    unsigned long current_elapsed;
    
    if (_is_running)
    {
        current_elapsed = millis() - _start_time_ms;
    }
    else
    {
        current_elapsed = _elapsed_ms;
    }
    
    return current_elapsed >= _duration_ms;
}

Timer::Components Timer::millisecondsToComponents(unsigned long ms)
{
    Components result;
    
    result.minutes = ms / 60000;
    ms %= 60000;
    
    result.seconds = ms / 1000;
    ms %= 1000;
    
    result.milliseconds = ms;
    
    return result;
}

unsigned long Timer::componentsToMilliseconds(const Components &components)
{
    unsigned long total = 0;
    
    total += components.minutes * 60000UL;
    total += components.seconds * 1000UL;
    total += components.milliseconds;
    
    return total;
}