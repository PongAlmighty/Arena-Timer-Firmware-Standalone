/**
 * Source code for displaying timer on RGB Matrix
 */

#include "TimerDisplay.h"
#include <Arduino.h>

TimerDisplay::TimerDisplay(Adafruit_Protomatter &matrix, Mode mode)
    : _matrix(matrix),
      _timer(),
      _mode(mode),
      _text_size(1),
      _current_font(NULL),  // Start with default bitmap font
      _letter_spacing(3),   // Default letter spacing of 3 pixels
      _color(matrix.color565(255, 255, 255)), // Default white
      _default_r(0), _default_g(255), _default_b(0), // Default green
      _threshold_count(0),
      _last_blink_ms(0),
      _blink_state(true),
      _was_expired(false)
{
    // Initialize cached positions as invalid
    _pos_single_digit_minutes.valid = false;
    _pos_double_digit_minutes.valid = false;
    _pos_seconds_mode.valid = false;
    
    // Set up default color thresholds
    // Green by default, Yellow < 2min, Red < 1min
    clearColorThresholds();
    addColorThreshold(120, 255, 255, 0);  // Yellow at 2 minutes
    addColorThreshold(60, 255, 0, 0);     // Red at 1 minute
}

void TimerDisplay::setMode(Mode mode)
{
    _mode = mode;
}

void TimerDisplay::setTextSize(uint8_t size)
{
    _text_size = size;
    calculateCachedPositions();
}

void TimerDisplay::setFont(const GFXfont *font)
{
    _current_font = font;  // Track the font
    _matrix.setFont(font);
    calculateCachedPositions();
}

void TimerDisplay::setLetterSpacing(int8_t spacing)
{
    _letter_spacing = spacing;
    calculateCachedPositions(); // Recalculate since spacing affects width
}

void TimerDisplay::setColor(uint8_t r, uint8_t g, uint8_t b)
{
    _color = _matrix.color565(r, g, b);
}

void TimerDisplay::addColorThreshold(unsigned int seconds, uint8_t r, uint8_t g, uint8_t b)
{
    if (_threshold_count >= MAX_THRESHOLDS) {
        return; // Array full
    }
    
    // Add new threshold
    _thresholds[_threshold_count] = {seconds, r, g, b};
    _threshold_count++;
    
    // Sort thresholds by seconds (descending order - highest time first)
    // Simple bubble sort since we have few elements
    for (size_t i = 0; i < _threshold_count - 1; i++) {
        for (size_t j = 0; j < _threshold_count - i - 1; j++) {
            if (_thresholds[j].seconds < _thresholds[j + 1].seconds) {
                ColorThreshold temp = _thresholds[j];
                _thresholds[j] = _thresholds[j + 1];
                _thresholds[j + 1] = temp;
            }
        }
    }
}

void TimerDisplay::clearColorThresholds()
{
    _threshold_count = 0;
}

const TimerDisplay::ColorThreshold* TimerDisplay::getColorThresholds(size_t& count) const
{
    count = _threshold_count;
    return _thresholds;
}

void TimerDisplay::getDefaultColor(uint8_t& r, uint8_t& g, uint8_t& b) const
{
    r = _default_r;
    g = _default_g;
    b = _default_b;
}

void TimerDisplay::setDefaultColor(uint8_t r, uint8_t g, uint8_t b)
{
    _default_r = r;
    _default_g = g;
    _default_b = b;
}

