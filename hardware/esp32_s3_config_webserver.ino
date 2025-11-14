/*
 * ESP32-S3 CONFIGURATION WEB SERVER
 * 
 * Mục đích: Tạo web server để cấu hình thiết bị qua WiFi
 * 
 * Chức năng:
 * 1. Tạo WiFi Access Point (AP) hoặc kết nối WiFi
 * 2. Web server với giao diện đẹp cho điện thoại
 * 3. Cấu hình: Device ID, User ID, Ngưỡng va chạm, Độ nhạy
 * 4. Lưu cấu hình vào EEPROM
 * 5. Kích hoạt bằng Serial command (thực tế sẽ dùng 2 nút)
 * 
 * Cách sử dụng:
 * - Gửi "START_CONFIG" qua Serial để bật web server
 * - Gửi "STOP_CONFIG" để tắt web server
 * - Kết nối WiFi: "HOP_DEN_CONFIG" (password: "12345678")
 * - Mở trình duyệt: http://192.168.4.1
 */

#include <WiFi.h>
#include <WebServer.h>
#include <EEPROM.h>
#include <ArduinoJson.h>

// ============================================
// CẤU HÌNH
// ============================================

// WiFi AP Mode
#define AP_SSID          "HOP_DEN_CONFIG"
#define AP_PASSWORD      "12345678"
#define AP_IP            IPAddress(192, 168, 4, 1)
#define AP_GATEWAY       IPAddress(192, 168, 4, 1)
#define AP_SUBNET        IPAddress(255, 255, 255, 0)

// EEPROM Settings
#define EEPROM_SIZE      512
#define EEPROM_ADDR      0

// Serial Commands
#define CMD_START_CONFIG "START_CONFIG"
#define CMD_STOP_CONFIG  "STOP_CONFIG"

// Web Server Port
#define WEB_SERVER_PORT  80

// ============================================
// CẤU TRÚC DỮ LIỆU
// ============================================

struct DeviceConfig {
  char deviceId[32];        // ID thiết bị
  char userId[32];          // ID user
  float collisionThreshold; // Ngưỡng cảnh báo va chạm (g)
  int sensitivity;          // Độ nhạy (1-5: kém nhạy -> rất nhạy)
  int gpsUpdateInterval;    // Khoảng thời gian cập nhật GPS (giây)
  int telemetryInterval;    // Khoảng thời gian gửi dữ liệu (giây)
  bool enableSMS;           // Bật/tắt SMS
  bool enableCall;          // Bật/tắt cuộc gọi
  bool enableRecording;     // Bật/tắt ghi âm
  int recordingDuration;    // Thời gian ghi âm (giây)
  bool valid;               // Flag để kiểm tra config hợp lệ
};

// ============================================
// BIẾN TOÀN CỤC
// ============================================

WebServer server(WEB_SERVER_PORT);
DeviceConfig config;
bool configModeActive = false;
String serialInput = "";

// ============================================
// SETUP
// ============================================

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("\n\n=== ESP32-S3 CONFIGURATION WEB SERVER ===");
  
  // Khởi tạo EEPROM
  EEPROM.begin(EEPROM_SIZE);
  Serial.println("EEPROM initialized");
  
  // Load cấu hình từ EEPROM
  loadConfig();
  
  // Hiển thị cấu hình hiện tại
  printConfig();
  
  Serial.println("\n=== SERIAL COMMANDS ===");
  Serial.println("Send 'START_CONFIG' to start web server");
  Serial.println("Send 'STOP_CONFIG' to stop web server");
  Serial.println("==========================================\n");
}

// ============================================
// LOOP
// ============================================

void loop() {
  // Đọc Serial input
  handleSerialInput();
  
  // Xử lý web server nếu đang bật
  if (configModeActive) {
    server.handleClient();
  }
  
  delay(10);
}

// ============================================
// XỬ LÝ SERIAL INPUT
// ============================================

