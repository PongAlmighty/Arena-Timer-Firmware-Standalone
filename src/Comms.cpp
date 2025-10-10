#include "Comms.h"
#include "WebSocketClient.h"
#include <SPI.h>
#include <EthernetBonjour.h>

// Font includes (12pt and below for 64x32 display)
#include <Fonts/FreeMono9pt7b.h>
#include <Fonts/FreeMono12pt7b.h>
#include <Fonts/FreeMonoBold9pt7b.h>
#include <Fonts/FreeMonoBold12pt7b.h>
#include <Fonts/FreeSans9pt7b.h>
#include <Fonts/FreeSans12pt7b.h>
#include <Fonts/FreeSansBold9pt7b.h>
#include <Fonts/FreeSansBold12pt7b.h>
#include <Fonts/FreeSerif9pt7b.h>
#include <Fonts/FreeSerif12pt7b.h>
#include <Fonts/FreeSerifBold9pt7b.h>
#include <Fonts/FreeSerifBold12pt7b.h>
// Retro/pixel fonts
#include <Fonts/Org_01.h>
#include <Fonts/Picopixel.h>
#include <Fonts/TomThumb.h>
// Custom fonts
#include <CustomFonts/Aquire_BW0ox12pt7b.h>
#include <CustomFonts/AquireBold_8Ma6012pt7b.h>
#include <CustomFonts/AquireLight_YzE0o12pt7b.h>

namespace Comms {
    // Pin definitions for W5500
    const int CS = 1;    // RP2040 GPIO1
    const int SCK = 2;   // RP2040 GPIO2
    const int MOSI = 3;  // RP2040 GPIO3
    const int MISO = 4;  // RP2040 GPIO4

    EthernetServer* server = nullptr;
    bool mdns_initialized = false;
    WebSocketClient* wsClient = nullptr;

    bool init(uint8_t mac[6], uint8_t ip[4]) {
        // Configure SPI pins for W5500
        SPI.setSCK(SCK);
        SPI.setTX(MOSI);
        SPI.setRX(MISO);
        SPI.setCS(CS);
        
        // Must call SPI.begin() to actually start the SPI peripheral
        SPI.begin();

        // Tell Ethernet library which CS pin to use
        Ethernet.init(CS);
        
        // Initialize Ethernet with static IP
        Ethernet.begin(mac, IPAddress(ip[0], ip[1], ip[2], ip[3]));

        // Give Ethernet time to initialize
        delay(1000);

        // Check if Ethernet is connected
        if (Ethernet.hardwareStatus() == EthernetNoHardware) {
            Serial.println("ERROR: Ethernet hardware not found");
            return false;
        }

        if (Ethernet.linkStatus() == LinkOFF) {
            Serial.println("WARNING: Ethernet cable not connected");
        }

        Serial.print("Ethernet initialized - IP: ");
        Serial.println(Ethernet.localIP());

        return true;
    }

    bool initMDNS(const char* hostname) {
        if (!EthernetBonjour.begin(hostname)) {
            Serial.println("ERROR: Failed to start mDNS responder");
            mdns_initialized = false;
            return false;
        }

        Serial.print("mDNS responder started: ");
        Serial.print(hostname);
        Serial.println(".local");
        mdns_initialized = true;
        return true;
    }

    void updateMDNS() {
        if (mdns_initialized) {
            EthernetBonjour.run();
        }
    }


    String getIPAddressString() {
        IPAddress ip = Ethernet.localIP();
        return String(ip[0]) + "." + String(ip[1]) + "." + String(ip[2]) + "." + String(ip[3]);
    }

    void startWebServer(uint16_t port) {
        if (server != nullptr) {
            delete server;
        }
        server = new EthernetServer(port);
        server->begin();
        Serial.print("Web server started on port ");
        Serial.println(port);
    }

    EthernetServer& getServer() {
        return *server;
    }

    void setWebSocketClient(WebSocketClient* client) {
        wsClient = client;
        Serial.println("WebSocket client registered with Comms");
    }

    // Helper function to send HTTP response
    void sendHTTPResponse(EthernetClient& client, int code, const char* contentType, const String& body) {
        client.print("HTTP/1.1 ");
        client.print(code);
        client.println(code == 200 ? " OK" : " Error");
        client.print("Content-Type: ");
        client.println(contentType);
        client.println("Connection: close");
        client.println();
        client.print(body);
    }

    // Helper function to URL decode a string
    String urlDecode(const String& str) {
        String decoded = "";
        char temp[] = "00";
        for (unsigned int i = 0; i < str.length(); i++) {
            char c = str.charAt(i);
            if (c == '%' && i + 2 < str.length()) {
                temp[0] = str.charAt(i + 1);
                temp[1] = str.charAt(i + 2);
                decoded += (char)strtol(temp, NULL, 16);
                i += 2;
            } else if (c == '+') {
                decoded += ' ';
            } else {
                decoded += c;
            }
        }
        return decoded;
    }

    // Helper function to parse hex color string to RGB
    void parseColor(const String& hexColor, uint8_t& r, uint8_t& g, uint8_t& b) {
        String color = hexColor;
        if (color.startsWith("#")) {
            color = color.substring(1);
        }
        
        long number = strtol(color.c_str(), NULL, 16);
        r = (number >> 16) & 0xFF;
        g = (number >> 8) & 0xFF;
        b = number & 0xFF;
    }

