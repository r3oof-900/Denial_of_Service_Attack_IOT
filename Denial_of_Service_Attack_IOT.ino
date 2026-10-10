
#include <WiFi.h>
#include <ESPping.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// ==========================================
// Project: Denial of Service Attack IoT
// Platform: ESP32
// Purpose: Educational Network Security Demo
// ==========================================

// Wi-Fi Configuration
// Replace these values for local testing.
// Do not publish real Wi-Fi credentials.

const char* ssid = "YOUR_WIFI_NAME";
const char* password = "YOUR_WIFI_PASSWORD";

// Hardware Configuration
#define BUTTON_PIN 0
#define LCD_SDA 21
#define LCD_SCL 22

LiquidCrystal_I2C lcd(0x27, 16, 2);

// Network Discovery
const int MAX_DEVICES = 20;
IPAddress devices[MAX_DEVICES];

int devicesFound = 0;
int selectedIndex = 0;

IPAddress selectedIP(0, 0, 0, 0);

// Simulation Statistics
unsigned long simulatedPackets = 0;
unsigned long startTime = 0;
unsigned long lastLCD = 0;
unsigned long lastSerial = 0;

const unsigned long DISPLAY_INTERVAL = 500;
const unsigned long SIMULATED_RATE = 1800;

// ==========================================
// 1. LCD Helper
// ==========================================

void showMessage(const String& line1,
                 const String& line2 = "") {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(line1);
  lcd.setCursor(0, 1);
  lcd.print(line2);
}

// ==========================================
// 2. Wi-Fi Connection
// ==========================================

void connectWiFi() {
  showMessage("Connecting...", "Please wait");

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  Serial.println("Connecting to Wi-Fi...");

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println();
  Serial.println("Wi-Fi connected!");

  Serial.print("ESP32 IP: ");
  Serial.println(WiFi.localIP());

  showMessage("WiFi Connected", WiFi.localIP().toString());
  delay(2000);
}

// ==========================================
// 3. Network Discovery
// ==========================================

void scanNetwork() {
  showMessage("Scanning LAN...", "Please wait");

  Serial.println();
  Serial.println("=== Network Discovery ===");

  devicesFound = 0;

  IPAddress localIP = WiFi.localIP();

  for (int i = 1;
       i <= 254 && devicesFound < MAX_DEVICES;
       i++) {

    IPAddress testIP(
      localIP[0],
      localIP[1],
      localIP[2],
      i
    );

    if (testIP == localIP) {
      continue;
    }

    lcd.setCursor(0, 1);
    lcd.print("Scan:");
    lcd.print(i);
    lcd.print("/254    ");

    if (Ping.ping(testIP, 1)) {
      devices[devicesFound] = testIP;
      devicesFound++;

      Serial.print("Found: ");
      Serial.println(testIP.toString());
    }
  }

  Serial.print("Total devices found: ");
  Serial.println(devicesFound);

  showMessage(
    "Scan Complete",
    "Found: " + String(devicesFound)
  );

  delay(2000);
}

// ==========================================
// 4. Device Selection
// ==========================================

void selectDevice() {
  if (devicesFound == 0) {
    showMessage("No Devices", "Restarting...");
    delay(3000);
    ESP.restart();
  }

  selectedIndex = 0;

  unsigned long lastChange = millis();
  unsigned long lastButton = 0;

  const unsigned long CONFIRM_DELAY = 5000;

  Serial.println();
  Serial.println("=== Device Selection ===");
  Serial.println("Press BOOT to change device.");

  while (true) {
    unsigned long now = millis();

    if (digitalRead(BUTTON_PIN) == LOW &&
        now - lastButton > 300) {

      lastButton = now;
      lastChange = now;

      selectedIndex =
        (selectedIndex + 1) % devicesFound;

      Serial.print("Selected: ");
      Serial.println(
        devices[selectedIndex].toString()
      );
    }

    if (now - lastChange >= CONFIRM_DELAY) {
      break;
    }

    lcd.setCursor(0, 0);
    lcd.print("Device:");
    lcd.print(selectedIndex + 1);
    lcd.print("/");
    lcd.print(devicesFound);
    lcd.print("    ");

    lcd.setCursor(0, 1);
    lcd.print(
      devices[selectedIndex].toString()
    );

    delay(100);
  }

  selectedIP = devices[selectedIndex];

  showMessage(
    "Selected Device",
    selectedIP.toString()
  );

  Serial.print("Confirmed IP: ");
  Serial.println(selectedIP.toString());

  delay(2000);
}

// ==========================================
// 5. Safe Traffic Simulation
// ==========================================

void updateSimulation() {
  unsigned long elapsedMs = millis() - startTime;

  // Calculated simulation only.
  // No UDP packets are transmitted.
  simulatedPackets =
    (elapsedMs / 1000UL) * SIMULATED_RATE +
    ((elapsedMs % 1000UL) * SIMULATED_RATE) / 1000UL;
}

// ==========================================
// 6. LCD Statistics
// ==========================================

void updateLCD() {
  lcd.clear();

  lcd.setCursor(0, 0);
  lcd.print("SIM Tgt:");
  lcd.print(selectedIP[3]);

  lcd.setCursor(0, 1);
  lcd.print("P:");

  if (simulatedPackets > 999999) {
    lcd.print(simulatedPackets / 1000);
    lcd.print("k");
  } else {
    lcd.print(simulatedPackets);
  }

  lcd.print(" R:");
  lcd.print(SIMULATED_RATE);
}

// ==========================================
// 7. Setup
// ==========================================

void setup() {
  Serial.begin(115200);
  delay(500);

  pinMode(BUTTON_PIN, INPUT_PULLUP);

  Wire.begin(LCD_SDA, LCD_SCL);

  lcd.init();
  lcd.backlight();

  showMessage(
    "IoT Security",
    "ESP32 Demo"
  );

  delay(2000);

  connectWiFi();
  scanNetwork();
  selectDevice();

  showMessage(
    "Simulation",
    "Starting..."
  );

  delay(1500);

  startTime = millis();

  Serial.println();
  Serial.println("=== Simulation Started ===");
  Serial.println("No packets are transmitted.");
}

// ==========================================
// 8. Main Loop
// ==========================================

void loop() {
  updateSimulation();

  unsigned long now = millis();

  if (now - lastLCD >= DISPLAY_INTERVAL) {
    lastLCD = now;
    updateLCD();
  }

  if (now - lastSerial >= 2000) {
    lastSerial = now;

    unsigned long elapsed =
      (now - startTime) / 1000;

    Serial.printf(
      "[SIMULATION] IP: %s | Packets: %lu | Time: %lus | Rate: %lu pkt/s\n",
      selectedIP.toString().c_str(),
      simulatedPackets,
      elapsed,
      SIMULATED_RATE
    );
  }
}