void handleSerialInput() {
  while (Serial.available() > 0) {
    char c = Serial.read();
    
    if (c == '\n' || c == '\r') {
      serialInput.trim();
      
      if (serialInput == CMD_START_CONFIG) {
        startConfigMode();
      } else if (serialInput == CMD_STOP_CONFIG) {
        stopConfigMode();
      } else {
        Serial.println("Unknown command. Use 'START_CONFIG' or 'STOP_CONFIG'");
      }
      
      serialInput = "";
    } else {
      serialInput += c;
    }
  }
}

// ============================================
// KHỞI ĐỘNG CHẾ ĐỘ CẤU HÌNH
// ============================================

void startConfigMode() {
  if (configModeActive) {
    Serial.println("⚠️ Config mode already active!");
    return;
  }
  
  Serial.println("\n=== STARTING CONFIG MODE ===");
  
  // Tạo WiFi AP
  WiFi.mode(WIFI_AP);
  WiFi.softAPConfig(AP_IP, AP_GATEWAY, AP_SUBNET);
  
  if (WiFi.softAP(AP_SSID, AP_PASSWORD)) {
    Serial.println("✅ WiFi AP started");
    Serial.print("SSID: ");
    Serial.println(AP_SSID);
    Serial.print("Password: ");
    Serial.println(AP_PASSWORD);
    Serial.print("IP Address: ");
    Serial.println(AP_IP);
    Serial.println("\n📱 Connect your phone to WiFi: " + String(AP_SSID));
    Serial.println("🌐 Open browser: http://192.168.4.1");
  } else {
    Serial.println("❌ Failed to start WiFi AP!");
    return;
  }
  
  // Setup web server routes
  setupWebServer();
  
  // Start server
  server.begin();
  configModeActive = true;
  
  Serial.println("✅ Web server started on port 80");
  Serial.println("=== CONFIG MODE ACTIVE ===\n");
}

// ============================================
// DỪNG CHẾ ĐỘ CẤU HÌNH
// ============================================

void stopConfigMode() {
  if (!configModeActive) {
    Serial.println("⚠️ Config mode not active!");
    return;
  }
  
  Serial.println("\n=== STOPPING CONFIG MODE ===");
  
  server.stop();
  WiFi.softAPdisconnect(true);
  WiFi.mode(WIFI_OFF);
  
  configModeActive = false;
  
  Serial.println("✅ Config mode stopped");
  Serial.println("=== CONFIG MODE INACTIVE ===\n");
}

// ============================================
// SETUP WEB SERVER ROUTES
// ============================================

void setupWebServer() {
  // Trang chủ - Giao diện cấu hình
  server.on("/", handleRoot);
  
  // API: Lấy cấu hình
  server.on("/api/config", HTTP_GET, handleGetConfig);
  
  // API: Lưu cấu hình
  server.on("/api/config", HTTP_POST, handleSaveConfig);
  
  // API: Reset cấu hình về mặc định
  server.on("/api/reset", HTTP_POST, handleResetConfig);
  
  // 404 handler
  server.onNotFound(handleNotFound);
}

// ============================================
// HANDLERS
// ============================================

void handleRoot() {
  String html = getHTMLPage();
  server.send(200, "text/html", html);
}

void handleGetConfig() {
  DynamicJsonDocument doc(1024);
  
  doc["deviceId"] = config.deviceId;
  doc["userId"] = config.userId;
  doc["collisionThreshold"] = config.collisionThreshold;
  doc["sensitivity"] = config.sensitivity;
  doc["gpsUpdateInterval"] = config.gpsUpdateInterval;
  doc["telemetryInterval"] = config.telemetryInterval;
  doc["enableSMS"] = config.enableSMS;
  doc["enableCall"] = config.enableCall;
  doc["enableRecording"] = config.enableRecording;
  doc["recordingDuration"] = config.recordingDuration;
  
  String response;
  serializeJson(doc, response);
  
  server.send(200, "application/json", response);
}

