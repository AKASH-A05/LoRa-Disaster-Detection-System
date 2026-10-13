#include <SPI.h>
#include <LoRa.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

#define SS 10
#define RST 9
#define DIO0 2
#define BUZZER 7

LiquidCrystal_I2C lcd(0x27, 16, 2);

#define NORMAL_SCROLL_DELAY 2000
#define EMERGENCY_HOLD_TIME 10000

String receivedData = "";
unsigned long lastUpdate = 0;
int page = 0;
bool emergencyActive = false;
String emergencyReason = "";

void setup() {
  Serial.begin(9600);
  lcd.init();
  lcd.backlight();
  pinMode(BUZZER, OUTPUT);
  digitalWrite(BUZZER, LOW);

  lcd.setCursor(0, 0);
  lcd.print("Initializing...");
  delay(1500);
  lcd.clear();

  LoRa.setPins(SS, RST, DIO0);
  if (!LoRa.begin(433E6)) {
    lcd.setCursor(0, 0);
    lcd.print("LoRa Failed!");
    while (1);
  }

  lcd.setCursor(0, 0);
  lcd.print("LoRa Ready");
  delay(1000);
  lcd.clear();
}

void loop() {
  int packetSize = LoRa.parsePacket();
  if (packetSize) {
    receivedData = "";
    while (LoRa.available()) {
      receivedData += (char)LoRa.read();
    }

    Serial.println("Received: " + receivedData);
    receivedData.trim();

    // ---------- Emergency detection ----------
    emergencyReason = detectEmergency(receivedData);
    if (receivedData.indexOf("EMERGENCY:") != -1 && emergencyReason != "") {
      emergencyActive = true;
      digitalWrite(BUZZER, HIGH);
      displayEmergency(emergencyReason);
      delay(EMERGENCY_HOLD_TIME);
      digitalWrite(BUZZER, LOW);
      emergencyActive = false;
      lcd.clear();
    }
  }

  // ---------- Normal Display Loop ----------
  if (!emergencyActive && (millis() - lastUpdate > NORMAL_SCROLL_DELAY)) {
    displayNormalData(receivedData);
    lastUpdate = millis();
  }
}

// ---------- Normal Data Display ----------
void displayNormalData(String data) {
  if (data.indexOf("NORMAL DATA") == -1) return; // only show when normal data
  lcd.clear();

  switch (page) {
    case 0:
      lcd.setCursor(0, 0);
      lcd.print("ZONE: SIET");
      lcd.setCursor(0, 1);
      lcd.print("Monitoring...");
      break;
    case 1:
      lcd.setCursor(0, 0);
      lcd.print("TEMP:");
      lcd.print(extractValue(data, "TEMP:"));
      break;
    case 2:
      lcd.setCursor(0, 0);
      lcd.print("HUMIDITY:");
      lcd.print(extractValue(data, "HUMIDITY:"));
      break;
    case 3:
      lcd.setCursor(0, 0);
      lcd.print("GAS:");
      lcd.print(extractValue(data, "GAS:"));
      break;
    case 4:
      lcd.setCursor(0, 0);
      lcd.print("FLAME:");
      lcd.print(extractValue(data, "FLAME:"));
      break;
    case 5:
      lcd.setCursor(0, 0);
      lcd.print("EARTHQUAKE:");
      lcd.setCursor(0, 1);
      lcd.print(extractValue(data, "EARTHQUAKE:"));
      break;
    case 6:
      lcd.setCursor(0, 0);
      lcd.print("SITUATION:");
      lcd.setCursor(0, 1);
      lcd.print("NORMAL");
      break;
  }

  page++;
  if (page > 6) page = 0;
}

// ---------- Emergency Display ----------
void displayEmergency(String reason) {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("SIET EMERGENCY");
  lcd.setCursor(0, 1);
  lcd.print(reason);
  Serial.println("🚨 EMERGENCY: " + reason);
}

// ---------- Extract values ----------
String extractValue(String data, String key) {
  int start = data.indexOf(key);
  if (start == -1) return "N/A";
  start += key.length();
  int end = data.indexOf('\n', start);
  if (end == -1) end = data.length();
  return data.substring(start, end);
}

// ---------- Detect emergency reason ----------
String detectEmergency(String data) {
  data.toUpperCase();

  if (data.indexOf("FIRE DETECTED") != -1 || data.indexOf("FLAME DETECTED") != -1)
    return "FIRE DETECTED";

  if (data.indexOf("HIGH TEMPERATURE") != -1 || data.indexOf("TEMP HIGH") != -1)
    return "HIGH TEMP";

  if (data.indexOf("GAS LEAK") != -1)
    return "GAS LEAK";

  if (data.indexOf("EARTHQUAKE DETECTED") != -1)
    return "EARTHQUAKE";

  return "";
}
