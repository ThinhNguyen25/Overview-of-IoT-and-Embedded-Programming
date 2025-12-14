// ============================================
// ROBOT LINE FOLLOWER + ESP32 CONTROL
// Chế độ: AUTO dò line | MANUAL điều khiển web
// ============================================

// TB6612FNG
#define STBY A0
#define PWMA 9
#define AIN1 10
#define AIN2 11
#define PWMB 5
#define BIN1 13
#define BIN2 A1

// TCRT5000
#define S1 2
#define S2 3
#define S3 4
#define S4 6
#define S5 7

// HC-SR04
#define trigPin 8
#define echoPin 12

// Biến toàn cục
long duration;
int distance;
bool isAvoiding = false;

// PID
float Kp = 75, Ki = 0, Kd = 1500;
int base_left = 80;
int base_right = 250;
int error = 0, last_error = 0, I = 0;
int left_speed, right_speed;

// Chế độ điều khiển
enum Mode { AUTO, MANUAL };
Mode currentMode = AUTO;

// Biến điều khiển manual
int manualSpeed = 150;
char lastCommand = 'S'; // S=Stop, F=Forward, B=Back, L=Left, R=Right

// Gửi dữ liệu định kỳ
unsigned long lastSendTime = 0;
const unsigned long sendInterval = 300;

void setup() {
  // Khởi tạo chân
  pinMode(STBY, OUTPUT); digitalWrite(STBY, HIGH);
  pinMode(AIN1, OUTPUT); pinMode(AIN2, OUTPUT); pinMode(PWMA, OUTPUT);
  pinMode(BIN1, OUTPUT); pinMode(BIN2, OUTPUT); pinMode(PWMB, OUTPUT);
  pinMode(S1, INPUT); pinMode(S2, INPUT); pinMode(S3, INPUT);
  pinMode(S4, INPUT); pinMode(S5, INPUT);
  pinMode(trigPin, OUTPUT); pinMode(echoPin, INPUT);

  // Serial giao tiếp ESP32
  Serial.begin(115200);
  
  delay(2000);
  Serial.println("ARDUINO: Ready!");
}

void loop() {
  // 1. Đọc lệnh từ ESP32
  readESP32Commands();
  
  // 2. Đọc khoảng cách
  readDistance();
  
  // 3. Xử lý theo chế độ
  if (currentMode == AUTO) {
    autoMode();
  } else {
    manualMode();
  }
  
  // 4. Gửi dữ liệu cho ESP32
  sendSensorData();
  
  delay(10);
}

// ========== ĐỌC LỆNH TỪ ESP32 ==========
void readESP32Commands() {
  if (Serial.available()) {
    String command = Serial.readStringUntil('\n');
    command.trim();
    
    if (command.length() > 0) {
      Serial.print("CMD: "); Serial.println(command);
      
      // Chuyển chế độ
      if (command == "MODE_AUTO") {
        currentMode = AUTO;
        stopMotor();
        Serial.println("MODE: AUTO");
      } 
      else if (command == "MODE_MANUAL") {
        currentMode = MANUAL;
        stopMotor();
        Serial.println("MODE: MANUAL");
      }
      
      // Lệnh điều khiển manual
      else if (currentMode == MANUAL) {
        lastCommand = command.charAt(0);
        
        switch(lastCommand) {
          case 'F': forward(manualSpeed, manualSpeed, 100); break;
          case 'B': backward(manualSpeed, manualSpeed, 100); break;
          case 'L': turnLeft(manualSpeed, 100); break;
          case 'R': turnRight(manualSpeed, 100); break;
          case 'S': stopMotor(); break;
          case '+': manualSpeed = constrain(manualSpeed + 20, 100, 255); break;
          case '-': manualSpeed = constrain(manualSpeed - 20, 100, 255); break;
        }
      }
    }
  }
}

// ========== CHẾ ĐỘ TỰ ĐỘNG ==========
void autoMode() {
  // Né vật cản
  if (distance > 0 && distance <= 15 && !isAvoiding) {
    isAvoiding = true;
    avoidObstacle();
    isAvoiding = false;
    return;
  }
  
  if (isAvoiding) return;
  
  // Dò line
  readSensors();
  pidControl();
  motorControl();
}

// ========== CHẾ ĐỘ THỦ CÔNG ==========
void manualMode() {
  // Giữ lệnh cuối (giữ nút trên web)
  if (lastCommand != 'S') {
    switch(lastCommand) {
      case 'F': forward(manualSpeed, manualSpeed, 100); break;
      case 'B': backward(manualSpeed, manualSpeed, 100); break;
      case 'L': turnLeft(manualSpeed, 100); break;
      case 'R': turnRight(manualSpeed, 100); break;
    }
  }
}

