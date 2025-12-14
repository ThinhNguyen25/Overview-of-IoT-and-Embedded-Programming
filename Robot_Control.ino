#include <WiFi.h>
#include <WebServer.h>
#include <ArduinoJson.h>

// ========== CẤU HÌNH WIFI AP ==========
const char* ssid = "ROBOT_CONTROL";
const char* password = "12345678";

WebServer server(80);

// ========== BIẾN DỮ LIỆU ==========
String sensorJson = "{}";
int sensorValues[5] = {0};
int distanceVal = 0;
int errorVal = 0;
int leftSpeed = 0;
int rightSpeed = 0;
bool avoiding = false;
int mode = 0; // 0=Auto, 1=Manual
char lastCmd = 'S';
int speedVal = 150;

// ========== UART VỚI ARDUINO ==========
#define RXD2 16
#define TXD2 17

void setup() {
  // Debug Serial
  Serial.begin(115200);
  
  // UART với Arduino
  Serial2.begin(115200, SERIAL_8N1, RXD2, TXD2);
  delay(1000);
  
  // Tạo WiFi Access Point
  WiFi.softAP(ssid, password);
  
  Serial.println("\n✅ WiFi AP Ready!");
  Serial.print("SSID: "); Serial.println(ssid);
  Serial.print("IP: "); Serial.println(WiFi.softAPIP());
  
  // Cấu hình Web Server Routes
  server.on("/", handleRoot);
  server.on("/data", handleData);
  server.on("/control", handleControl);
  server.on("/mode", handleMode);
  server.on("/style.css", handleCSS);
  server.on("/script.js", handleJS);
  
  server.begin();
  Serial.println("✅ Web Server Started!");
}

void loop() {
  server.handleClient();
  readArduinoData();
  delay(10);
}

// ========== ĐỌC DỮ LIỆU TỪ ARDUINO ==========
void readArduinoData() {
  if (Serial2.available()) {
    String json = Serial2.readStringUntil('\n');
    json.trim();
    
    if (json.startsWith("{")) {
      sensorJson = json;
      parseSensorData(json);
    }
  }
}

void parseSensorData(String json) {
  StaticJsonDocument<512> doc;
  DeserializationError err = deserializeJson(doc, json);
  
  if (!err) {
    // Cảm biến line
    JsonArray s = doc["s"];
    for (int i = 0; i < 5 && i < s.size(); i++) {
      sensorValues[i] = s[i];
    }
    
    // Các giá trị khác
    distanceVal = doc["d"] | 0;
    errorVal = doc["e"] | 0;
    leftSpeed = doc["l"] | 0;
    rightSpeed = doc["r"] | 0;
    avoiding = doc["a"] | false;
    mode = doc["m"] | 0;
    lastCmd = doc["c"] | 'S';
    speedVal = doc["sp"] | 150;
  }
}

// ========== GỬI LỆNH ĐIỀU KHIỂN ==========
void sendCommand(String cmd) {
  Serial2.println(cmd);
  Serial.println("Sent: " + cmd);
}