    // Helper function to get font pointer from font ID
    const GFXfont* getFontById(int fontId) {
        switch(fontId) {
            case 0: return nullptr; // Default font (5x7 pixels)
            // Sans-Serif fonts
            case 1: return &FreeSans9pt7b;
            case 2: return &FreeSans12pt7b;
            case 3: return &FreeSansBold9pt7b;
            case 4: return &FreeSansBold12pt7b;
            // Monospace fonts
            case 5: return &FreeMono9pt7b;
            case 6: return &FreeMono12pt7b;
            case 7: return &FreeMonoBold9pt7b;
            case 8: return &FreeMonoBold12pt7b;
            // Serif fonts
            case 9: return &FreeSerif9pt7b;
            case 10: return &FreeSerif12pt7b;
            case 11: return &FreeSerifBold9pt7b;
            case 12: return &FreeSerifBold12pt7b;
            // Retro/Pixel fonts
            case 13: return &Org_01;
            case 14: return &Picopixel;
            case 15: return &TomThumb;
            // Custom fonts
            case 16: return &Aquire_BW0ox12pt7b;
            case 17: return &AquireBold_8Ma6012pt7b;
            case 18: return &AquireLight_YzE0o12pt7b;
            default: return &FreeSansBold12pt7b; // Default to 12pt bold
        }
    }

    // Helper function to get text size for font ID
    uint8_t getTextSizeForFont(int fontId) {
        // Default font and retro fonts use larger scaling
        if (fontId == 0) return 2;  // Default 5x7 @ 2x
        if (fontId >= 13 && fontId <= 15) return 3;  // Retro fonts @ 3x (they're very small)
        return 1;  // All other fonts @ 1x
    }