void handleSaveConfig() {
  if (!server.hasArg("plain")) {
    server.send(400, "application/json", "{\"error\":\"No data\"}");
    return;
  }
  
  String body = server.arg("plain");
  DynamicJsonDocument doc(1024);
  
  DeserializationError error = deserializeJson(doc, body);
  if (error) {
    server.send(400, "application/json", "{\"error\":\"Invalid JSON\"}");
    return;
  }
  
  // Cập nhật cấu hình
  if (doc.containsKey("deviceId")) {
    strncpy(config.deviceId, doc["deviceId"].as<const char*>(), sizeof(config.deviceId) - 1);
    config.deviceId[sizeof(config.deviceId) - 1] = '\0';
  }
  
  if (doc.containsKey("userId")) {
    strncpy(config.userId, doc["userId"].as<const char*>(), sizeof(config.userId) - 1);
    config.userId[sizeof(config.userId) - 1] = '\0';
  }
  
  if (doc.containsKey("collisionThreshold")) {
    config.collisionThreshold = doc["collisionThreshold"].as<float>();
  }
  
  if (doc.containsKey("sensitivity")) {
    config.sensitivity = doc["sensitivity"].as<int>();
  }
  
  if (doc.containsKey("gpsUpdateInterval")) {
    config.gpsUpdateInterval = doc["gpsUpdateInterval"].as<int>();
  }
  
  if (doc.containsKey("telemetryInterval")) {
    config.telemetryInterval = doc["telemetryInterval"].as<int>();
  }
  
  if (doc.containsKey("enableSMS")) {
    config.enableSMS = doc["enableSMS"].as<bool>();
  }
  
  if (doc.containsKey("enableCall")) {
    config.enableCall = doc["enableCall"].as<bool>();
  }
  
  if (doc.containsKey("enableRecording")) {
    config.enableRecording = doc["enableRecording"].as<bool>();
  }
  
  if (doc.containsKey("recordingDuration")) {
    config.recordingDuration = doc["recordingDuration"].as<int>();
  }
  
  // Lưu vào EEPROM
  saveConfig();
  
  Serial.println("✅ Configuration saved via web interface");
  printConfig();
  
  server.send(200, "application/json", "{\"success\":true,\"message\":\"Configuration saved\"}");
}

void handleResetConfig() {
  setDefaultConfig();
  saveConfig();
  
  Serial.println("✅ Configuration reset to defaults");
  
  server.send(200, "application/json", "{\"success\":true,\"message\":\"Configuration reset\"}");
}

void handleNotFound() {
  server.send(404, "text/plain", "Not Found");
}

// ============================================
// HTML PAGE
// ============================================