// ========== WEB SERVER HANDLERS ==========
void handleRoot() {
  String html = R"=====(
<!DOCTYPE html>
<html lang="vi">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>🤖 Robot Line Follower Control</title>
    <link rel="stylesheet" href="/style.css">
    <link rel="stylesheet" href="https://cdnjs.cloudflare.com/ajax/libs/font-awesome/6.4.0/css/all.min.css">
    <script src="/script.js"></script>
</head>
<body>
    <div class="container">
        <!-- HEADER -->
        <header>
            <h1><i class="fas fa-robot"></i> ROBOT LINE FOLLOWER CONTROL</h1>
            <div class="connection">
                <span class="status connected"><i class="fas fa-wifi"></i> Connected</span>
                <span class="ip">IP: )=====" + WiFi.softAPIP().toString() + R"=====(</span>
            </div>
        </header>

        <div class="main-content">
            <!-- LEFT PANEL: HIỂN THỊ DỮ LIỆU -->
            <div class="data-panel">
                <div class="card">
                    <h2><i class="fas fa-satellite-dish"></i> CẢM BIẾN LINE</h2>
                    <div class="sensor-grid">
                        <div class="sensor" id="s1"><span>S1</span><br><span class="val">TRẮNG</span></div>
                        <div class="sensor" id="s2"><span>S2</span><br><span class="val">TRẮNG</span></div>
                        <div class="sensor" id="s3"><span>S3</span><br><span class="val">TRẮNG</span></div>
                        <div class="sensor" id="s4"><span>S4</span><br><span class="val">TRẮNG</span></div>
                        <div class="sensor" id="s5"><span>S5</span><br><span class="val">TRẮNG</span></div>
                    </div>
                </div>

                <div class="card">
                    <h2><i class="fas fa-tachometer-alt"></i> THÔNG SỐ</h2>
                    <div class="stats">
                        <div class="stat-item">
                            <span class="label">Khoảng cách:</span>
                            <span class="value" id="distance">0 cm</span>
                        </div>
                        <div class="stat-item">
                            <span class="label">Lỗi Line:</span>
                            <span class="value" id="error">0</span>
                        </div>
                        <div class="stat-item">
                            <span class="label">Tốc độ trái:</span>
                            <span class="value" id="leftSpeed">0</span>
                        </div>
                        <div class="stat-item">
                            <span class="label">Tốc độ phải:</span>
                            <span class="value" id="rightSpeed">0</span>
                        </div>
                        <div class="stat-item">
                            <span class="label">Trạng thái:</span>
                            <span class="value" id="avoiding">BÌNH THƯỜNG</span>
                        </div>
                    </div>
                </div>
            </div>

            <!-- RIGHT PANEL: ĐIỀU KHIỂN -->
            <div class="control-panel">
                <div class="card">
                    <h2><i class="fas fa-cogs"></i> CHẾ ĐỘ</h2>
                    <div class="mode-selector">
                        <button class="mode-btn active" id="btnAuto" onclick="setMode('auto')">
                            <i class="fas fa-route"></i><br>AUTO
                        </button>
                        <button class="mode-btn" id="btnManual" onclick="setMode('manual')">
                            <i class="fas fa-gamepad"></i><br>MANUAL
                        </button>
                    </div>
                    <div class="current-mode">
                        <span id="currentModeText">Đang dò line tự động</span>
                    </div>
                </div>

                <div class="card">
                    <h2><i class="fas fa-gamepad"></i> ĐIỀU KHIỂN TAY</h2>
                    <div class="joystick-area">
                        <div class="joystick">
                            <button class="ctrl-btn up" onmousedown="sendCmd('F')" onmouseup="sendCmd('S')">
                                <i class="fas fa-arrow-up"></i>
                            </button>
                            <br>
                            <button class="ctrl-btn left" onmousedown="sendCmd('L')" onmouseup="sendCmd('S')">
                                <i class="fas fa-arrow-left"></i>
                            </button>
                            <button class="ctrl-btn stop" onclick="sendCmd('S')">
                                <i class="fas fa-stop"></i>
                            </button>
                            <button class="ctrl-btn right" onmousedown="sendCmd('R')" onmouseup="sendCmd('S')">
                                <i class="fas fa-arrow-right"></i>
                            </button>
                            <br>
                            <button class="ctrl-btn down" onmousedown="sendCmd('B')" onmouseup="sendCmd('S')">
                                <i class="fas fa-arrow-down"></i>
                            </button>
                        </div>
                    </div>
                </div>

                <div class="card">
                    <h2><i class="fas fa-sliders-h"></i> TỐC ĐỘ</h2>
                    <div class="speed-control">
                        <button class="speed-btn" onclick="sendCmd('-')">
                            <i class="fas fa-minus"></i> Giảm
                        </button>
                        <span class="speed-value" id="speedValue">150</span>
                        <button class="speed-btn" onclick="sendCmd('+')">
                            <i class="fas fa-plus"></i> Tăng
                        </button>
                    </div>
                    <div class="speed-bar">
                        <div class="speed-fill" id="speedFill"></div>
                    </div>
                </div>

                <div class="card">
                    <h2><i class="fas fa-info-circle"></i> TRẠNG THÁI</h2>
                    <div class="status-display">
                        <div class="status-item">
                            <span>Chế độ:</span>
                            <span id="modeStatus">AUTO</span>
                        </div>
                        <div class="status-item">
                            <span>Lệnh cuối:</span>
                            <span id="lastCommand">S</span>
                        </div>
                        <div class="status-item">
                            <span>Kết nối:</span>
                            <span class="conn-status">ONLINE</span>
                        </div>
                    </div>
                </div>
            </div>
        </div>

        <!-- FOOTER -->
        <footer>
            <p><i class="far fa-copyright"></i> 2024 - Robot Line Follower Project | Real-time Web Control</p>
        </footer>
    </div>
</body>
</html>
)=====";
  
  server.send(200, "text/html", html);
}