    void handleClient(TimerDisplay& timerDisplay) {
        // Update mDNS responder to keep hostname resolution alive
        updateMDNS();
        
        if (server == nullptr) return;

        EthernetClient client = server->available();
        if (client) {
            String currentLine = "";
            String requestType = "";
            String requestPath = "";
            String postData = "";
            bool isPost = false;
            int contentLength = 0;

            // Read HTTP request
            while (client.connected()) {
                if (client.available()) {
                    char c = client.read();
                    
                    if (c == '\n') {
                        if (currentLine.length() == 0) {
                            // End of headers, read POST data if needed
                            if (isPost && contentLength > 0) {
                                postData.reserve(contentLength);
                                for (int i = 0; i < contentLength; i++) {
                                    if (client.available()) {
                                        postData += (char)client.read();
                                    }
                                }
                            }
                            break;
                        } else {
                            // Parse request line
                            if (requestType == "") {
                                int firstSpace = currentLine.indexOf(' ');
                                int secondSpace = currentLine.indexOf(' ', firstSpace + 1);
                                if (firstSpace > 0 && secondSpace > firstSpace) {
                                    requestType = currentLine.substring(0, firstSpace);
                                    requestPath = currentLine.substring(firstSpace + 1, secondSpace);
                                    isPost = (requestType == "POST");
                                }
                            }
                            // Check for Content-Length header
                            if (currentLine.startsWith("Content-Length: ")) {
                                contentLength = currentLine.substring(16).toInt();
                            }
                            currentLine = "";
                        }
                    } else if (c != '\r') {
                        currentLine += c;
                    }
                }
            }

            // Handle different endpoints
            if (requestPath == "/" || requestPath.startsWith("/?")) {
                // Serve web page
                Serial.println("Client connected - serving web page");
                client.println("HTTP/1.1 200 OK");
                client.println("Content-Type: text/html");
                client.println("Connection: close");
                client.println();
                
                // Send HTML in chunks using F() to save RAM
                client.print(F("<!DOCTYPE html><html lang='en'><head><meta charset='UTF-8'>"));
                client.print(F("<meta name='viewport' content='width=device-width, initial-scale=1.0'>"));
                client.print(F("<title>Arena Timer Control</title><style>"));
                client.print(F("body{font-family:Arial,sans-serif;margin:0;padding:20px;"));
                client.print(F("background:linear-gradient(135deg,#667eea 0%,#764ba2 100%);"));
                client.print(F("min-height:100vh}"));
                client.print(F(".container{background:white;border-radius:10px;padding:30px;"));
                client.print(F("box-shadow:0 10px 40px rgba(0,0,0,0.2);max-width:1400px;margin:0 auto}"));
                client.print(F("h1{text-align:center;color:#333;margin-bottom:30px}"));
                client.print(F(".grid-container{display:grid;grid-template-columns:repeat(3,1fr);"));
                client.print(F("gap:20px;margin-top:20px}"));
                client.print(F("@media (max-width:1200px){.grid-container{grid-template-columns:1fr}}"));
                client.print(F(".section{margin-bottom:25px;padding:20px;background:#f5f5f5;"));
                client.print(F("border-radius:8px}.section h2{margin-top:0;color:#667eea;font-size:18px}"));
                client.print(F(".controls{display:grid;grid-template-columns:1fr 1fr;gap:10px;margin-bottom:15px}"));
                client.print(F("button{padding:15px 20px;border:none;border-radius:6px;font-size:16px;"));
                client.print(F("cursor:pointer;transition:all 0.3s;font-weight:bold}"));
                client.print(F(".btn-start{background:#4CAF50;color:white;grid-column:1/-1}"));
                client.print(F(".btn-start:hover{background:#45a049}"));
                client.print(F(".btn-pause{background:#FF9800;color:white}"));
                client.print(F(".btn-pause:hover{background:#e68900}"));
                client.print(F(".btn-reset{background:#f44336;color:white}"));
                client.print(F(".btn-reset:hover{background:#da190b}"));
                client.print(F(".form-group{margin-bottom:15px}"));
                client.print(F("label{display:block;margin-bottom:5px;color:#555;font-weight:bold}"));
                client.print(F("input[type='number'],input[type='color'],select{width:100%;padding:10px;"));
                client.print(F("border:2px solid #ddd;border-radius:6px;font-size:14px;box-sizing:border-box}"));
                client.print(F("input[type='number']:focus,input[type='color']:focus,select:focus{"));
                client.print(F("border-color:#667eea;outline:none}"));
                client.print(F("input[type='color']{height:45px;cursor:pointer;border-radius:6px;min-width:60px}"));
                client.print(F(".threshold-list{margin-bottom:15px}"));
                client.print(F(".threshold-item{display:flex;align-items:center;gap:10px;"));
                client.print(F("margin-bottom:10px;padding:12px;background:white;border-radius:8px;"));
                client.print(F("border-left:4px solid #667eea;box-shadow:0 2px 4px rgba(0,0,0,0.05)}"));
                client.print(F(".threshold-item .time-inputs{display:flex;gap:5px;align-items:center;"));
                client.print(F("flex:1;white-space:nowrap}"));
                client.print(F(".threshold-item .when-label{color:#666;font-weight:500;white-space:nowrap}"));
                client.print(F(".threshold-item input[type='number']{width:60px;padding:8px;text-align:center;"));
                client.print(F("font-size:16px;font-weight:bold;flex-shrink:0}"));
                client.print(F(".threshold-item .time-label{font-size:12px;color:#999;font-weight:normal}"));
                client.print(F(".threshold-item .arrow{color:#667eea;font-size:20px;margin:0 8px}"));
                client.print(F(".threshold-default{display:flex;align-items:center;gap:10px;"));
                client.print(F("padding:12px;background:white;border-radius:8px;"));
                client.print(F("border-left:4px solid #667eea;box-shadow:0 2px 4px rgba(0,0,0,0.05);margin-bottom:10px}"));
                client.print(F(".threshold-default .label{flex:1;color:#666;font-weight:500}"));
                client.print(F(".duration-card{padding:20px;background:white;border-radius:8px;"));
                client.print(F("box-shadow:0 2px 4px rgba(0,0,0,0.05);margin-top:15px}"));
                client.print(F(".duration-inputs{display:flex;gap:8px;align-items:center;margin-top:10px}"));
                client.print(F(".duration-inputs input{width:80px;text-align:center;font-size:16px;font-weight:bold}"));
                client.print(F(".duration-inputs span{color:#666;font-size:14px}"));
                client.print(F(".btn-remove{background:#ff5252;color:white;padding:8px 12px;border:none;"));
                client.print(F("border-radius:6px;cursor:pointer;font-size:14px;font-weight:bold;"));
                client.print(F("transition:background 0.2s}"));
                client.print(F(".btn-remove:hover{background:#ff1744}"));
                client.print(F(".btn-add{background:#4CAF50;color:white;padding:12px;border:none;"));
                client.print(F("border-radius:8px;cursor:pointer;width:100%;font-size:14px;font-weight:bold;"));
                client.print(F("margin-bottom:15px;transition:background 0.2s}"));
                client.print(F(".btn-add:hover{background:#45a049}"));
                client.print(F(".status{padding:10px;margin-top:15px;border-radius:6px;"));
                client.print(F("text-align:center;font-weight:bold;display:none}"));
                client.print(F(".status.show{display:block}"));
                client.print(F(".status.success{background:#d4edda;color:#155724}"));
                client.print(F(".status.error{background:#f8d7da;color:#721c24}"));
                client.print(F("</style></head><body><div class='container'>"));
                client.print(F("<h1>⏱️ Arena Timer Control</h1>"));
                client.print(F("<div class='grid-container'>"));
                
                // Column 1: Timer Controls & Duration
                client.print(F("<div class='grid-column'>"));
                client.print(F("<div class='section'><h2>🎮 Timer Controls</h2><div class='controls'>"));
                client.print(F("<button id='startBtn' class='btn-start' onclick='sendCommand(\"start\")'>▶️ Start</button>"));
                client.print(F("<button class='btn-pause' onclick='sendCommand(\"pause\")'>⏸️ Pause</button>"));
                client.print(F("<button class='btn-reset' onclick='sendCommand(\"reset\")'>🔄 Reset</button>"));
                client.print(F("</div></div>"));
                client.print(F("<div class='section'><h2>⏲️ Timer Duration</h2>"));
                client.print(F("<div class='duration-inputs'>"));
                client.print(F("<input type='number' id='durationMin' value='3' min='0' max='60'>"));
                client.print(F("<span>min</span>"));
                client.print(F("<input type='number' id='durationSec' value='0' min='0' max='59'>"));
                client.print(F("<span>sec</span></div></div>"));
                client.print(F("</div>"));  // End column 1
                
                // Column 2: Color Thresholds & Font Selection
                client.print(F("<div class='grid-column'>"));
                client.print(F("<div class='section'><h2>⏱️ Color Thresholds</h2>"));
                client.print(F("<p style='font-size:13px;color:#666;margin-bottom:20px'>"));
                client.print(F("The timer automatically changes color as time runs out</p>"));
                client.print(F("<div id='thresholds' class='threshold-list'></div>"));
                client.print(F("<button class='btn-add' onclick='addThreshold()'>+ Add Threshold</button>"));
                client.print(F("<p style='font-size:13px;color:#666;margin:15px 0 10px 0;font-style:italic'>"));
                client.print(F("When no threshold matches:</p>"));
                client.print(F("<div class='threshold-default'>"));
                client.print(F("<span class='label'>Default Color</span>"));
                client.print(F("<span class='arrow'>→</span>"));
                client.print(F("<input type='color' id='defaultColor' value='#00FF00'>"));
                client.print(F("</div></div>"));
                
                client.print(F("<div class='section'><h2>🔤 Font Selection</h2>"));
                client.print(F("<div class='duration-card'>"));
                client.print(F("<label for='fontSelect' style='margin-bottom:10px'>Display Font:</label>"));
                client.print(F("<select id='fontSelect' style='font-size:16px'>"));
                client.print(F("<option value='0'>Adafruit Default (5x7 @ 2x scale)</option>"));
                client.print(F("<optgroup label='Sans-Serif'>"));
                client.print(F("<option value='1'>Sans 9pt</option>"));
                client.print(F("<option value='2'>Sans 12pt</option>"));
                client.print(F("<option value='3'>Sans Bold 9pt</option>"));
                client.print(F("<option value='4' selected>Sans Bold 12pt (default)</option>"));
                client.print(F("</optgroup>"));
                client.print(F("<optgroup label='Monospace'>"));
                client.print(F("<option value='5'>Mono 9pt</option>"));
                client.print(F("<option value='6'>Mono 12pt</option>"));
                client.print(F("<option value='7'>Mono Bold 9pt</option>"));
                client.print(F("<option value='8'>Mono Bold 12pt</option>"));
                client.print(F("</optgroup>"));
                client.print(F("<optgroup label='Serif'>"));
                client.print(F("<option value='9'>Serif 9pt</option>"));
                client.print(F("<option value='10'>Serif 12pt</option>"));
                client.print(F("<option value='11'>Serif Bold 9pt</option>"));
                client.print(F("<option value='12'>Serif Bold 12pt</option>"));
                client.print(F("</optgroup>"));
                client.print(F("<optgroup label='Retro/Pixel'>"));
                client.print(F("<option value='13'>Org_01 (Retro @ 3x)</option>"));
                client.print(F("<option value='14'>Picopixel (Tiny @ 3x)</option>"));
                client.print(F("<option value='15'>TomThumb (Pixel @ 3x)</option>"));
                client.print(F("</optgroup>"));
                client.print(F("<optgroup label='Custom Fonts'>"));
                client.print(F("<option value='16'>Aquire (12pt)</option>"));
                client.print(F("<option value='17'>Aquire Bold (12pt)</option>"));
                client.print(F("<option value='18'>Aquire Light (12pt)</option>"));
                client.print(F("</optgroup>"));
                client.print(F("</select>"));
                client.print(F("<label for='letterSpacing' style='margin-top:15px;margin-bottom:5px'>Character Spacing:</label>"));
                client.print(F("<div style='display:flex;align-items:center;gap:10px'>"));
                client.print(F("<input type='range' id='letterSpacing' min='-2' max='5' value='3' style='flex:1'>"));
                client.print(F("<span id='spacingValue' style='min-width:30px;text-align:center'>3</span>"));
                client.print(F("</div></div></div></div>"));  // End duration-card, Font Selection section, and column 2
                
                // Column 3: WebSocket Connection
                client.print(F("<div class='grid-column'>"));
                client.print(F("<div class='section'><h2>🔌 WebSocket Connection</h2>"));
                client.print(F("<p style='font-size:13px;color:#666;margin-bottom:15px'>"));
                client.print(F("Connect to FightTimer or other WebSocket timer server</p>"));
                client.print(F("<div class='form-group'><label>Server Host / IP:</label>"));
                client.print(F("<input type='text' id='wsHost' placeholder='192.168.1.100' value=''>"));
                client.print(F("</div><div class='form-group'><label>Port:</label>"));
                client.print(F("<input type='number' id='wsPort' value='8765' min='1' max='65535'>"));
                client.print(F("</div><div class='form-group'><label>Path:</label>"));
                client.print(F("<input type='text' id='wsPath' placeholder='/socket.io/' value='/socket.io/'>"));
                client.print(F("</div><div style='display:flex;gap:10px'>"));
                client.print(F("<button class='btn-start' onclick='connectWebSocket()' style='flex:1'>"));
                client.print(F("🔗 Connect</button>"));
                client.print(F("<button class='btn-reset' onclick='disconnectWebSocket()' style='flex:1'>"));
                client.print(F("❌ Disconnect</button></div>"));
                client.print(F("<div id='wsStatus' style='margin-top:15px;padding:10px;background:white;"));
                client.print(F("border-radius:6px;font-size:14px;text-align:center'>"));
                client.print(F("<span style='color:#888'>Not connected</span></div>"));
                client.print(F("</div></div>"));  // End column 3
                
                client.print(F("</div>"));  // End grid-container
                
                client.print(F("<button class='btn-start' onclick='applySettings()' style='margin-top:20px;width:100%'>"));
                client.print(F("✓ Apply All Settings</button>"));
                
                client.print(F("<div id='status' class='status'></div></div>"));  // End container
                client.print(F("<script>"));
                client.print(F("let thresholds=[];"));
                client.print(F("function showStatus(message,isSuccess){"));
                client.print(F("const status=document.getElementById('status');"));
                client.print(F("status.textContent=message;"));
                client.print(F("status.className='status show '+(isSuccess?'success':'error');"));
                client.print(F("setTimeout(()=>{status.classList.remove('show')},3000)}"));
                client.print(F("function updateButtonState(){"));
                client.print(F("fetch('/api/status').then(r=>r.json()).then(data=>{"));
                client.print(F("const btn=document.getElementById('startBtn');"));
                client.print(F("if(data.isPaused){btn.textContent='▶️ Resume';}"));
                client.print(F("else{btn.textContent='▶️ Start';}"));
                client.print(F("}).catch(err=>console.log('Status check failed'));}"));
                client.print(F("function loadThresholds(){"));
                client.print(F("fetch('/api/thresholds').then(r=>r.json()).then(data=>{"));
                client.print(F("thresholds=data.thresholds||[];"));
                client.print(F("if(data.defaultColor){document.getElementById('defaultColor').value=data.defaultColor;}"));
                client.print(F("renderThresholds();"));
                client.print(F("}).catch(err=>console.log('Load failed'));}"));
                client.print(F("function renderThresholds(){"));
                client.print(F("const container=document.getElementById('thresholds');"));
                client.print(F("container.innerHTML='';"));
                client.print(F("thresholds.forEach((t,i)=>{"));
                client.print(F("const div=document.createElement('div');"));
                client.print(F("div.className='threshold-item';"));
                client.print(F("const mins=Math.floor(t.seconds/60);const secs=t.seconds%60;"));
                client.print(F("div.innerHTML=`<div class='time-inputs'>"));
                client.print(F("<span class='when-label'>When ≤</span>"));
                client.print(F("<input type='number' value='${mins}' min='0' max='60' "));
                client.print(F("onchange='updateThreshold(${i},\"minutes\",this.value)'>"));
                client.print(F("<span class='time-label'>min</span>"));
                client.print(F("<input type='number' value='${secs}' min='0' max='59' "));
                client.print(F("onchange='updateThreshold(${i},\"seconds\",this.value)'>"));
                client.print(F("<span class='time-label'>sec</span></div>"));
                client.print(F("<span class='arrow'>→</span>"));
                client.print(F("<input type='color' value='${t.color}' "));
                client.print(F("onchange='updateThreshold(${i},\"color\",this.value)'>"));
                client.print(F("<button class='btn-remove' onclick='removeThreshold(${i})'>✕</button>`;"));
                client.print(F("container.appendChild(div);});}"));
                client.print(F("function addThreshold(){"));
                client.print(F("thresholds.push({seconds:60,color:'#FFFF00'});renderThresholds();}"));
                client.print(F("function removeThreshold(i){thresholds.splice(i,1);renderThresholds();}"));
                client.print(F("function updateThreshold(i,field,value){"));
                client.print(F("if(field==='minutes'){const s=thresholds[i].seconds%60;"));
                client.print(F("thresholds[i].seconds=parseInt(value)*60+s;}"));
                client.print(F("else if(field==='seconds'){const m=Math.floor(thresholds[i].seconds/60);"));
                client.print(F("thresholds[i].seconds=m*60+parseInt(value);}"));
                client.print(F("else if(field==='color'){thresholds[i].color=value;}}"));
                client.print(F("function sendCommand(cmd){"));
                client.print(F("fetch('/api',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},"));
                client.print(F("body:'action='+cmd}).then(r=>r.text()).then(data=>{showStatus(data,true);updateButtonState();})"));
                client.print(F(".catch(()=>showStatus('Error sending command',false))}"));
                client.print(F("function applySettings(){"));
                client.print(F("const durationMin=parseInt(document.getElementById('durationMin').value)||0;"));
                client.print(F("const durationSec=parseInt(document.getElementById('durationSec').value)||0;"));
                client.print(F("const duration=durationMin*60+durationSec;"));
                client.print(F("const defaultColor=document.getElementById('defaultColor').value;"));
                client.print(F("const font=document.getElementById('fontSelect').value;"));
                client.print(F("const spacing=document.getElementById('letterSpacing').value;"));
                client.print(F("let params='action=settings&duration='+duration+'&font='+font+'&spacing='+spacing;"));
                client.print(F("fetch('/api',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},"));
                client.print(F("body:params}).then(()=>{"));
                client.print(F("const thresholdData=thresholds.map(t=>t.seconds+':'+t.color).join('|');"));
                client.print(F("const thresholdParams='thresholds='+encodeURIComponent(thresholdData)+'&default='+encodeURIComponent(defaultColor);"));
                client.print(F("return fetch('/api/thresholds',{method:'POST',"));
                client.print(F("headers:{'Content-Type':'application/x-www-form-urlencoded'},body:thresholdParams});"));
                client.print(F("}).then(r=>r.text()).then(data=>showStatus('Settings applied',true))"));
                client.print(F(".catch(()=>showStatus('Error applying settings',false))}"));
                client.print(F("document.getElementById('letterSpacing').addEventListener('input',function(){"));
                client.print(F("document.getElementById('spacingValue').textContent=this.value;});"));
                
                // WebSocket functions
                client.print(F("function updateWebSocketStatus(){"));
                client.print(F("fetch('/api/websocket/status').then(r=>r.json()).then(data=>{"));
                client.print(F("const wsStatus=document.getElementById('wsStatus');"));
                client.print(F("if(data.connected){"));
                client.print(F("wsStatus.innerHTML='<span style=\"color:#4CAF50\">✅ Connected to '+data.url+'</span>';}"));
                client.print(F("else{wsStatus.innerHTML='<span style=\"color:#888\">'+data.status+'</span>';}"));
                client.print(F("}).catch(()=>{});}"));
                client.print(F("function connectWebSocket(){"));
                client.print(F("const host=document.getElementById('wsHost').value;"));
                client.print(F("const port=document.getElementById('wsPort').value;"));
                client.print(F("const path=document.getElementById('wsPath').value;"));
                client.print(F("if(!host){showStatus('Please enter a host',false);return;}"));
                client.print(F("const params=new URLSearchParams({host:host,port:port,path:path});"));
                client.print(F("fetch('/api/websocket/connect',{method:'POST',body:params})"));
                client.print(F(".then(r=>r.json()).then(data=>{"));
                client.print(F("showStatus(data.message,data.status==='success');"));
                client.print(F("setTimeout(updateWebSocketStatus,1000);"));
                client.print(F("}).catch(()=>showStatus('Connection failed',false));}"));
                client.print(F("function disconnectWebSocket(){"));
                client.print(F("fetch('/api/websocket/disconnect',{method:'POST'})"));
                client.print(F(".then(r=>r.json()).then(data=>{"));
                client.print(F("showStatus(data.message,data.status==='success');"));
                client.print(F("setTimeout(updateWebSocketStatus,1000);"));
                client.print(F("}).catch(()=>showStatus('Disconnect failed',false));}"));
                
                client.print(F("setInterval(updateButtonState,2000);updateButtonState();loadThresholds();"));
                client.print(F("setInterval(updateWebSocketStatus,3000);updateWebSocketStatus();"));
                client.print(F("</script></body></html>"));
                
                Serial.println("Web page sent");
            } else if (requestPath == "/api/websocket/status") {
                // Return WebSocket connection status
                String status = "{\"connected\":";
                status += (wsClient && wsClient->isConnected()) ? "true" : "false";
                status += ",\"status\":\"";
                status += wsClient ? wsClient->getStatus() : "Not initialized";
                status += "\",\"url\":\"";
                status += wsClient ? wsClient->getServerUrl() : "";
                status += "\"}";
                sendHTTPResponse(client, 200, "application/json", status);
                
            } else if (requestPath == "/api/websocket/connect" && isPost) {
                // Connect to WebSocket server
                // Expected format: host=192.168.1.100&port=8765&path=/socket.io/
                
                if (!wsClient) {
                    sendHTTPResponse(client, 500, "application/json", 
                        "{\"status\":\"error\",\"message\":\"WebSocket client not initialized\"}");
                } else {
                    String host = "";
                    uint16_t port = 8765;
                    String path = "/socket.io/";
                    
                    // Parse host
                    int hostStart = postData.indexOf("host=");
                    if (hostStart >= 0) {
                        int hostEnd = postData.indexOf('&', hostStart);
                        if (hostEnd < 0) hostEnd = postData.length();
                        host = postData.substring(hostStart + 5, hostEnd);
                        host.trim();
                    }
                    
                    // Parse port
                    int portStart = postData.indexOf("port=");
                    if (portStart >= 0) {
                        int portEnd = postData.indexOf('&', portStart);
                        if (portEnd < 0) portEnd = postData.length();
                        port = postData.substring(portStart + 5, portEnd).toInt();
                    }
                    
                    // Parse path
                    int pathStart = postData.indexOf("path=");
                    if (pathStart >= 0) {
                        int pathEnd = postData.indexOf('&', pathStart);
                        if (pathEnd < 0) pathEnd = postData.length();
                        path = postData.substring(pathStart + 5, pathEnd);
                        path.replace("%2F", "/");  // URL decode /
                        path.replace("%3F", "?");  // URL decode ?
                        path.replace("%3D", "=");  // URL decode =
                        path.replace("%26", "&");  // URL decode &
                        path.trim();
                    }
                    
                    if (host.length() == 0) {
                        sendHTTPResponse(client, 400, "application/json", 
                            "{\"status\":\"error\",\"message\":\"Host parameter required\"}");
                    } else {
                        Serial.print("Connecting to WebSocket: ");
                        Serial.print(host);
                        Serial.print(":");
                        Serial.print(port);
                        Serial.println(path);
                        
                        bool connected = wsClient->connect(host.c_str(), port, path.c_str());
                        
                        if (connected) {
                            sendHTTPResponse(client, 200, "application/json", 
                                "{\"status\":\"success\",\"message\":\"Connected to WebSocket server\"}");
                        } else {
                            sendHTTPResponse(client, 500, "application/json", 
                                "{\"status\":\"error\",\"message\":\"Failed to connect to WebSocket server\"}");
                        }
                    }
                }
                
            } else if (requestPath == "/api/websocket/disconnect" && isPost) {
                // Disconnect from WebSocket server
                if (!wsClient) {
                    sendHTTPResponse(client, 500, "application/json", 
                        "{\"status\":\"error\",\"message\":\"WebSocket client not initialized\"}");
                } else {
                    wsClient->disconnect();
                    sendHTTPResponse(client, 200, "application/json", 
                        "{\"status\":\"success\",\"message\":\"Disconnected from WebSocket server\"}");
                }
                
            } else if (requestPath == "/api/status") {
                // Return current timer status as JSON (silent - polled frequently)
                String status = "{\"isPaused\":";
                status += timerDisplay.getTimer().isPaused() ? "true" : "false";
                status += ",\"isRunning\":";
                status += timerDisplay.getTimer().isRunning() ? "true" : "false";
                status += "}";
                sendHTTPResponse(client, 200, "application/json", status);
            } else if (requestPath == "/api/thresholds") {
                if (isPost) {
                    // Update thresholds from POST data
                    // Expected format: thresholds=[{"seconds":120,"color":"#FFFF00"},{"seconds":60,"color":"#FF0000"}]&default=#0000FF
                    
                    Serial.println("Updating thresholds...");
                    Serial.println(postData);
                    
                    // Parse default color
                    String defaultColorStr = "";
                    int defaultStart = postData.indexOf("default=");
                    if (defaultStart >= 0) {
                        int defaultEnd = postData.indexOf('&', defaultStart);
                        if (defaultEnd < 0) defaultEnd = postData.length();
                        defaultColorStr = postData.substring(defaultStart + 8, defaultEnd);
                        defaultColorStr.replace("%23", "#");
                        
                        uint8_t r, g, b;
                        parseColor(defaultColorStr, r, g, b);
                        timerDisplay.setDefaultColor(r, g, b);
                        Serial.print("Set default color: ");
                        Serial.println(defaultColorStr);
                    }
                    
                    // Clear existing thresholds
                    timerDisplay.clearColorThresholds();
                    Serial.println("Cleared thresholds");
                    
                    // Parse and add new thresholds
                    // Simple parser for threshold data (seconds:color pairs separated by |)
                    // Format: thresholds=120:#FFFF00|60:#FF0000
                    int thresholdsStart = postData.indexOf("thresholds=");
                    if (thresholdsStart >= 0) {
                        int thresholdsEnd = postData.indexOf('&', thresholdsStart);
                        if (thresholdsEnd < 0) thresholdsEnd = postData.length();
                        String thresholdsStr = postData.substring(thresholdsStart + 11, thresholdsEnd);
                        
                        // URL decode the threshold string
                        thresholdsStr = urlDecode(thresholdsStr);
                        
                        Serial.print("Threshold string: ");
                        Serial.println(thresholdsStr);
                        
                        // Parse each threshold (format: seconds:color)
                        int start = 0;
                        int count = 0;
                        while (start < thresholdsStr.length()) {
                            int pipePos = thresholdsStr.indexOf('|', start);
                            if (pipePos < 0) pipePos = thresholdsStr.length();
                            
                            String entry = thresholdsStr.substring(start, pipePos);
                            int colonPos = entry.indexOf(':');
                            if (colonPos > 0) {
                                unsigned int seconds = entry.substring(0, colonPos).toInt();
                                String color = entry.substring(colonPos + 1);
                                color.replace("%23", "#");
                                
                                uint8_t r, g, b;
                                parseColor(color, r, g, b);
                                timerDisplay.addColorThreshold(seconds, r, g, b);
                                count++;
                                Serial.print("Added threshold: ");
                                Serial.print(seconds);
                                Serial.print("s -> ");
                                Serial.println(color);
                            }
                            
                            start = pipePos + 1;
                        }
                        Serial.print("Total thresholds added: ");
                        Serial.println(count);
                    }
                    
                    sendHTTPResponse(client, 200, "text/plain", "Thresholds updated");
                } else {
                    // GET - Return current thresholds as JSON
                    size_t count;
                    const TimerDisplay::ColorThreshold* thresholds = timerDisplay.getColorThresholds(count);
                    
                    // Get default color
                    uint8_t def_r, def_g, def_b;
                    timerDisplay.getDefaultColor(def_r, def_g, def_b);
                    
                    String json = "{\"thresholds\":[";
                    for (size_t i = 0; i < count; i++) {
                        if (i > 0) json += ",";
                        json += "{\"seconds\":";
                        json += String(thresholds[i].seconds);
                        json += ",\"color\":\"#";
                        // Convert RGB to hex
                        char hex[7];
                        snprintf(hex, sizeof(hex), "%02X%02X%02X", thresholds[i].r, thresholds[i].g, thresholds[i].b);
                        json += hex;
                        json += "\"}";
                    }
                    json += "],\"defaultColor\":\"#";
                    // Convert default color to hex
                    char def_hex[7];
                    snprintf(def_hex, sizeof(def_hex), "%02X%02X%02X", def_r, def_g, def_b);
                    json += def_hex;
                    json += "\"}";
                    
                    sendHTTPResponse(client, 200, "application/json", json);
                }
            } else if (requestPath == "/api" && isPost) {
                // Handle API requests
                Serial.print("API request: ");
                Serial.println(postData);
                
                // Parse action parameter
                String action = "";
                int actionStart = postData.indexOf("action=");
                if (actionStart >= 0) {
                    int actionEnd = postData.indexOf('&', actionStart);
                    if (actionEnd < 0) actionEnd = postData.length();
                    action = postData.substring(actionStart + 7, actionEnd);
                }
                
                if (action == "start") {
                    timerDisplay.getTimer().start();
                    sendHTTPResponse(client, 200, "text/plain", "Timer started");
                } else if (action == "pause") {
                    timerDisplay.getTimer().stop();
                    sendHTTPResponse(client, 200, "text/plain", "Timer paused");
                } else if (action == "reset") {
                    timerDisplay.getTimer().reset();
                    sendHTTPResponse(client, 200, "text/plain", "Timer reset");
                } else if (action == "settings") {
                    // Parse duration setting
                    String durationStr = "";
                    int durationStart = postData.indexOf("duration=");
                    if (durationStart >= 0) {
                        int durationEnd = postData.indexOf('&', durationStart);
                        if (durationEnd < 0) durationEnd = postData.length();
                        durationStr = postData.substring(durationStart + 9, durationEnd);
                    }
                    
                    // Parse font setting
                    String fontStr = "";
                    int fontStart = postData.indexOf("font=");
                    if (fontStart >= 0) {
                        int fontEnd = postData.indexOf('&', fontStart);
                        if (fontEnd < 0) fontEnd = postData.length();
                        fontStr = postData.substring(fontStart + 5, fontEnd);
                    }
                    
                    // Parse spacing parameter
                    String spacingStr = "";
                    int spacingStart = postData.indexOf("spacing=");
                    if (spacingStart >= 0) {
                        int spacingEnd = postData.indexOf('&', spacingStart);
                        if (spacingEnd < 0) spacingEnd = postData.length();
                        spacingStr = postData.substring(spacingStart + 8, spacingEnd);
                    }
                    
                    Serial.print("Settings - Duration: ");
                    Serial.print(durationStr);
                    Serial.print(", Font: ");
                    Serial.print(fontStr);
                    Serial.print(", Spacing: ");
                    Serial.println(spacingStr);
                    
                    // Apply duration setting
                    if (durationStr.length() > 0) {
                        int totalSeconds = durationStr.toInt();
                        if (totalSeconds > 0 && totalSeconds <= 3600) {  // Max 60 minutes
                            unsigned int minutes = totalSeconds / 60;
                            unsigned int seconds = totalSeconds % 60;
                            Timer::Components newDuration = {minutes, seconds, 0};
                            timerDisplay.getTimer().setDuration(newDuration);
                            timerDisplay.getTimer().reset();  // Reset to apply new duration
                        }
                    }
                    
                    // Apply font setting
                    if (fontStr.length() > 0) {
                        int fontId = fontStr.toInt();
                        const GFXfont* font = getFontById(fontId);
                        uint8_t textSize = getTextSizeForFont(fontId);
                        timerDisplay.setFont(font);
                        timerDisplay.setTextSize(textSize);
                        
                        Serial.print("Applied font ID: ");
                        Serial.print(fontId);
                        Serial.print(" with text size: ");
                        Serial.println(textSize);
                    }
                    
                    // Apply letter spacing setting
                    if (spacingStr.length() > 0) {
                        int8_t spacing = spacingStr.toInt();
                        timerDisplay.setLetterSpacing(spacing);
                        Serial.print("Applied letter spacing: ");
                        Serial.println(spacing);
                    }
                    
                    sendHTTPResponse(client, 200, "text/plain", "Settings applied");
                } else {
                    sendHTTPResponse(client, 400, "text/plain", "Invalid action");
                }
            } else {
                sendHTTPResponse(client, 404, "text/plain", "Not Found");
            }

            delay(1);
            client.stop();
            
            // Only log disconnect for meaningful requests (not status polling)
            if (requestPath != "/api/status") {
                Serial.println("Client disconnected");
            }
        }
    }
}