void TimerDisplay::drawTimeWithCenteredColon(const String &time_str, int16_t base_x, int16_t base_y, bool show_ms)
{
    // This function draws the time string with the colon (or period) vertically centered
    // relative to the numbers, which is needed for GFX fonts
    
    char separator = show_ms ? '.' : ':';
    
    // Find the separator position in the string
    int separator_pos = time_str.indexOf(separator);
    if (separator_pos == -1)
    {
        // No separator found, draw normally
        _matrix.setCursor(base_x, base_y);
        _matrix.print(time_str);
        return;
    }
    
    // Split the string into before and after separator
    String before_sep = time_str.substring(0, separator_pos);
    String after_sep = time_str.substring(separator_pos + 1);
    
    // Measure the height of a digit (use "8" as it's typically the tallest)
    int16_t x1, y1;
    uint16_t w, h;
    _matrix.getTextBounds("8", 0, 0, &x1, &y1, &w, &h);
    int16_t digit_y1 = y1;
    int16_t digit_height = h;
    
    // Measure the height of the separator
    char sep_str[2] = {separator, '\0'};
    _matrix.getTextBounds(sep_str, 0, 0, &x1, &y1, &w, &h);
    int16_t separator_y1 = y1;
    int16_t separator_height = h;
    
    // Calculate vertical offset - only for colon, not decimal
    // Colon should be vertically centered with digits
    // Decimal should be bottom-aligned with digits (no offset)
    int16_t sep_offset = 0;
    if (!show_ms) {  // If it's a colon (not showing milliseconds)
        sep_offset = (digit_y1 + digit_height / 2) - (separator_y1 + separator_height / 2);
    }
    
    // Draw the parts using cursor advancement for proper spacing
    int16_t current_x = base_x;
    
    // Draw before separator (minutes or seconds) character by character for letter spacing
    for (unsigned int i = 0; i < before_sep.length(); i++)
    {
        _matrix.setCursor(current_x, base_y);
        _matrix.print(before_sep.charAt(i));
        current_x = _matrix.getCursorX() + _letter_spacing;
    }
    
    // Draw separator with vertical offset
    _matrix.setCursor(current_x, base_y + sep_offset);
    _matrix.print(separator);
    // Advance cursor
    current_x = _matrix.getCursorX() + _letter_spacing;
    
    // Draw after separator (seconds or tenths) character by character for letter spacing
    for (unsigned int i = 0; i < after_sep.length(); i++)
    {
        _matrix.setCursor(current_x, base_y);
        _matrix.print(after_sep.charAt(i));
        current_x = _matrix.getCursorX() + _letter_spacing;
    }
}

Timer &TimerDisplay::getTimer()
{
    return _timer;
}

void TimerDisplay::update()
{
    unsigned long current_ms = millis();
    
    // Handle flashing when expired (check this first, even if running)
    if (_timer.isExpired())
    {
        // If we just became expired, start with visible state
        if (!_was_expired)
        {
            _blink_state = true;
            _last_blink_ms = current_ms;
            _was_expired = true;
        }
        
        // Flash faster when expired (every 500ms)
        if (current_ms - _last_blink_ms >= 500)
        {
            _blink_state = !_blink_state;
            _last_blink_ms = current_ms;
        }
    }
    // Handle blinking when paused (not idle, not running)
    else if (_timer.isPaused())
    {
        // If we just became paused, start with invisible state
        if (_was_expired)
        {
            _blink_state = false;
            _last_blink_ms = current_ms;
            _was_expired = false;
        }
        
        if (current_ms - _last_blink_ms >= 500)
        {
            _blink_state = !_blink_state;
            _last_blink_ms = current_ms;
        }
    }
    else
    {
        // Always show when running normally or idle (no blinking)
        _blink_state = true;
        _was_expired = false;
    }

    draw();
}

void TimerDisplay::draw()
{
    Timer::Components time_to_show = getDisplayTime();
    
    // Determine if we should show milliseconds (only in timer mode when <1 minute)
    bool show_ms = false;
    if (_mode == Mode::TIMER)
    {
        Timer::Components remaining = _timer.getRemainingTime();
        if (remaining.minutes == 0)
        {
            show_ms = true;
        }
    }
    
    String time_str = formatTime(time_to_show, show_ms);
    
    // Get cached position for this format
    CachedPosition pos = getCachedPosition(show_ms);
    
    // Draw text only if blink state is true
    if (_blink_state)
    {
        _matrix.setTextColor(getCurrentColor());
        
        // For GFX fonts, we need to draw the colon separately with vertical centering
        // The default 5x7 font doesn't need this adjustment
        if (_current_font != NULL)
        {
            // Using a custom GFX font - need to center the colon vertically
            drawTimeWithCenteredColon(time_str, pos.x, pos.y, show_ms);
        }
        else
        {
            // Using default bitmap font - draw normally
            _matrix.setCursor(pos.x, pos.y);
            _matrix.print(time_str);
        }
    }
}

