#include <WiFi.h>
#include <FirebaseESP32.h>
#include <DHT.h>

// WiFi Credentials
#define WIFI_SSID "Kingshouse-Tret1"
#define WIFI_PASSWORD "kingshouse2018"

// Firebase Credentials
#define FIREBASE_HOST "https://app-mobile-esp32-default-rtdb.firebaseio.com/"
#define FIREBASE_AUTH "0N0o2YXFi9h4itqUT8tHk6t0squrkhH2hmH7B186"

// DHT11 Configuration
#define DHTPIN 4
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

// LED Pin Configuration
#define LED_PIN 26 // LED

// Firebase Objects
FirebaseData fbdo;
FirebaseConfig firebaseConfig;
FirebaseAuth firebaseAuth;

// Timing
unsigned long previousMillis = 0;
const long interval = 1000; // Giảm xuống 1 giây để đọc nhanh hơn

void setup() {
    Serial.begin(115200);
    Serial.println(F("Khởi động hệ thống cho ESP32 DHT Control!"));

    // Thiết lập chân LED
    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, LOW);
    Serial.println("Chân LED được thiết lập: GPIO " + String(LED_PIN));

    // Kiểm tra LED ban đầu
    testLED();

    // Khởi động DHT11
    dht.begin();

    // Kết nối WiFi
    connectWiFi();

    // Cấu hình Firebase
    firebaseConfig.host = FIREBASE_HOST;
    firebaseConfig.signer.tokens.legacy_token = FIREBASE_AUTH;
    Firebase.begin(&firebaseConfig, &firebaseAuth);
    Firebase.reconnectWiFi(true);

    if (Firebase.ready()) {
        Serial.println("Đã kết nối Firebase!");
    } else {
        Serial.println("Không thể kết nối Firebase!");
    }

    // Khởi tạo trạng thái Firebase
    initializeFirebaseStates();
}

void loop() {
    unsigned long currentMillis = millis();
    if (currentMillis - previousMillis >= interval) {
        previousMillis = currentMillis;

        // Kiểm tra WiFi
        if (WiFi.status() != WL_CONNECTED) {
            Serial.println("WiFi ngắt kết nối, thử lại...");
            connectWiFi();
        }

        if (Firebase.ready()) {
            Serial.println("Đang kiểm tra trạng thái..."); // Debug
            sendSensorData();
            controlLED();
        } else {
            Serial.println("Firebase không sẵn sàng!");
        }
    }
}

void connectWiFi() {
    Serial.print("Đang kết nối WiFi...");
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    int retryCount = 0, maxRetries = 20;
    while (WiFi.status() != WL_CONNECTED && retryCount < maxRetries) {
        Serial.print(".");
        delay(500);
        retryCount++;
    }
    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("\nĐã kết nối WiFi! IP: " + WiFi.localIP().toString());
    } else {
        Serial.println("\nKhông thể kết nối WiFi!");
    }
}

void initializeFirebaseStates() {
    Firebase.setFloat(fbdo, "/TEMPERATURE", 0.0);
    Firebase.setFloat(fbdo, "/HUMIDITY", 0.0);
    Firebase.setInt(fbdo, "/LED", 0);

    if (fbdo.errorReason() == "") {
        Serial.println("Khởi tạo trạng thái Firebase hoàn tất!");
    } else {
        Serial.println("Lỗi khởi tạo Firebase: " + fbdo.errorReason());
    }
}

void sendSensorData() {
    float humidity = dht.readHumidity();
    float temperature = dht.readTemperature();

    if (!isnan(humidity) && !isnan(temperature)) {
        if (Firebase.setFloat(fbdo, "/TEMPERATURE", temperature)) {
            Serial.println("Nhiệt độ: " + String(temperature) + "°C");
        } else {
            Serial.println("Lỗi gửi nhiệt độ: " + fbdo.errorReason());
        }
        if (Firebase.setFloat(fbdo, "/HUMIDITY", humidity)) {
            Serial.println("Độ ẩm: " + String(humidity) + "%");
        } else {
            Serial.println("Lỗi gửi độ ẩm: " + fbdo.errorReason());
        }
    } else {
        Firebase.setString(fbdo, "/sensor_status", "DHT11 Error");
        Serial.println("Lỗi đọc DHT11!");
    }
}

void controlLED() {
    if (Firebase.getInt(fbdo, "/LED")) {
        int state = fbdo.intData();
        Serial.println("Giá trị LED từ Firebase: " + String(state)); // Debug
        digitalWrite(LED_PIN, state == 1 ? HIGH : LOW); // Sửa để rõ ràng hơn
        Serial.println("LED: " + String(state == 1 ? "Bật" : "Tắt"));
    } else {
        Serial.println("Lỗi đọc trạng thái LED: " + fbdo.errorReason());
    }
}

void testLED() {
    Serial.println("Kiểm tra LED...");
    digitalWrite(LED_PIN, HIGH);
    Serial.println("LED bật...");
    delay(2000);
    digitalWrite(LED_PIN, LOW);
    Serial.println("LED tắt...");
    delay(2000);
}