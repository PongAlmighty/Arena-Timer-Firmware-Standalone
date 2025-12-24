from adafruit_httpserver import Server, Request, Response, POST, GET
import adafruit_connection_manager
# Use the official socket interface for the ESP32SPI
import adafruit_esp32spi.adafruit_esp32spi_socket as socket
import json

class WebServer:
    def __init__(self, timer, timer_display, esp):
        self.timer = timer
        self.timer_display = timer_display
        self.esp = esp
        
        # Configure the global socket interface to use our ESP object
        socket.set_interface(self.esp)
        
        # Create server using the official socket compatibility layer
        # This layer provides bind/listen/accept methods that were missing
        self.server = Server(socket, "/static")
        self.setup_routes()
        
        try:
             print(f"Starting server on {self.esp.pretty_ip(self.esp.ip_address)}")
             self.server.start(str(self.esp.pretty_ip(self.esp.ip_address)))
        except Exception as e:
             print(f"Server start message: {e}")

    def setup_routes(self):
        @self.server.route("/")
        def base(request: Request):
            return Response(request, "Redirecting...", status=302, location="/index.html")

        @self.server.route("/api", methods=(POST,))
        def api_control(request: Request):
            params = request.query_params
            if not params:
                try:
                    body = request.body.decode("utf-8")
                    params = {p.split("=")[0]: p.split("=")[1] for p in (body.split("&") if "&" in body else [body])}
                except:
                    pass

            action = params.get("action")
            if action == "start":
                self.timer.start()
                return Response(request, "Started")
            elif action == "pause":
                self.timer.stop()
                return Response(request, "Paused")
            elif action == "reset":
                self.timer.reset()
                return Response(request, "Reset")
            elif action == "flip":
                return Response(request, "Flipped")
            elif action == "settings":
                duration = int(params.get("duration", 180))
                self.timer.set_duration(duration // 60, duration % 60)
                brightness = int(params.get("brightness", 255))
                self.timer_display.set_brightness(brightness)
                return Response(request, "Settings Applied")
            
            return Response(request, "Unknown action", status=400)

        @self.server.route("/api/status")
        def status(request: Request):
            data = {
                "isPaused": self.timer.is_paused,
                "isRunning": self.timer.is_running
            }
            return Response(request, json.dumps(data), content_type="application/json")

        @self.server.route("/api/network/status")
        def network_status(request: Request):
            ip = self.esp.pretty_ip(self.esp.ip_address) if self.esp.is_connected else "0.0.0.0"
            return Response(request, json.dumps({"ip": ip}), content_type="application/json")

        @self.server.route("/api/thresholds", methods=(POST, GET))
        def thresholds(request: Request):
            if request.method == POST:
                return Response(request, "Thresholds Updated")
            return Response(request, json.dumps({"thresholds": [], "defaultColor": "#00FF00"}), content_type="application/json")

    def poll(self):
        try:
            self.server.poll()
        except Exception as e:
            pass