String getHTMLPage() {
  String html = R"HTML(
<!DOCTYPE html>
<html lang="vi">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Hộp Đen - Cấu Hình</title>
    <style>
        * {
            margin: 0;
            padding: 0;
            box-sizing: border-box;
        }
        
        body {
            font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, Oxygen, Ubuntu, Cantarell, sans-serif;
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
            min-height: 100vh;
            padding: 20px;
        }
        
        .container {
            max-width: 500px;
            margin: 0 auto;
            background: white;
            border-radius: 20px;
            box-shadow: 0 20px 60px rgba(0,0,0,0.3);
            overflow: hidden;
        }
        
        .header {
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
            color: white;
            padding: 30px 20px;
            text-align: center;
        }
        
        .header h1 {
            font-size: 24px;
            margin-bottom: 5px;
        }
        
        .header p {
            font-size: 14px;
            opacity: 0.9;
        }
        
        .content {
            padding: 30px 20px;
        }
        
        .form-group {
            margin-bottom: 25px;
        }
        
        .form-group label {
            display: block;
            font-weight: 600;
            margin-bottom: 8px;
            color: #333;
            font-size: 14px;
        }
        
        .form-group input,
        .form-group select {
            width: 100%;
            padding: 12px;
            border: 2px solid #e0e0e0;
            border-radius: 10px;
            font-size: 16px;
            transition: all 0.3s;
        }
        
        .form-group input:focus,
        .form-group select:focus {
            outline: none;
            border-color: #667eea;
            box-shadow: 0 0 0 3px rgba(102, 126, 234, 0.1);
        }
        
        .form-group small {
            display: block;
            margin-top: 5px;
            color: #666;
            font-size: 12px;
        }
        
        .range-group {
            position: relative;
        }
        
        .range-value {
            display: inline-block;
            margin-left: 10px;
            font-weight: bold;
            color: #667eea;
            font-size: 18px;
        }
        
        .range-labels {
            display: flex;
            justify-content: space-between;
            margin-top: 5px;
            font-size: 11px;
            color: #999;
        }
        
        .checkbox-group {
            display: flex;
            align-items: center;
            gap: 10px;
        }
        
        .checkbox-group input[type="checkbox"] {
            width: 24px;
            height: 24px;
            cursor: pointer;
        }
        
        .buttons {
            display: flex;
            gap: 10px;
            margin-top: 30px;
        }
        
        .btn {
            flex: 1;
            padding: 15px;
            border: none;
            border-radius: 10px;
            font-size: 16px;
            font-weight: 600;
            cursor: pointer;
            transition: all 0.3s;
        }
        
        .btn-primary {
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
            color: white;
        }
        
        .btn-primary:hover {
            transform: translateY(-2px);
            box-shadow: 0 5px 15px rgba(102, 126, 234, 0.4);
        }
        
        .btn-secondary {
            background: #f5f5f5;
            color: #333;
        }
        
        .btn-secondary:hover {
            background: #e0e0e0;
        }
        
        .alert {
            padding: 15px;
            border-radius: 10px;
            margin-bottom: 20px;
            display: none;
        }
        
        .alert-success {
            background: #d4edda;
            color: #155724;
            border: 1px solid #c3e6cb;
        }
        
        .alert-error {
            background: #f8d7da;
            color: #721c24;
            border: 1px solid #f5c6cb;
        }
    </style>
</head>
<body>
    <div class="container">
        <div class="header">
            <h1>⚙️ Cấu Hình Thiết Bị</h1>
            <p>Hộp Đen Thông Minh</p>
        </div>
        
        <div class="content">
            <div id="alert" class="alert"></div>
            
            <form id="configForm">
                <div class="form-group">
                    <label for="deviceId">ID Thiết Bị</label>
                    <input type="text" id="deviceId" name="deviceId" placeholder="VD: ESP32_001" required>
                    <small>Mã định danh duy nhất của thiết bị</small>
                </div>
                
                <div class="form-group">
                    <label for="userId">ID Người Dùng</label>
                    <input type="text" id="userId" name="userId" placeholder="VD: user_123" required>
                    <small>ID người dùng liên kết với thiết bị</small>
                </div>
                
                <div class="form-group">
                    <label for="collisionThreshold">
                        Ngưỡng Cảnh Báo Va Chạm (g)
                        <span class="range-value" id="thresholdValue">2.0</span>
                    </label>
                    <input type="range" id="collisionThreshold" name="collisionThreshold" 
                           min="0.5" max="10.0" step="0.1" value="2.0">
                    <div class="range-labels">
                        <span>Nhẹ (0.5g)</span>
                        <span>Nặng (10g)</span>
                    </div>
                    <small>Gia tốc tối thiểu để phát hiện va chạm</small>
                </div>
                
                <div class="form-group">
                    <label for="sensitivity">
                        Độ Nhạy
                        <span class="range-value" id="sensitivityValue">3</span>
                    </label>
                    <input type="range" id="sensitivity" name="sensitivity" 
                           min="1" max="5" step="1" value="3">
                    <div class="range-labels">
                        <span>Kém nhạy (1)</span>
                        <span>Rất nhạy (5)</span>
                    </div>
                    <small>Độ nhạy của cảm biến (1-5)</small>
                </div>
                
                <div class="form-group">
                    <label for="gpsUpdateInterval">Khoảng Thời Gian Cập Nhật GPS (giây)</label>
                    <input type="number" id="gpsUpdateInterval" name="gpsUpdateInterval" 
                           min="1" max="60" value="5" required>
                    <small>Thời gian giữa các lần đọc GPS</small>
                </div>
                
                <div class="form-group">
                    <label for="telemetryInterval">Khoảng Thời Gian Gửi Dữ Liệu (giây)</label>
                    <input type="number" id="telemetryInterval" name="telemetryInterval" 
                           min="5" max="300" value="30" required>
                    <small>Thời gian giữa các lần gửi dữ liệu lên server</small>
                </div>
                
                <div class="form-group">
                    <div class="checkbox-group">
                        <input type="checkbox" id="enableSMS" name="enableSMS" checked>
                        <label for="enableSMS">Bật Gửi SMS</label>
                    </div>
                </div>
                
                <div class="form-group">
                    <div class="checkbox-group">
                        <input type="checkbox" id="enableCall" name="enableCall" checked>
                        <label for="enableCall">Bật Cuộc Gọi</label>
                    </div>
                </div>
                
                <div class="form-group">
                    <div class="checkbox-group">
                        <input type="checkbox" id="enableRecording" name="enableRecording" checked>
                        <label for="enableRecording">Bật Ghi Âm</label>
                    </div>
                </div>
                
                <div class="form-group">
                    <label for="recordingDuration">Thời Gian Ghi Âm (giây)</label>
                    <input type="number" id="recordingDuration" name="recordingDuration" 
                           min="5" max="60" value="10" required>
                    <small>Thời gian ghi âm khi phát hiện va chạm</small>
                </div>
                
                <div class="buttons">
                    <button type="button" class="btn btn-secondary" onclick="resetConfig()">Dat Lai</button>
                    <button type="submit" class="btn btn-primary">Lưu Cấu Hình</button>
                </div>
            </form>
        </div>
    </div>
    
    <script>
        // Load config khi trang load
        window.addEventListener('DOMContentLoaded', () => {
            loadConfig();
            
            // Update range values
            document.getElementById('collisionThreshold').addEventListener('input', (e) => {
                document.getElementById('thresholdValue').textContent = parseFloat(e.target.value).toFixed(1);
            });
            
            document.getElementById('sensitivity').addEventListener('input', (e) => {
                document.getElementById('sensitivityValue').textContent = e.target.value;
            });
        });
        
        // Load config từ server
        async function loadConfig() {
            try {
                const response = await fetch('/api/config');
                const config = await response.json();
                
                document.getElementById('deviceId').value = config.deviceId || "";
                document.getElementById('userId').value = config.userId || "";
                document.getElementById('collisionThreshold').value = config.collisionThreshold || 2.0;
                document.getElementById('thresholdValue').textContent = parseFloat(config.collisionThreshold || 2.0).toFixed(1);
                document.getElementById('sensitivity').value = config.sensitivity || 3;
                document.getElementById('sensitivityValue').textContent = config.sensitivity || 3;
                document.getElementById('gpsUpdateInterval').value = config.gpsUpdateInterval || 5;
                document.getElementById('telemetryInterval').value = config.telemetryInterval || 30;
                document.getElementById('enableSMS').checked = config.enableSMS !== false;
                document.getElementById('enableCall').checked = config.enableCall !== false;
                document.getElementById('enableRecording').checked = config.enableRecording !== false;
                document.getElementById('recordingDuration').value = config.recordingDuration || 10;
            } catch (error) {
                showAlert('Loi khi tai cau hinh: ' + error.message, 'error');
            }
        }
        
        // Save config
        document.getElementById('configForm').addEventListener('submit', async (e) => {
            e.preventDefault();
            
            const formData = {
                deviceId: document.getElementById('deviceId').value,
                userId: document.getElementById('userId').value,
                collisionThreshold: parseFloat(document.getElementById('collisionThreshold').value),
                sensitivity: parseInt(document.getElementById('sensitivity').value),
                gpsUpdateInterval: parseInt(document.getElementById('gpsUpdateInterval').value),
                telemetryInterval: parseInt(document.getElementById('telemetryInterval').value),
                enableSMS: document.getElementById('enableSMS').checked,
                enableCall: document.getElementById('enableCall').checked,
                enableRecording: document.getElementById('enableRecording').checked,
                recordingDuration: parseInt(document.getElementById('recordingDuration').value)
            };
            
            try {
                const response = await fetch('/api/config', {
                    method: 'POST',
                    headers: {
                        'Content-Type': 'application/json'
                    },
                    body: JSON.stringify(formData)
                });
                
                const result = await response.json();
                
                if (result.success) {
                    showAlert('Cau hinh da duoc luu thanh cong!', 'success');
                } else {
                    showAlert('Loi: ' + (result.error || 'Khong the luu cau hinh'), 'error');
                }
            } catch (error) {
                showAlert('Loi: ' + error.message, 'error');
            }
        });
        
        // Reset config
        async function resetConfig() {
            if (!confirm('Ban co chac muon dat lai cau hinh ve mac dinh?')) {
                return;
            }
            
            try {
                const response = await fetch('/api/reset', {
                    method: 'POST'
                });
                
                const result = await response.json();
                
                if (result.success) {
                    showAlert('Da dat lai cau hinh ve mac dinh!', 'success');
                    setTimeout(() => {
                        loadConfig();
                    }, 1000);
                } else {
                    showAlert('Loi khi dat lai cau hinh', 'error');
                }
            } catch (error) {
                showAlert('Loi: ' + error.message, 'error');
            }
        }
        
        // Show alert
        function showAlert(message, type) {
            const alert = document.getElementById('alert');
            alert.textContent = message;
            alert.className = 'alert alert-' + type;
            alert.style.display = 'block';
            
            setTimeout(() => {
                alert.style.display = 'none';
            }, 5000);
        }
    </script>
</body>
</html>
)HTML";
  
  return html;
}

