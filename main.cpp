#include <Wire.h>
#include <Adafruit_INA219.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>

// --- Libraries for OLED ---
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// --- Your Wi-Fi Credentials ---
const char* ssid = "AndroidAP_6162";
const char* password = "buddhadeb";

// --- OLED Display Setup (0.96 inch, 128x64) ---
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// --- Global Variables for INA219 Data ---
float voltage_V = 0;
float current_mA = 0;
float power_mW = 0;
double totalEnergy_mWs = 0.0; // --- NEW: For total energy calculation

// --- NEW: Timers for non-blocking updates ---
unsigned long lastReadTime = 0;
unsigned long lastOledUpdateTime = 0;
const int oledUpdateInterval = 500; // Update OLED every 500ms (2x per second)

// --- Setup Hardware ---
Adafruit_INA219 ina219;
AsyncWebServer server(80); // Create a server on port 80

// --- NEW Function: Update OLED Display (Redesigned for new data) ---
void updateOLED() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  
  display.setCursor(0, 0);
  display.println("--- ENERGY MONITOR ---");
  
  display.setCursor(0, 12);
  display.print("Volt: "); display.print(voltage_V, 2); display.println(" V");
  
  display.setCursor(0, 22);
  display.print("Curr: "); display.print(current_mA, 1); display.println(" mA");
  
  display.setCursor(0, 32);
  display.print("Powr: "); display.print(power_mW, 1); display.println(" mW");

  // --- NEW: Display Total Energy in Joules (1 J = 1000 mWs) ---
  display.setCursor(0, 42);
  display.print("Nrg:  "); display.print(totalEnergy_mWs / 1000.0, 3); display.println(" J");
  
  display.setCursor(0, 54); // Moved IP to bottom
  display.print("IP: "); display.println(WiFi.localIP());
  
  display.display(); // Send data to OLED
}

// --- MODIFIED: Function to build the HTML page ---
String buildHtmlPage() {
  String html = "<!DOCTYPE HTML><html><head>";
  html += "<title>ESP32 Energy Monitor</title>";
  // --- MODIFIED: Refresh every 0.5 seconds ---
  html += "<meta http-equiv='refresh' content='0.5'>"; 
  html += "<meta name='viewport' content='width=device-width, initial-scale=1'>";
  html += "<style>";
  html += "body { font-family: Arial, sans-serif; text-align: center; background-color: #f0f0f0; }";
  html += "h1 { color: #333; }";
  html += ".card { background-color: white; padding: 20px; margin: 20px auto; border-radius: 8px; box-shadow: 0 4px 8px rgba(0,0,0,0.1); max-width: 300px; }";
  html += "p { font-size: 1.5rem; margin: 10px; }";
  html += ".label { font-size: 1rem; color: #777; }";
  html += "</style></head><body>";
  
  html += "<h1>ESP32 Energy Monitor</h1>";
  html += "<div class='card'>";
  html += "<p><span class='label'>Voltage: </span>" + String(voltage_V, 2) + " V</p>";
  html += "<p><span class='label'>Current: </span>" + String(current_mA, 1) + " mA</p>";
  html += "<p><span class='label'>Power: </span>" + String(power_mW, 1) + " mW</p>";
  // --- NEW: Add total energy to webpage (in Joules) ---
  html += "<p><span class='label'>Total Energy: </span>" + String(totalEnergy_mWs / 1000.0, 3) + " J</p>";
  html += "</div></body></html>";
  
  return html;
}

// --- MODIFIED: Function to read from INA219 and calculate energy ---
void readINA219() {
  // --- NEW: Calculate time interval for energy calculation ---
  unsigned long now = millis();
  unsigned long timeInterval_ms = now - lastReadTime;

  // Read new values
  voltage_V = ina219.getBusVoltage_V();
  current_mA = ina219.getCurrent_mA();
  power_mW = ina219.getPower_mW();
  
  // --- NEW: Calculate energy and add to total ---
  // Energy (mWs) = Power (mW) * Time (s)
  if (lastReadTime > 0) { // Don't calculate on first run
    double energyInterval_mWs = power_mW * (timeInterval_ms / 1000.0);
    totalEnergy_mWs += energyInterval_mWs;
  }
  
  lastReadTime = now; // Update the last read time

  // We no longer update the OLED from here. It's done in the loop.
  // We also remove the Serial.print spam from here.
}

void setup() {
  Serial.begin(115200);
  Wire.begin();
  
  // --- Initialize INA219 ---
  if (!ina219.begin()) {
    Serial.println("Failed to find INA219 chip");
    while (1) { delay(10); }
  }
  Serial.println("INA219 Initialized.");

  // --- Initialize OLED Display ---
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { 
    Serial.println(F("SSD1306 allocation failed"));
    for(;;);
  }
  Serial.println("OLED Initialized.");
  
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0,0);
  display.println("Connecting to WiFi...");
  display.display();

  // --- Connect to Wi-Fi ---
  WiFi.begin(ssid, password);
  Serial.print("Connecting to Wi-Fi...");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nConnected!");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP()); 

  // --- MODIFIED: Setup timers and do initial reads ---
  Serial.println("Reading initial values...");
  lastReadTime = millis(); // Set initial time
  readINA219(); // Get first reading
  
  lastOledUpdateTime = millis(); // Set initial OLED time
  updateOLED(); // Show initial data (IP, zeroed values)
  
  // --- MODIFIED: Web server no longer calls readINA219() ---
  // It just displays the global variables that the loop() is updating.
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    String html = buildHtmlPage();
    request->send(200, "text/html", html);
  });

  // Start the server
  server.begin();
  Serial.println("HTTP server started.");
}

// --- MODIFIED: loop() now does all the work! ---
void loop() {
  // 1. Read the sensor as fast as possible for accurate energy calculation
  readINA219(); 

  // 2. Check if it's time to update the OLED display
  unsigned long now = millis();
  if (now - lastOledUpdateTime > oledUpdateInterval) {
    updateOLED(); // Update the display
    lastOledUpdateTime = now; // Reset the OLED timer
    
    // --- NEW: Throttled Serial Print for debugging ---
    Serial.print("V: "); Serial.print(voltage_V); Serial.print(" V | ");
    Serial.print("C: "); Serial.print(current_mA); Serial.print(" mA | ");
    Serial.print("P: "); Serial.print(power_mW); Serial.print(" mW | ");
    Serial.print("E: "); Serial.print(totalEnergy_mWs / 1000.0, 3); Serial.println(" J");
  }
  
  // The async web server handles itself in the background.
}
