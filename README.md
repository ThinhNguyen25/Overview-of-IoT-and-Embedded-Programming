# 🤖 **Robot Dò Line & Điều Khiển Web IoT**

<p align="center">
  <img src="https://github.com/user-attachments/assets/6f2f369f-302a-43a2-9e72-346077742443" alt="Robot Main" width="800"/>
  <br>
  <em>Hệ thống robot IoT thông minh - Tự động dò line và điều khiển qua web</em>
</p>

## 📋 **Tổng Quan Dự Án**

Dự án này xây dựng một **hệ thống robot IoT** kết hợp khả năng tự động dò line với điều khiển từ xa qua web. Robot có thể hoạt động ở hai chế độ: **tự động** (dò line và né vật cản) và **thủ công** (điều khiển qua giao diện web). Hệ thống sử dụng Arduino Uno xử lý cảm biến và động cơ, ESP32 làm gateway WiFi để phát sóng và tạo web server.

---

## 🎯 **Tính Năng Chính**

### 🤖 **Chế Độ Tự Động**
- **Dò line chính xác** với thuật toán PID
- **Né vật cản tự động** bằng cảm biến siêu âm HC-SR04
- **Xử lý đường cong** và ngã rẽ phức tạp
- **Tự động quay về line** sau khi né vật cản

### 🌐 **Điều Khiển Qua Web**
- **Giao diện web responsive** hoạt động trên cả PC và mobile
- **Điều khiển real-time** với độ trễ thấp
- **Hiển thị dữ liệu cảm biến** trực quan
- **Chuyển đổi chế độ** linh hoạt (Auto ↔ Manual)

### 📱 **Đa Nền Tảng**
- **ESP32 phát WiFi AP** - không cần router
- **Truy cập qua bất kỳ thiết bị nào**: điện thoại, máy tính, tablet
- **Điều khiển bằng**: nút trên web, bàn phím (WASD), touch screen

---

## 🛠️ **Thành Phần Hệ Thống**

### **Phần Cứng**
| Component | Số Lượng | Chức Năng |
|-----------|----------|-----------|
| Arduino Uno R3 | 1 | Bộ xử lý chính, điều khiển cảm biến và động cơ |
| ESP32 DevKit | 1 | Gateway WiFi, Web Server |
| TCRT5000 (5 LED) | 1 | Cảm biến dò line đen/trắng |
| HC-SR04 | 1 | Cảm biến đo khoảng cách vật cản |
| TB6612FNG | 1 | Driver điều khiển motor DC |
| Motor DC N20 giảm tốc | 2 | Động cơ di chuyển robot |
| Pin LiPo 7.4V | 1 | Nguồn điện cho hệ thống |
| Dây jumper | Nhiều | Kết nối linh kiện |

### **Phần Mềm & Công Nghệ**
- **Arduino IDE** - Lập trình vi điều khiển
- **PlatformIO** - Phát triển ESP32
- **HTML5/CSS3/JavaScript** - Giao diện web
- **JSON** - Định dạng dữ liệu giao tiếp
- **HTTP/REST API** - Giao tiếp client-server

---

## 🔌 **Sơ Đồ Kết Nối**

### **Arduino Uno ↔ ESP32**
```
Arduino Uno           ESP32
TX (Pin 1)    →    RX2 (GPIO16)
RX (Pin 0)    →    TX2 (GPIO17)
GND           →    GND
```

### **Arduino với Các Module**
```cpp
// TB6612FNG Driver
STBY  → A0
PWMA  → 9    (Motor phải - PWM)
AIN1  → 10   (Hướng motor phải)
AIN2  → 11   (Hướng motor phải)
PWMB  → 5    (Motor trái - PWM)
BIN1  → 13   (Hướng motor trái)
BIN2  → A1   (Hướng motor trái)

// TCRT5000 (5 cảm biến)
S1    → 2    (Trái nhất)
S2    → 3
S3    → 4    (Giữa)
S4    → 6
S5    → 7    (Phải nhất)

// HC-SR04
Trig  → 8
Echo  → 12
```

<p align="center">
  <img src="https://github.com/user-attachments/assets/685842f4-b097-400c-83dd-36b55d3045f1" alt="Sơ đồ mạch" width="600"/>
</p>

## 🖥️ **Giao Diện Web**

### **Dashboard Chính**
<p align="center">
  <img src="https://via.placeholder.com/800x400/3498db/ffffff?text=Robot+Control+Dashboard" alt="Dashboard" width="800"/>
</p>

### **Các Khu Vực Chức Năng**
1. **Hiển Thị Cảm Biến**: 5 đèn LED hiển thị trạng thái line
2. **Thông Số Vận Hành**: Khoảng cách, lỗi PID, tốc độ motor
3. **Chọn Chế Độ**: Auto (tự động) / Manual (thủ công)
4. **Điều Khiển Tay**: Joystick ảo (WASD layout)
5. **Điều Chỉnh Tốc độ**: Tăng/giảm tốc độ motor
6. **Trạng Thái Hệ Thống**: Mode, lệnh cuối, kết nối

