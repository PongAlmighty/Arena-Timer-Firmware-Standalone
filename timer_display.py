import displayio
import terminalio
from adafruit_display_text import label
from adafruit_bitmap_font import bitmap_font
import time

class TimerDisplay:
    MODE_TIMER = 0
    MODE_STOPWATCH = 1

    def __init__(self, display, mode=MODE_TIMER):
        self._display = display
        self._mode = mode
        self._text_size = 1
        self._current_font = terminalio.FONT
        self._letter_spacing = 3
        self._default_color = 0x00FF00  # Green
        self._brightness = 255
        self._thresholds = []
        self._last_blink_ms = 0
        self._blink_state = True
        self._was_expired = False
        
        # Timer object will be set from outside (main.py)
        self._timer = None

        # UI Group and Label
        self.group = displayio.Group()
        self.label = label.Label(self._current_font, text="00:00", color=self._default_color)
        self.label.anchor_point = (0.5, 0.5)
        self.label.anchored_position = (display.width // 2, display.height // 2)
        self.group.append(self.label)

    def set_timer(self, timer):
        self._timer = timer

    def set_mode(self, mode):
        self._mode = mode

    def set_text_size(self, size):
        self._text_size = size
        self.label.scale = size

    def set_font(self, font_path):
        try:
            if font_path is None:
                self._current_font = terminalio.FONT
            else:
                self._current_font = bitmap_font.load_font(font_path)
            self.label.font = self._current_font
        except Exception as e:
            print(f"Error loading font: {e}")

    def set_letter_spacing(self, spacing):
        # adafruit_display_text handles spacing differently, this might need fallback
        # for now we'll just store it. Some versions of label support line_spacing but not char_spacing directly easily
        self._letter_spacing = spacing

    def set_color(self, r, g, b):
        self._default_color = (r << 16) | (g << 8) | b
        self.label.color = self._default_color

    def add_color_threshold(self, seconds, r, g, b):
        color = (r << 16) | (g << 8) | b
        self._thresholds.append({"seconds": seconds, "color": color})
        # Sort thresholds descending by seconds
        self._thresholds.sort(key=lambda x: x["seconds"], reverse=True)

    def clear_color_thresholds(self):
        self._thresholds = []

    def set_brightness(self, brightness):
        self._brightness = brightness
        # In CircuitPython, brightness is usually set at the display level
        if hasattr(self._display, "brightness"):
            self._display.brightness = brightness / 255.0

    def update(self):
        if self._timer is None:
            return

        now = time.monotonic() * 1000
        
        # Handle blinking/flashing logic
        if self._timer.is_expired():
            if not self._was_expired:
                self._blink_state = True
                self._last_blink_ms = now
                self._was_expired = True
            
            if now - self._last_blink_ms >= 500:
                self._blink_state = not self._blink_state
                self._last_blink_ms = now
        elif self._timer.is_paused:
            if self._was_expired:
                self._blink_state = False
                self._last_blink_ms = now
                self._was_expired = False
            
            if now - self._last_blink_ms >= 500:
                self._blink_state = not self._blink_state
                self._last_blink_ms = now
        else:
            self._blink_state = True
            self._was_expired = False

        self.draw()

    def draw(self):
        if self._timer is None:
            return

        # Get time to show
        show_ms = False
        if self._mode == self.MODE_TIMER:
            display_ms = self._timer.get_remaining_ms()
            if display_ms < 60000:
                show_ms = True
        else:
            display_ms = self._timer.get_current_elapsed_ms()

        components = self._timer.ms_to_components(display_ms)
        
        # Format string
        if show_ms:
            # ss.d
            deciseconds = components["milliseconds"] // 100
            text = f"{components['seconds']:02d}.{deciseconds:01d}"
        else:
            if components["minutes"] < 10:
                text = f"{components['minutes']}:{components['seconds']:02d}"
            else:
                text = f"{components['minutes']:02d}:{components['seconds']:02d}"

        # Update text
        if self._blink_state:
            self.label.text = text
            self.label.color = self._get_current_color(display_ms)
        else:
            self.label.text = ""

    def _get_current_color(self, current_ms):
        if self._mode != self.MODE_TIMER or not self._thresholds:
            return self._default_color
        
        total_seconds = current_ms // 1000
        
        # Check thresholds (sorted descending)
        for t in reversed(self._thresholds):
            if total_seconds <= t["seconds"]:
                return t["color"]
        
        return self._default_color
