"""
Arena Timer Controller - FightTimer to Hardware Bridge

This script connects to FightTimer via Socket.IO and controls your Arena Timer
hardware directly via HTTP API calls. No modifications needed to FightTimer!

Usage:
    python arena_timer_controller.py

Configuration:
    Update the IP addresses below to match your setup
"""

import socketio
import requests
import time
import threading
from typing import Optional

class ArenaTimerController:
    def __init__(self, fighttimer_host='127.0.0.1', fighttimer_port=8765, 
                 arena_timer_host='10.0.0.21', arena_timer_port=80, debug=True):
        """
        Controller that bridges FightTimer to Arena Timer hardware
        
        Args:
            fighttimer_host: IP of FightTimer server
            fighttimer_port: Port of FightTimer server  
            arena_timer_host: IP of Arena Timer hardware (RP2040)
            arena_timer_port: Port of Arena Timer web server (usually 80)
            debug: Enable debug output (default: True)
        """
        self.fighttimer_url = f"http://{fighttimer_host}:{fighttimer_port}"
        self.arena_timer_url = f"http://{arena_timer_host}:{arena_timer_port}"
        
        # Socket.IO client for FightTimer
        self.sio = socketio.Client()
        self.setup_socketio_handlers()
        
        # State tracking
        self.connected_to_fighttimer = False
        self.connected_to_arena_timer = False
        self.last_arena_timer_check = 0
        self.current_duration = 180  # Track current timer duration (default 3 minutes)
        self.debug = debug
        
        print(f"Arena Timer Controller initialized")
        print(f"FightTimer: {self.fighttimer_url}")
        print(f"Arena Timer: {self.arena_timer_url}")
    
    def debug_print(self, message: str):
        """Print debug message only if debug mode is enabled"""
        if self.debug:
            print(message)
    
    def setup_socketio_handlers(self):
        """Setup Socket.IO event handlers for FightTimer"""
        
        @self.sio.event
        def connect():
            print("✅ Connected to FightTimer")
            self.connected_to_fighttimer = True
        
        @self.sio.event
        def disconnect():
            print("❌ Disconnected from FightTimer")
            self.connected_to_fighttimer = False
        
        @self.sio.on('timer_update')
        def on_timer_update(data):
            """Handle timer updates from FightTimer"""
            self.debug_print(f"📡 FightTimer event: {data}")
            
            try:
                action = data.get('action')
                
                # Try different ways FightTimer might send duration
                minutes = data.get('minutes')
                seconds = data.get('seconds')
                duration_direct = data.get('duration')
                
                # Calculate duration from available data
                if duration_direct is not None:
                    duration = duration_direct
                elif minutes is not None and seconds is not None:
                    duration = minutes * 60 + seconds
                elif minutes is not None:
                    duration = minutes * 60
                else:
                    # Use stored duration (handles FightTimer API violation where start/stop don't include duration)
                    duration = self.current_duration
                
                if action == 'start':
                    self.start_arena_timer(duration)
                elif action == 'stop':
                    self.pause_arena_timer()
                elif action == 'reset':
                    # Store the duration when we reset
                    self.current_duration = duration
                    self.reset_arena_timer(duration)
                elif action == 'settings':
                    # Handle settings updates if needed
                    settings = data.get('settings', {})
                    self.debug_print(f"⚙️ Settings update: {settings}")
                    # Also store duration if provided in settings
                    if duration_direct is not None or minutes is not None:
                        self.current_duration = duration
                else:
                    self.debug_print(f"⚠️ Unknown action: {action}")
                    
            except Exception as e:
                self.debug_print(f"❌ Error handling timer update: {e}")
                if self.debug:
                    import traceback
                    traceback.print_exc()
    
    def check_arena_timer_connection(self) -> bool:
        """Check if Arena Timer hardware is reachable"""
        try:
            response = requests.get(f"{self.arena_timer_url}/api/status", timeout=2)
            self.connected_to_arena_timer = (response.status_code == 200)
            return self.connected_to_arena_timer
        except requests.exceptions.RequestException:
            self.connected_to_arena_timer = False
            return False
    
    def check_fighttimer_connection(self) -> bool:
        """Check if FightTimer web server is reachable"""
        try:
            response = requests.get(self.fighttimer_url, timeout=3)
            return response.status_code == 200
        except requests.exceptions.RequestException:
            return False
    
    def start_arena_timer(self, duration_seconds: int):
        """Start the Arena Timer - just send start command, reset/settings are handled separately"""
        start_time = time.time()
        try:
            # Simply send start command - the timer should already be configured from reset/settings events
            response = requests.post(
                f"{self.arena_timer_url}/api", 
                data="action=start",
                headers={'Content-Type': 'application/x-www-form-urlencoded'},
                timeout=5
            )
            
            if response.status_code == 200:
                elapsed = (time.time() - start_time) * 1000
                self.debug_print(f"▶️ Arena Timer started (took {elapsed:.0f}ms)")
            else:
                self.debug_print(f"❌ Failed to start Arena Timer: {response.status_code}")
                
        except requests.exceptions.RequestException as e:
            self.debug_print(f"❌ Error starting Arena Timer: {e}")
    
    def pause_arena_timer(self):
        """Pause the Arena Timer"""
        try:
            response = requests.post(
                f"{self.arena_timer_url}/api", 
                data="action=pause",
                headers={'Content-Type': 'application/x-www-form-urlencoded'},
                timeout=5
            )
            
            if response.status_code == 200:
                self.debug_print("⏸️ Arena Timer paused")
            else:
                self.debug_print(f"❌ Failed to pause Arena Timer: {response.status_code}")
                
        except requests.exceptions.RequestException as e:
            self.debug_print(f"❌ Error pausing Arena Timer: {e}")
    
    def reset_arena_timer(self, duration_seconds: int):
        """Reset the Arena Timer to specified duration"""
        try:
            duration_minutes = duration_seconds // 60
            duration_seconds_remainder = duration_seconds % 60
            
            # Check if timer is currently running before reset
            was_running = False
            try:
                status_response = requests.get(f"{self.arena_timer_url}/api/status", timeout=2)
                if status_response.status_code == 200:
                    status_data = status_response.json()
                    was_running = status_data.get('isRunning', False) and not status_data.get('isPaused', True)
                    if was_running:
                        self.debug_print(f"🏃 Timer was running, will restart after reset")
            except:
                # If status check fails, assume not running
                pass
            
            # Set duration first
            response = requests.post(
                f"{self.arena_timer_url}/api", 
                data=f"action=settings&duration={duration_seconds}",
                headers={'Content-Type': 'application/x-www-form-urlencoded'},
                timeout=5
            )
            
            if response.status_code == 200:
                self.debug_print(f"⚙️ Arena Timer duration set to {duration_minutes}:{duration_seconds_remainder:02d}")
            
            # Reset the timer
            response = requests.post(
                f"{self.arena_timer_url}/api", 
                data="action=reset",
                headers={'Content-Type': 'application/x-www-form-urlencoded'},
                timeout=5
            )
            
            if response.status_code == 200:
                self.debug_print(f"🔄 Arena Timer reset to {duration_minutes}:{duration_seconds_remainder:02d}")
                
                # If timer was running before reset, restart it (FightTimer behavior)
                if was_running:
                    time.sleep(0.1)  # Small delay to ensure reset completes
                    restart_response = requests.post(
                        f"{self.arena_timer_url}/api", 
                        data="action=start",
                        headers={'Content-Type': 'application/x-www-form-urlencoded'},
                        timeout=5
                    )
                    if restart_response.status_code == 200:
                        self.debug_print(f"▶️ Arena Timer restarted (mimicking FightTimer behavior)")
                    else:
                        self.debug_print(f"❌ Failed to restart Arena Timer after reset: {restart_response.status_code}")
            else:
                self.debug_print(f"❌ Failed to reset Arena Timer: {response.status_code}")
                
        except requests.exceptions.RequestException as e:
            self.debug_print(f"❌ Error resetting Arena Timer: {e}")
    
    def status_monitor(self):
        """Background thread to monitor connection status"""
        while True:
            time.sleep(5)  # Check every 5 seconds
            
            # Check Arena Timer connection
            current_time = time.time()
            if current_time - self.last_arena_timer_check > 10:  # Check every 10 seconds
                was_connected = self.connected_to_arena_timer
                is_connected = self.check_arena_timer_connection()
                
                if was_connected != is_connected:
                    if is_connected:
                        self.debug_print("✅ Arena Timer hardware connected")
                    else:
                        self.debug_print("❌ Arena Timer hardware disconnected")
                
                self.last_arena_timer_check = current_time
    
    def connect_to_fighttimer(self, max_retries=3):
        """Connect to FightTimer Socket.IO server with retry logic"""
        for attempt in range(max_retries):
            try:
                self.debug_print(f"🔌 Connecting to FightTimer at {self.fighttimer_url}... (attempt {attempt + 1}/{max_retries})")
                self.sio.connect(self.fighttimer_url)
                return True
            except Exception as e:
                if "10061" in str(e) or "Connection refused" in str(e):
                    self.debug_print(f"❌ FightTimer not responding on {self.fighttimer_url}")
                    if attempt < max_retries - 1:
                        self.debug_print("   Retrying in 3 seconds...")
                        time.sleep(3)
                    else:
                        self.debug_print("\n💡 Troubleshooting:")
                        self.debug_print("   1. Is FightTimer running? Start it with: python main.py")
                        self.debug_print("   2. Check if FightTimer is on a different port")
                        self.debug_print("   3. Try accessing http://127.0.0.1:8765 in your browser")
                else:
                    self.debug_print(f"❌ Failed to connect to FightTimer: {e}")
                    if attempt < max_retries - 1:
                        self.debug_print("   Retrying in 3 seconds...")
                        time.sleep(3)
        return False
    
    def disconnect(self):
        """Disconnect from FightTimer"""
        if self.connected_to_fighttimer:
            self.sio.disconnect()
    
    def run(self):
        """Main run loop"""
        self.debug_print("\n🚀 Starting Arena Timer Controller...")
        
        # Check initial Arena Timer connection
        if self.check_arena_timer_connection():
            self.debug_print("✅ Arena Timer hardware is reachable")
        else:
            self.debug_print("⚠️ Arena Timer hardware not reachable - will keep trying...")
        
        # Check FightTimer web server first
        self.debug_print("🌐 Checking FightTimer web server...")
        if self.check_fighttimer_connection():
            self.debug_print("✅ FightTimer web server is reachable")
        else:
            self.debug_print("❌ FightTimer web server not reachable")
            self.debug_print("\n💡 Please start FightTimer first:")
            self.debug_print("   cd /path/to/FightTimer")
            self.debug_print("   python main.py")
            self.debug_print("\n   Then try this controller again.")
            return
        
        # Start status monitor thread
        monitor_thread = threading.Thread(target=self.status_monitor, daemon=True)
        monitor_thread.start()
        
        # Connect to FightTimer Socket.IO
        if not self.connect_to_fighttimer():
            self.debug_print("❌ Failed to connect to FightTimer Socket.IO. Exiting.")
            return
        
        self.debug_print("\n🎯 Arena Timer Controller is running!")
        self.debug_print("Control your timer from FightTimer's web interface")
        self.debug_print("Press Ctrl+C to stop\n")
        
        try:
            # Keep the main thread alive
            while True:
                time.sleep(1)
                
        except KeyboardInterrupt:
            self.debug_print("\n👋 Shutting down Arena Timer Controller...")
            self.disconnect()

def main():
    # Configuration - modify these IPs as needed
    FIGHTTIMER_HOST = '127.0.0.1'    # Your PC running FightTimer
    FIGHTTIMER_PORT = 8765           # FightTimer port
    ARENA_TIMER_HOST = '10.0.0.21'   # Your RP2040 IP address
    ARENA_TIMER_PORT = 80            # Arena Timer web server port
    DEBUG_OUTPUT = False              # Set to False to disable debug messages for better timing
    
    print("🎮 Arena Timer Controller")
    print("=" * 40)
    
    controller = ArenaTimerController(
        fighttimer_host=FIGHTTIMER_HOST,
        fighttimer_port=FIGHTTIMER_PORT,
        arena_timer_host=ARENA_TIMER_HOST,
        arena_timer_port=ARENA_TIMER_PORT,
        debug=DEBUG_OUTPUT
    )
    
    controller.run()

if __name__ == "__main__":
    main()