void handleData() {
  String json = "{";
  json += "\"sensors\":[" + 
          String(sensorValues[0]) + "," +
          String(sensorValues[1]) + "," +
          String(sensorValues[2]) + "," +
          String(sensorValues[3]) + "," +
          String(sensorValues[4]) + "],";
  json += "\"distance\":" + String(distanceVal) + ",";
  json += "\"error\":" + String(errorVal) + ",";
  json += "\"leftSpeed\":" + String(leftSpeed) + ",";
  json += "\"rightSpeed\":" + String(rightSpeed) + ",";
  json += "\"avoiding\":" + String(avoiding ? "true" : "false") + ",";
  json += "\"mode\":" + String(mode) + ",";
  json += "\"lastCommand\":\"" + String(lastCmd) + "\",";
  json += "\"speed\":" + String(speedVal);
  json += "}";
  
  server.send(200, "application/json", json);
}

void handleControl() {
  if (server.hasArg("cmd")) {
    String cmd = server.arg("cmd");
    sendCommand(cmd);
    server.send(200, "text/plain", "OK");
  }
}

void handleMode() {
  if (server.hasArg("mode")) {
    String mode = server.arg("mode");
    if (mode == "auto") {
      sendCommand("MODE_AUTO");
    } else if (mode == "manual") {
      sendCommand("MODE_MANUAL");
    }
    server.send(200, "text/plain", "Mode changed");
  }
}

