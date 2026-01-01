/*
 * GLOBAL ENERGY MONITOR (ESP32 + INA219 + OLED + BLYNK)
 * Optimized for Speed: Decoupled Display (Fast) from Cloud (Slow)
 */

// --- 1. PASTE YOUR BLYNK CONFIG HERE ---
#define BLYNK_TEMPLATE_ID "TMPL3srLlqD84"
#define BLYNK_TEMPLATE_NAME "Energy meter"
#define BLYNK_AUTH_TOKEN "TYuaXEORdDSRb_dGKr-u60AkPYMNfpey" // <--- MAKE SURE THIS IS CORRECT

// --- Libraries ---
#define BLYNK_PRINT Serial
#include <WiFi.h>
#include <WiFiClient.h>
#include <BlynkSimpleEsp32.h>
#include <Wire.h>
#include <Adafruit_INA219.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// --- Wi-Fi Credentials ---
char ssid[] = "AndroidAP_6162";
char pass[] = "buddhadeb";

// --- OLED Display Setup ---
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// --- INA219 Setup ---
Adafruit_INA219 ina219;

// --- Global Variables ---
float voltage_V = 0;
float current_mA = 0;
float power_mW = 0;
double totalEnergy_mWs = 0.0;
unsigned long lastReadTime = 0;

// --- Blynk Timer ---
BlynkTimer timer;

// --- FUNCTION 1: FAST LOOP (Runs every 250ms) ---
// This handles local updates so the screen feels responsive
void updateSensorsAndScreen() {
  // 1. Calculate Time Passed (for Energy Math)
  unsigned long now = millis();
  unsigned long timeInterval_ms = now - lastReadTime;
  
  // 2. Read Sensor
  voltage_V = ina219.getBusVoltage_V();
  current_mA = ina219.getCurrent_mA();
  power_mW = ina219.getPower_mW();
  
  // 3. Calculate Energy (Accumulate every 250ms for better accuracy)
  if (lastReadTime > 0) {
    double energyInterval_mWs = power_mW * (timeInterval_ms / 1000.0);
    totalEnergy_mWs += energyInterval_mWs;
  }
  lastReadTime = now;

  // 4. Update Local OLED
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  
  display.setCursor(0, 0);
  display.println("--- GLOBAL MONITOR ---"); 
  
  display.setCursor(0, 12);
  display.print("Volt: "); display.print(voltage_V, 2); display.println(" V");
  
  display.setCursor(0, 22);
  display.print("Curr: "); display.print(current_mA, 1); display.println(" mA");
  
  display.setCursor(0, 32);
  display.print("Powr: "); display.print(power_mW, 1); display.println(" mW");

  display.setCursor(0, 42);
  display.print("Nrg:  "); display.print(totalEnergy_mWs / 1000.0, 3); display.println(" J");
  
  display.setCursor(0, 54);
  display.print("Status: "); 
  if(Blynk.connected()) {
    display.print("Online");
  } else {
    display.print("Offline");
  }
  
  display.display();
}

// --- FUNCTION 2: SLOW LOOP (Runs every 1000ms) ---
// This sends data to the cloud. We keep this slow to prevent lag.
void sendDataToCloud() {
  if (Blynk.connected()) {
    Blynk.virtualWrite(V0, voltage_V);                 // Send Voltage
    Blynk.virtualWrite(V1, current_mA);                // Send Current
    Blynk.virtualWrite(V2, power_mW);                  // Send Power
    Blynk.virtualWrite(V3, totalEnergy_mWs / 1000.0);  // Send Energy in Joules
    
    // Debug
    Serial.print("Cloud Update -> Power: "); Serial.print(power_mW); Serial.println(" mW");
  }
}

void setup() {
  Serial.begin(115200);
  Wire.begin();

  // --- Init Hardware ---
  if (!ina219.begin()) {
    Serial.println("Failed to find INA219 chip");
    while (1) { delay(10); }
  }

  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { 
    Serial.println(F("SSD1306 allocation failed"));
    for(;;);
  }
  
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0,0);
  
  // --- MANUAL CONNECTION DEBUGGING ---
  display.println("Connecting WiFi...");
  display.println(ssid);
  display.display();
  
  WiFi.begin(ssid, pass);
  int attempt = 0;
  while (WiFi.status() != WL_CONNECTED && attempt < 20) { // Reduced wait time
    delay(500);
    Serial.print(".");
    attempt++;
  }
  
  display.clearDisplay();
  display.setCursor(0,0);
  
  if (WiFi.status() == WL_CONNECTED) {
    display.println("WiFi Connected!");
    display.println(WiFi.localIP());
    display.display();
    delay(500);
    
    // Configure Blynk
    display.println("Connecting Blynk...");
    display.display();
    Blynk.config(BLYNK_AUTH_TOKEN);
    Blynk.connect(); // Try to connect once
    // We don't block here anymore, we let loop handle it
    
  } else {
    display.println("WiFi Failed!");
    display.display();
    delay(2000);
  }

  // --- Setup Timers ---
  lastReadTime = millis();
  
  // Timer 1: Read Sensor & Update OLED (FAST - every 250ms)
  timer.setInterval(250L, updateSensorsAndScreen);
  
  // Timer 2: Send to Blynk Cloud (SLOW - every 1000ms)
  timer.setInterval(1000L, sendDataToCloud);
}

void loop() {
  // Only run Blynk.run() if connected, otherwise it blocks the OLED!
  if (Blynk.connected()) {
    Blynk.run();
  }
  
  // Always run timers (this keeps OLED updating even if WiFi is down)
  timer.run(); 
}