// ============================================
// EEPROM FUNCTIONS
// ============================================

void loadConfig() {
  EEPROM.get(EEPROM_ADDR, config);
  
  if (!config.valid) {
    Serial.println("⚠️ No valid config found, using defaults");
    setDefaultConfig();
    saveConfig();
  } else {
    Serial.println("✅ Config loaded from EEPROM");
  }
}

void saveConfig() {
  config.valid = true;
  EEPROM.put(EEPROM_ADDR, config);
  EEPROM.commit();
  Serial.println("✅ Config saved to EEPROM");
}

void setDefaultConfig() {
  strcpy(config.deviceId, "ESP32_001");
  strcpy(config.userId, "");
  config.collisionThreshold = 2.0;  // 2g
  config.sensitivity = 3;             // Trung bình
  config.gpsUpdateInterval = 5;      // 5 giây
  config.telemetryInterval = 30;     // 30 giây
  config.enableSMS = true;
  config.enableCall = true;
  config.enableRecording = true;
  config.recordingDuration = 10;     // 10 giây
  config.valid = true;
}

void printConfig() {
  Serial.println("\n=== CURRENT CONFIGURATION ===");
  Serial.printf("Device ID: %s\n", config.deviceId);
  Serial.printf("User ID: %s\n", config.userId);
  Serial.printf("Collision Threshold: %.2f g\n", config.collisionThreshold);
  Serial.printf("Sensitivity: %d (1-5)\n", config.sensitivity);
  Serial.printf("GPS Update Interval: %d seconds\n", config.gpsUpdateInterval);
  Serial.printf("Telemetry Interval: %d seconds\n", config.telemetryInterval);
  Serial.printf("Enable SMS: %s\n", config.enableSMS ? "Yes" : "No");
  Serial.printf("Enable Call: %s\n", config.enableCall ? "Yes" : "No");
  Serial.printf("Enable Recording: %s\n", config.enableRecording ? "Yes" : "No");
  Serial.printf("Recording Duration: %d seconds\n", config.recordingDuration);
  Serial.println("=============================\n");
}

