import time

class Timer:
    def __init__(self):
        self._duration_ms = 0
        self._start_time_monotonic = 0
        self._stop_time_monotonic = 0
        self._elapsed_ms = 0
        self._is_running = False
        self._is_idle = True

    def set_duration(self, minutes, seconds, milliseconds=0):
        self._duration_ms = (minutes * 60000) + (seconds * 1000) + milliseconds

    def start(self):
        if not self._is_running:
            self._is_idle = False
            now = time.monotonic() * 1000  # Convert to ms
            
            if self._elapsed_ms > 0:
                self._start_time_monotonic = now - self._elapsed_ms
            else:
                self._start_time_monotonic = now
            self._is_running = True

    def stop(self):
        if self._is_running:
            self._stop_time_monotonic = time.monotonic() * 1000
            self._elapsed_ms = self._stop_time_monotonic - self._start_time_monotonic
            self._is_running = False

    def reset(self):
        self._is_running = False
        self._is_idle = True
        self._start_time_monotonic = 0
        self._stop_time_monotonic = 0
        self._elapsed_ms = 0

    @property
    def is_running(self):
        return self._is_running

    @property
    def is_paused(self):
        return not self._is_running and not self._is_idle

    @property
    def is_idle(self):
        return self._is_idle

    def is_expired(self):
        return self.get_current_elapsed_ms() >= self._duration_ms

    def get_current_elapsed_ms(self):
        if self._is_running:
            return (time.monotonic() * 1000) - self._start_time_monotonic
        return self._elapsed_ms

    def get_remaining_ms(self):
        elapsed = self.get_current_elapsed_ms()
        if elapsed >= self._duration_ms:
            return 0
        return self._duration_ms - elapsed

    def get_duration_ms(self):
        return self._duration_ms

    @staticmethod
    def ms_to_components(ms):
        minutes = int(ms // 60000)
        ms %= 60000
        seconds = int(ms // 1000)
        ms %= 1000
        return {
            "minutes": minutes,
            "seconds": seconds,
            "milliseconds": int(ms)
        }
