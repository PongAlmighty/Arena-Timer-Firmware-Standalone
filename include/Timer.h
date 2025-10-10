/**
 * Header for timer calculations and management
 */

#pragma once

class Timer
{
public:
    struct Components
    {
        unsigned int minutes;
        unsigned int seconds = 0;
        unsigned int milliseconds = 0;
    };

    /// @brief Construct a new Timer object
    Timer();

    /// @brief Set the timer duration (can count either up or down)
    /// @param duration Duration components (minutes, seconds, milliseconds).
    /// Seconds and milliseconds are optional and default to 0
    void setDuration(Components duration);

    /// @brief Start the timer
    void start();

    /// @brief Stop/pause the timer. Can be resumed with start().
    void stop();

    /// @brief Reset the timer
    void reset();

    /// @brief Get the elapsed time (counting up) since the timer was started
    /// @return Elapsed time components
    Components getElapsedTime();

    /// @brief Get the remaining time (counting down) until the timer reaches zero
    /// @return Remaining time components
    Components getRemainingTime();

    /// @brief Get the currently set duration of the timer
    /// @return Duration components
    Components getDuration();

    /// @brief Check if timer is currently running (not stopped/paused)
    /// @return true if running, false otherwise
    bool isRunning();

    /// @brief Check if timer is paused (stopped after being started)
    /// @return true if paused, false otherwise
    bool isPaused();

    /// @brief Check if timer is idle (reset and never started)
    /// @return true if idle, false otherwise
    bool isIdle();

    /// @brief Check if timer has reached or exceeded duration
    /// @return true if expired
    bool isExpired();

private:
    unsigned long _duration_ms;   // Currently set duration in milliseconds
    unsigned long _start_time_ms; // millis() value when started
    unsigned long _stop_time_ms;  // millis() value when stopped/paused
    unsigned long _elapsed_ms;    // Total elapsed time when paused
    bool _is_running;             // Whether the timer is currently running
    bool _is_idle;                // Whether the timer is in idle state (reset, never started)

    /// @brief Convert milliseconds to time components
    /// @param ms Total milliseconds
    /// @return Time components
    Components millisecondsToComponents(unsigned long ms);

    /// @brief Convert time components to milliseconds
    /// @param components Time components
    /// @return Total milliseconds
    unsigned long componentsToMilliseconds(const Components &components);
};