// ========== GỬI DỮ LIỆU CHO ESP32 ==========
void sendSensorData() {
  unsigned long currentTime = millis();
  
  if (currentTime - lastSendTime >= sendInterval) {
    // Tạo JSON
    Serial.print("{");
    
    // Cảm biến line
    Serial.print("\"s\":[");
    Serial.print(digitalRead(S1)); Serial.print(",");
    Serial.print(digitalRead(S2)); Serial.print(",");
    Serial.print(digitalRead(S3)); Serial.print(",");
    Serial.print(digitalRead(S4)); Serial.print(",");
    Serial.print(digitalRead(S5));
    Serial.print("],");
    
    // Dữ liệu khác
    Serial.print("\"d\":");
    Serial.print(distance);
    Serial.print(",\"e\":");
    Serial.print(error);
    Serial.print(",\"l\":");
    Serial.print(left_speed);
    Serial.print(",\"r\":");
    Serial.print(right_speed);
    Serial.print(",\"a\":");
    Serial.print(isAvoiding ? "1" : "0");
    Serial.print(",\"m\":");
    Serial.print(currentMode == AUTO ? "0" : "1");
    Serial.print(",\"c\":\"");
    Serial.print(lastCommand);
    Serial.print("\",\"sp\":");
    Serial.print(manualSpeed);
    
    Serial.println("}");
    
    lastSendTime = currentTime;
  }
}

// ========== CÁC HÀM ĐIỀU KHIỂN ĐỘNG CƠ ==========
void motorControl() {
  digitalWrite(AIN1, HIGH); digitalWrite(AIN2, LOW);
  analogWrite(PWMA, right_speed);
  digitalWrite(BIN1, HIGH); digitalWrite(BIN2, LOW);
  analogWrite(PWMB, left_speed);
}

void forward(int speedL, int speedR, int t) {
  digitalWrite(AIN1, HIGH); digitalWrite(AIN2, LOW);
  analogWrite(PWMA, speedR);
  digitalWrite(BIN1, HIGH); digitalWrite(BIN2, LOW);
  analogWrite(PWMB, speedL);
  if (t > 0) delay(t);
}

void backward(int speedL, int speedR, int t) {
  digitalWrite(AIN1, LOW); digitalWrite(AIN2, HIGH);
  analogWrite(PWMA, speedR);
  digitalWrite(BIN1, LOW); digitalWrite(BIN2, HIGH);
  analogWrite(PWMB, speedL);
  if (t > 0) delay(t);
}

void turnLeft(int speed, int t) {
  digitalWrite(AIN1, LOW); digitalWrite(AIN2, HIGH);
  analogWrite(PWMA, speed);
  digitalWrite(BIN1, HIGH); digitalWrite(BIN2, LOW);
  analogWrite(PWMB, speed);
  if (t > 0) delay(t);
}

void turnRight(int speed, int t) {
  digitalWrite(AIN1, HIGH); digitalWrite(AIN2, LOW);
  analogWrite(PWMA, speed);
  digitalWrite(BIN1, LOW); digitalWrite(BIN2, HIGH);
  analogWrite(PWMB, speed);
  if (t > 0) delay(t);
}

void stopMotor() {
  analogWrite(PWMA, 0);
  analogWrite(PWMB, 0);
}

// ========== HÀM DÒ LINE VÀ PID ==========
void readDistance() {
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  duration = pulseIn(echoPin, HIGH, 30000);
  distance = duration * 0.034 / 2;
}

void readSensors() {
  int s1 = digitalRead(S1);
  int s2 = digitalRead(S2);
  int s3 = digitalRead(S3);
  int s4 = digitalRead(S4);
  int s5 = digitalRead(S5);

  if      (s1==1&&s2==1&&s3==1&&s4==1&&s5==0) error=-4;
  else if (s1==1&&s2==1&&s3==1&&s4==0&&s5==0) error=-3;
  else if (s1==1&&s2==1&&s3==1&&s4==0&&s5==1) error=-2;
  else if (s1==1&&s2==1&&s3==0&&s4==0&&s5==1) error=-1;
  else if (s1==1&&s2==1&&s3==0&&s4==1&&s5==1) error=0;
  else if (s1==1&&s2==0&&s3==0&&s4==1&&s5==1) error=1;
  else if (s1==1&&s2==0&&s3==1&&s4==1&&s5==1) error=2;
  else if (s1==0&&s2==0&&s3==1&&s4==1&&s5==1) error=3;
  else if (s1==0&&s2==1&&s3==1&&s4==1&&s5==1) error=4;
  else error = last_error;
}

void pidControl() {
  int P = error;
  I += error;
  if (I > 50) I = 50; if (I < -50) I = -50;
  int D = error - last_error;
  int pid = Kp * P + Ki * I + Kd * D;
  last_error = error;

  if (error >= 3) {
    left_speed = 0;
    right_speed = base_right * 1.65;
  } else if (error <= -3) {
    left_speed = base_left * 1.65;
    right_speed = 0;
  } else if (error == 2 || error == -2) {
    pid *= 0.9;
    left_speed = base_left - pid;
    right_speed = base_right + pid;
  } else {
    left_speed = base_left - pid;
    right_speed = base_right + pid;
  }
  
  left_speed = constrain(left_speed, 0, 255);
  right_speed = constrain(right_speed, 0, 255);
}

// ========== NÉ VẬT CẢN ==========
void avoidObstacle() {
  stopMotor(); delay(300);
  backward(100, 200, 300);
  stopMotor(); delay(300);
  turnLeft(250, 1700);
  stopMotor(); delay(500);
  forward(100, 200, 2000);
  stopMotor(); delay(500);
  turnRight(240, 1700);
  stopMotor(); delay(500);
  forward(100, 200, 3500);
  stopMotor(); delay(500);
  turnRight(240, 1700);
  stopMotor(); delay(500);
  forward(100, 200, 2000);
  stopMotor(); delay(500);
  turnLeft(250, 1700);
  stopMotor(); delay(300);
}