void handleCSS() {
  String css = R"=====(
* {
    margin: 0;
    padding: 0;
    box-sizing: border-box;
    font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;
}

body {
    background: linear-gradient(135deg, #1a2980, #26d0ce);
    color: #333;
    min-height: 100vh;
    padding: 20px;
}

.container {
    max-width: 1400px;
    margin: 0 auto;
}

/* HEADER */
header {
    background: rgba(255, 255, 255, 0.95);
    padding: 20px 30px;
    border-radius: 15px;
    margin-bottom: 25px;
    box-shadow: 0 5px 20px rgba(0,0,0,0.15);
    display: flex;
    justify-content: space-between;
    align-items: center;
}

header h1 {
    color: #2c3e50;
    font-size: 1.8em;
}

header h1 i {
    color: #3498db;
    margin-right: 10px;
}

.connection {
    display: flex;
    gap: 20px;
    align-items: center;
}

.status {
    padding: 8px 15px;
    border-radius: 20px;
    font-weight: bold;
    font-size: 0.9em;
}

.connected {
    background: #2ecc71;
    color: white;
}

.ip {
    background: #3498db;
    color: white;
    padding: 8px 15px;
    border-radius: 20px;
    font-family: monospace;
}

/* MAIN CONTENT */
.main-content {
    display: grid;
    grid-template-columns: 1fr 1fr;
    gap: 25px;
    margin-bottom: 30px;
}

/* CARD STYLES */
.card {
    background: white;
    border-radius: 15px;
    padding: 25px;
    margin-bottom: 25px;
    box-shadow: 0 5px 15px rgba(0,0,0,0.1);
    transition: transform 0.3s ease;
}

.card:hover {
    transform: translateY(-5px);
}

.card h2 {
    color: #2c3e50;
    margin-bottom: 20px;
    font-size: 1.4em;
    display: flex;
    align-items: center;
    gap: 10px;
    border-bottom: 2px solid #eee;
    padding-bottom: 10px;
}

/* SENSOR GRID */
.sensor-grid {
    display: grid;
    grid-template-columns: repeat(5, 1fr);
    gap: 15px;
}

.sensor {
    background: #ecf0f1;
    border-radius: 10px;
    padding: 20px 10px;
    text-align: center;
    transition: all 0.3s ease;
}

.sensor.active {
    background: linear-gradient(135deg, #e74c3c, #c0392b);
    color: white;
    box-shadow: 0 5px 15px rgba(231, 76, 60, 0.4);
}

.sensor span.val {
    font-size: 0.9em;
    margin-top: 5px;
    display: block;
}

/* STATS */
.stats {
    display: flex;
    flex-direction: column;
    gap: 15px;
}

.stat-item {
    display: flex;
    justify-content: space-between;
    align-items: center;
    padding: 12px 15px;
    background: #f8f9fa;
    border-radius: 10px;
}

.stat-item .label {
    font-weight: 600;
    color: #555;
}

.stat-item .value {
    font-weight: bold;
    font-size: 1.2em;
    color: #2c3e50;
}

/* MODE SELECTOR */
.mode-selector {
    display: flex;
    gap: 20px;
    justify-content: center;
}

.mode-btn {
    flex: 1;
    padding: 20px;
    border: none;
    border-radius: 10px;
    background: #ecf0f1;
    color: #7f8c8d;
    font-weight: bold;
    font-size: 1.1em;
    cursor: pointer;
    transition: all 0.3s ease;
}

.mode-btn.active {
    background: linear-gradient(135deg, #3498db, #2980b9);
    color: white;
    box-shadow: 0 5px 15px rgba(52, 152, 219, 0.4);
}

.mode-btn i {
    font-size: 2em;
    margin-bottom: 10px;
}

.current-mode {
    text-align: center;
    margin-top: 20px;
    padding: 15px;
    background: #f8f9fa;
    border-radius: 10px;
    font-weight: bold;
    color: #2c3e50;
}

/* JOYSTICK */
.joystick-area {
    display: flex;
    justify-content: center;
}

.joystick {
    display: inline-block;
    text-align: center;
}

.ctrl-btn {
    width: 80px;
    height: 80px;
    border: none;
    border-radius: 50%;
    margin: 5px;
    font-size: 1.5em;
    cursor: pointer;
    transition: all 0.2s ease;
    display: inline-flex;
    align-items: center;
    justify-content: center;
}

.ctrl-btn:active {
    transform: scale(0.95);
}

.up { background: #2ecc71; color: white; }
.down { background: #e74c3c; color: white; }
.left { background: #f39c12; color: white; }
.right { background: #3498db; color: white; }
.stop { background: #34495e; color: white; width: 100px; }

/* SPEED CONTROL */
.speed-control {
    display: flex;
    justify-content: space-between;
    align-items: center;
    margin-bottom: 20px;
}

.speed-btn {
    padding: 12px 25px;
    background: #3498db;
    color: white;
    border: none;
    border-radius: 10px;
    cursor: pointer;
    font-weight: bold;
    transition: background 0.3s;
}

.speed-btn:hover {
    background: #2980b9;
}

.speed-value {
    font-size: 2em;
    font-weight: bold;
    color: #2c3e50;
    font-family: monospace;
}

.speed-bar {
    height: 20px;
    background: #ecf0f1;
    border-radius: 10px;
    overflow: hidden;
}

.speed-fill {
    height: 100%;
    background: linear-gradient(90deg, #2ecc71, #f1c40f);
    width: 60%;
    transition: width 0.5s ease;
}

/* STATUS DISPLAY */
.status-display {
    display: flex;
    flex-direction: column;
    gap: 15px;
}

.status-item {
    display: flex;
    justify-content: space-between;
    padding: 12px;
    background: #f8f9fa;
    border-radius: 10px;
}

.conn-status {
    color: #2ecc71;
    font-weight: bold;
}

/* FOOTER */
footer {
    text-align: center;
    color: white;
    padding: 20px;
    margin-top: 30px;
    opacity: 0.8;
}

footer i {
    margin-right: 5px;
}

/* RESPONSIVE */
@media (max-width: 1100px) {
    .main-content {
        grid-template-columns: 1fr;
    }
    
    .sensor-grid {
        grid-template-columns: repeat(3, 1fr);
    }
}

@media (max-width: 768px) {
    header {
        flex-direction: column;
        gap: 15px;
        text-align: center;
    }
    
    .sensor-grid {
        grid-template-columns: repeat(2, 1fr);
    }
    
    .joystick {
        transform: scale(0.9);
    }
}
)=====";
  
  server.send(200, "text/css", css);
}

void handleJS() {
  String js = R"=====(
// Biến toàn cục
let currentMode = 'auto';
let autoUpdate = true;

// Cập nhật dữ liệu từ server
function updateData() {
    if (!autoUpdate) return;
    
    fetch('/data')
        .then(response => response.json())
        .then(data => {
            // Cập nhật cảm biến line
            for(let i = 1; i <= 5; i++) {
                let sensor = document.getElementById('s' + i);
                let value = data.sensors[i-1];
                sensor.classList.toggle('active', value === 1);
                sensor.querySelector('.val').textContent = value === 1 ? 'ĐEN' : 'TRẮNG';
            }
            
            // Cập nhật thông số
            document.getElementById('distance').textContent = data.distance + ' cm';
            document.getElementById('error').textContent = data.error;
            document.getElementById('leftSpeed').textContent = data.leftSpeed;
            document.getElementById('rightSpeed').textContent = data.rightSpeed;
            document.getElementById('avoiding').textContent = 
                data.avoiding ? 'ĐANG NÉ VẬT CẢN' : 'BÌNH THƯỜNG';
            document.getElementById('avoiding').style.color = 
                data.avoiding ? '#e74c3c' : '#2ecc71';
            
            // Cập nhật chế độ
            currentMode = data.mode === 0 ? 'auto' : 'manual';
            updateModeDisplay();
            
            // Cập nhật lệnh cuối
            document.getElementById('lastCommand').textContent = data.lastCommand;
            
            // Cập nhật tốc độ
            let speed = data.speed || 150;
            document.getElementById('speedValue').textContent = speed;
            let percent = (speed / 255) * 100;
            document.getElementById('speedFill').style.width = percent + '%';
            
            // Cập nhật trạng thái
            document.getElementById('modeStatus').textContent = 
                currentMode === 'auto' ? 'AUTO' : 'MANUAL';
            document.getElementById('modeStatus').style.color = 
                currentMode === 'auto' ? '#3498db' : '#e74c3c';
        })
        .catch(error => {
            console.error('Error fetching data:', error);
        });
}

// Gửi lệnh điều khiển
function sendCmd(cmd) {
    fetch('/control?cmd=' + cmd)
        .then(response => response.text())
        .then(data => {
            console.log('Command sent:', cmd);
        });
}

// Đổi chế độ
function setMode(mode) {
    currentMode = mode;
    
    // Cập nhật nút
    document.getElementById('btnAuto').classList.toggle('active', mode === 'auto');
    document.getElementById('btnManual').classList.toggle('active', mode === 'manual');
    
    // Gửi lệnh
    fetch('/mode?mode=' + mode)
        .then(response => response.text())
        .then(data => {
            console.log('Mode changed to:', mode);
        });
    
    // Cập nhật hiển thị
    updateModeDisplay();
}

// Cập nhật hiển thị chế độ
function updateModeDisplay() {
    let modeText = document.getElementById('currentModeText');
    if (currentMode === 'auto') {
        modeText.textContent = 'Đang dò line tự động';
        modeText.style.color = '#3498db';
    } else {
        modeText.textContent = 'Đang điều khiển thủ công';
        modeText.style.color = '#e74c3c';
    }
}

// Keyboard control
document.addEventListener('keydown', function(e) {
    if (currentMode === 'manual') {
        switch(e.key.toLowerCase()) {
            case 'w': sendCmd('F'); break;
            case 's': sendCmd('B'); break;
            case 'a': sendCmd('L'); break;
            case 'd': sendCmd('R'); break;
            case ' ': sendCmd('S'); break;
            case '+': case '=': sendCmd('+'); break;
            case '-': case '_': sendCmd('-'); break;
        }
    }
});

// Khởi động
window.onload = function() {
    // Cập nhật dữ liệu mỗi 300ms
    setInterval(updateData, 300);
    
    // Cập nhật ngay lần đầu
    updateData();
    
    console.log('Robot Control Interface Ready!');
};
)=====";
  
  server.send(200, "application/javascript", js);
}