### **Điều Khiển Bằng Bàn Phím**
```
W / ↑    : Tiến
S / ↓    : Lùi
A / ←    : Rẽ trái
D / →    : Rẽ phải
SPACE    : Dừng
+        : Tăng tốc
-        : Giảm tốc
```

---

## ⚙️ **Thuật Toán PID**

```cpp
// Tham số PID (có thể điều chỉnh)
float Kp = 75.0;    // Proportional gain
float Ki = 0.0;     // Integral gain  
float Kd = 1500.0;  // Derivative gain

// Tính toán PID
int P = error;
I += error;
I = constrain(I, -50, 50);  // Anti-windup
int D = error - last_error;
int pid_value = Kp * P + Ki * I + Kd * D;
```

### **Mapping Error từ Cảm Biến**
```cpp
if (S1=1, S2=1, S3=1, S4=1, S5=0) error = -4;  // Lệch phải mạnh
if (S1=1, S2=1, S3=0, S4=1, S5=1) error = 0;   // Chính giữa
if (S1=0, S2=1, S3=1, S4=1, S5=1) error = 4;   // Lệch trái mạnh
```

---

## 🚧 **Thuật Toán Né Vật Cản**

```cpp
void avoidObstacle() {
    stopMotor(); delay(300);
    backward(100, 200, 300);     // Lùi lại
    stopMotor(); delay(300);
    turnLeft(250, 1700);         // Rẽ trái
    forward(100, 200, 2000);     // Đi thẳng
    turnRight(240, 1700);        // Rẽ phải
    forward(100, 200, 3500);     // Đi thẳng dài
    turnRight(240, 1700);        // Rẽ phải
    forward(100, 200, 2000);     // Đi thẳng
    turnLeft(250, 1700);         // Rẽ trái về line
}
```

---

## 📊 **Thông Số Kỹ Thuật**

| Thông Số | Giá Trị | Đơn Vị |
|----------|---------|--------|
| Tốc độ tối đa | 0.5 | m/s |
| Độ chính xác dò line | >95 | % |
| Khoảng cách phát hiện | 2-400 | cm |
| Thời gian né vật cản | ~8 | giây |
| Khoảng cách WiFi | ~30 | mét |
| Độ trễ điều khiển | <100 | ms |
| Thời gian chạy pin | ~2 | giờ |

---

## 🎥 **Video Demo**

<p align="center">
  <img src="https://github.com/user-attachments/assets/f167314a-7e9b-476b-a88a-c042c5d86992" alt="Video Demo" width="800"/>
  <br>
  <a href="#"><em>Xem video demo đầy đủ</em></a>
</p>

---

## 🔧 **Xử Lý Sự Cố**

| Vấn Đề | Nguyên Nhân | Giải Pháp |
|--------|-------------|-----------|
| Không kết nối WiFi | ESP32 chưa khởi động | Reset ESP32 (nút EN) |
| Web không load | Sai địa chỉ IP | Truy cập 192.168.4.1 |
| Robot không di chuyển | Hết pin | Sạc pin LiPo |
| Dò line sai | Cảm biến bẩn | Vệ sinh TCRT5000 |
| PID không ổn định | Tham số chưa tối ưu | Điều chỉnh Kp, Ki, Kd |

---

## 📈 **Hướng Phát Triển**

### **Ngắn Hạn**
- [ ] Thêm camera ESP32-CAM để stream video
- [ ] Tích hợp điều khiển giọng nói
- [ ] Lưu trữ dữ liệu lên cloud (Firebase)
- [ ] Tạo ứng dụng di động (React Native)

### **Dài Hạn**
- [ ] Machine Learning cho nhận diện vật cản
- [ ] SLAM cho lập bản đồ
- [ ] Điều khiển đa robot
- [ ] Tích hợp ROS (Robot Operating System)


## 🙏 **Ghi Nhận**

| Thành Viên | Vai Trò |
|------------|---------|
| [Nhóm 3 CNTT 17-01] | 
| [Bùi Hữu Tri Phương[ | Phát triển phần cứng |
| [Nguyễn Quang Thịnh[ | Lập trình Arduino |
| [Lại Minh Hiệp[ | Phát triển web interface |
| [Hoàng Đình Gia Huy[ | Testing & Documentation |




<p align="center">
  <strong>⭐ Nếu bạn thấy dự án hữu ích, hãy cho chúng tôi một star trên GitHub!</strong>
</p>

<p align="center">
  <img src="https://img.shields.io/github/stars/your-repo?style=social" alt="GitHub Stars"/>
  <img src="https://img.shields.io/github/forks/your-repo?style=social" alt="GitHub Forks"/>
  <img src="https://img.shields.io/github/issues/your-repo" alt="GitHub Issues"/>
</p>