void TimerDisplay::calculateCachedPositions()
{
    int16_t x1, y1;
    uint16_t w, h;
    
    _matrix.setTextSize(_text_size);
    
    // Calculate position for single digit minutes: "9:99"
    _matrix.getTextBounds("9:99", 0, 0, &x1, &y1, &w, &h);
    // Add letter spacing to width (3 characters + 1 separator = 4 characters, so 3 gaps)
    w += _letter_spacing * 3;
    _pos_single_digit_minutes.x = (_matrix.width() - w) / 2 - x1;
    _pos_single_digit_minutes.y = (_matrix.height() - h) / 2 - y1;
    _pos_single_digit_minutes.valid = true;
    
    // Calculate position for double digit minutes: "99:99"
    _matrix.getTextBounds("99:99", 0, 0, &x1, &y1, &w, &h);
    // Add letter spacing to width (4 characters + 1 separator = 5 characters, so 4 gaps)
    w += _letter_spacing * 4;
    _pos_double_digit_minutes.x = (_matrix.width() - w) / 2 - x1;
    _pos_double_digit_minutes.y = (_matrix.height() - h) / 2 - y1;
    _pos_double_digit_minutes.valid = true;
    
    // Calculate position for seconds mode: "99.9"
    _matrix.getTextBounds("99.9", 0, 0, &x1, &y1, &w, &h);
    // Add letter spacing to width (3 characters + 1 separator = 4 characters, so 3 gaps)
    w += _letter_spacing * 3;
    _pos_seconds_mode.x = (_matrix.width() - w) / 2 - x1;
    _pos_seconds_mode.y = (_matrix.height() - h) / 2 - y1;
    _pos_seconds_mode.valid = true;
}

TimerDisplay::CachedPosition TimerDisplay::getCachedPosition(bool show_milliseconds)
{
    // If positions aren't calculated yet, calculate them now
    if (!_pos_single_digit_minutes.valid)
    {
        calculateCachedPositions();
    }
    
    if (show_milliseconds)
    {
        return _pos_seconds_mode;
    }
    else
    {
        // Check the actual time being displayed to choose format
        Timer::Components time_to_show = getDisplayTime();
        if (time_to_show.minutes < 10)
        {
            return _pos_single_digit_minutes;
        }
        else
        {
            return _pos_double_digit_minutes;
        }
    }
}

String TimerDisplay::formatTime(const Timer::Components &components, bool show_milliseconds)
{
    char buffer[10];
    
    if (show_milliseconds)
    {
        // Format as ss.d (e.g., "59.9" or "05.1")
        unsigned int deciseconds = components.milliseconds / 100; // Convert ms to tenths of seconds (0-9)
        snprintf(buffer, sizeof(buffer), "%02u.%01u", components.seconds, deciseconds);
    }
    else
    {
        // Format based on actual displayed time
        if (components.minutes < 10)
        {
            // Use single digit format: "9:59"
            snprintf(buffer, sizeof(buffer), "%u:%02u", components.minutes, components.seconds);
        }
        else
        {
            // Use double digit format: "10:00"
            snprintf(buffer, sizeof(buffer), "%02u:%02u", components.minutes, components.seconds);
        }
    }
    
    return String(buffer);
}

Timer::Components TimerDisplay::getDisplayTime()
{
    // If timer is running, show current time
    if (_timer.isRunning())
    {
        if (_mode == Mode::TIMER)
        {
            return _timer.getRemainingTime();
        }
        else // STOPWATCH
        {
            return _timer.getElapsedTime();
        }
    }
    
    // If timer is stopped/paused, check if it's been reset
    Timer::Components elapsed = _timer.getElapsedTime();
    
    // If elapsed is 0, timer was reset
    if (elapsed.minutes == 0 && elapsed.seconds == 0 && elapsed.milliseconds == 0)
    {
        if (_mode == Mode::TIMER)
        {
            // Show the set duration
            return _timer.getDuration();
        }
        else // STOPWATCH
        {
            // Show 0
            return {0, 0, 0};
        }
    }
    
    // Otherwise, show the paused time
    if (_mode == Mode::TIMER)
    {
        return _timer.getRemainingTime();
    }
    else // STOPWATCH
    {
        return _timer.getElapsedTime();
    }
}

uint16_t TimerDisplay::getCurrentColor()
{
    // Only apply color thresholds in TIMER mode
    if (_mode != Mode::TIMER || _threshold_count == 0) {
        return _matrix.color565(_default_r, _default_g, _default_b);
    }
    
    // Get remaining time in seconds
    Timer::Components remaining = _timer.getRemainingTime();
    unsigned int total_seconds = remaining.minutes * 60 + remaining.seconds;
    
    // Check thresholds (already sorted descending, so we check highest first)
    for (size_t i = _threshold_count; i > 0; i--) {
        if (total_seconds <= _thresholds[i-1].seconds) {
            return _matrix.color565(_thresholds[i-1].r, _thresholds[i-1].g, _thresholds[i-1].b);
        }
    }
    
    // No threshold matched, use default color
    return _matrix.color565(_default_r, _default_g, _default_b);
}
