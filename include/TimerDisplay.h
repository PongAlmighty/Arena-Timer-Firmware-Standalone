/**
 * Header for displaying timer on RGB Matrix
 */

#pragma once

#include "Timer.h"
#include <Adafruit_Protomatter.h>

class TimerDisplay
{
public:
    enum class Mode
    {
        TIMER,     // Countdown mode - shows remaining time
        STOPWATCH  // Count-up mode - shows elapsed time
    };

    /// @brief Color threshold entry - defines a color for a time range
    struct ColorThreshold
    {
        unsigned int seconds;  // Time threshold in seconds (timer shows this color when remaining time <= this value)
        uint8_t r;            // Red (0-255)
        uint8_t g;            // Green (0-255)
        uint8_t b;            // Blue (0-255)
    };

    /// @brief Construct a new TimerDisplay object
    /// @param matrix Reference to the Adafruit_Protomatter matrix
    /// @param mode Timer mode (TIMER or STOPWATCH)
    TimerDisplay(Adafruit_Protomatter &matrix, Mode mode = Mode::TIMER);

    /// @brief Set the timer mode
    /// @param mode TIMER (countdown) or STOPWATCH (count-up)
    void setMode(Mode mode);

    /// @brief Set the text size
    /// @param size Text size multiplier (1 = small, 2 = medium, 3 = large, etc.)
    void setTextSize(uint8_t size);

    /// @brief Set a custom font (from Adafruit_GFX font library)
    /// @param font Pointer to GFXfont structure, or nullptr for default font
    void setFont(const GFXfont *font);

    /// @brief Set letter spacing (extra pixels between characters)
    /// @param spacing Number of pixels to add between characters (can be negative)
    void setLetterSpacing(int8_t spacing);

    /// @brief Set the text color
    /// @param r Red (0-255)
    /// @param g Green (0-255)
    /// @param b Blue (0-255)
    void setColor(uint8_t r, uint8_t g, uint8_t b);

    /// @brief Add a color threshold (timer will change to this color when remaining time <= seconds)
    /// @param seconds Time threshold in seconds
    /// @param r Red (0-255)
    /// @param g Green (0-255)
    /// @param b Blue (0-255)
    void addColorThreshold(unsigned int seconds, uint8_t r, uint8_t g, uint8_t b);

    /// @brief Clear all color thresholds
    void clearColorThresholds();

    /// @brief Get all color thresholds
    /// @return Pointer to array of thresholds and count
    const ColorThreshold* getColorThresholds(size_t& count) const;

    /// @brief Get default color RGB values
    /// @param r Output: Red (0-255)
    /// @param g Output: Green (0-255)
    /// @param b Output: Blue (0-255)
    void getDefaultColor(uint8_t& r, uint8_t& g, uint8_t& b) const;

    /// @brief Set default color (used when no threshold matches)
    /// @param r Red (0-255)
    /// @param g Green (0-255)
    /// @param b Blue (0-255)
    void setDefaultColor(uint8_t r, uint8_t g, uint8_t b);

    /// @brief Set display brightness
    /// @param brightness Brightness level (0-255, where 0 is off and 255 is full brightness)
    void setBrightness(uint8_t brightness);

    /// @brief Get current brightness level
    /// @return Current brightness (0-255)
    uint8_t getBrightness() const;

    /// @brief Get the underlying Timer object
    /// @return Reference to the Timer
    Timer &getTimer();

    /// @brief Update and draw the timer on the display. Call this in loop()
    void update();

    /// @brief Draw the timer immediately (without auto-update logic)
    void draw();

private:
    Adafruit_Protomatter &_matrix;
    Timer _timer;
    Mode _mode;
    
    uint8_t _text_size;
    const GFXfont *_current_font;  // Track the current font (NULL = default bitmap font)
    int8_t _letter_spacing;        // Extra spacing between characters (pixels)
    uint16_t _color;
    uint8_t _default_r, _default_g, _default_b;  // Default color (no threshold)
    uint8_t _brightness;  // Display brightness (0-255)
    
    // Color thresholds (sorted by seconds, descending)
    static const size_t MAX_THRESHOLDS = 10;
    ColorThreshold _thresholds[MAX_THRESHOLDS];
    size_t _threshold_count;
    
    unsigned long _last_blink_ms;
    bool _blink_state;
    bool _was_expired;  // Track if we were expired in the last update
    
    // Cached positions for different time formats to prevent jitter
    struct CachedPosition {
        int16_t x;
        int16_t y;
        bool valid;
    };
    
    CachedPosition _pos_single_digit_minutes;  // "9:99"
    CachedPosition _pos_double_digit_minutes;  // "99:99"
    CachedPosition _pos_seconds_mode;          // "99.9"
    
    /// @brief Calculate and cache centered positions for all time formats
    void calculateCachedPositions();
    
    /// @brief Draw time string with vertically centered colon/period for GFX fonts
    /// @param time_str The formatted time string (e.g., "9:59" or "59.9")
    /// @param base_x X position for the start of the string
    /// @param base_y Y position baseline
    /// @param show_ms Whether displaying milliseconds (uses period) or minutes (uses colon)
    void drawTimeWithCenteredColon(const String &time_str, int16_t base_x, int16_t base_y, bool show_ms);
    
    /// @brief Get the cached position for the current display format
    /// @param show_milliseconds Whether in seconds mode
    /// @return Cached position to use
    CachedPosition getCachedPosition(bool show_milliseconds);
    
    /// @brief Format time as mm:ss or ss.d string
    /// @param components Time components to format
    /// @param show_milliseconds If true, show ss.d format. If false, show mm:ss
    /// @return Formatted string
    String formatTime(const Timer::Components &components, bool show_milliseconds);
    
    /// @brief Get the time to display based on current mode
    /// @return Time components to display
    Timer::Components getDisplayTime();
    
    /// @brief Get the appropriate color based on remaining time and thresholds
    /// @return 16-bit color value
    uint16_t getCurrentColor();

    /// @brief Apply brightness scaling to RGB color
    /// @param r Red (0-255), modified in place
    /// @param g Green (0-255), modified in place
    /// @param b Blue (0-255), modified in place
    void applyBrightness(uint8_t& r, uint8_t& g, uint8_t& b);
};
