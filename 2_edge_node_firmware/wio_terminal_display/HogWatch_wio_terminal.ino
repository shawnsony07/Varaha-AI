#include "TFT_eSPI.h"
#include <Wire.h>

TFT_eSPI tft = TFT_eSPI();

// I2C Configuration
#define I2C_SLAVE_ADDR 0x42

// Detection data structure (must match ESP32 side)
struct DetectionData {
    uint8_t fieldClear;        // 1 = clear, 0 = boar detected
    uint8_t totalDetections;   // Total detections count
    int8_t rssi;              // RSSI value
    int8_t snr;               // SNR value
    uint8_t commFailures;     // Communication failure count
    uint32_t lastDetectionTime; // Time since last detection (seconds)
    uint32_t systemUptime;    // System uptime (seconds)
    uint8_t loraConnected;    // 1 = connected, 0 = disconnected
};

// Display state
DetectionData currentData;
unsigned long lastI2CUpdate = 0;
unsigned long lastDisplayUpdate = 0;
bool dataReceived = false;
bool blinkState = false;

// Colors
#define COLOR_SAFE     0x07E0   // Green
#define COLOR_DANGER   0xF800   // Red
#define COLOR_WARNING  0xFFE0   // Yellow
#define COLOR_BG       0x0000   // Black
#define COLOR_TEXT     0xFFFF   // White
#define COLOR_GRAY     0x8410   // Gray
#define COLOR_BLUE     0x001F   // Blue

// Screen dimensions
#define SCREEN_WIDTH  320
#define SCREEN_HEIGHT 240

void setup() {
    Serial.begin(115200);
    
    // Initialize I2C as slave
    Wire.begin(I2C_SLAVE_ADDR);
    Wire.onReceive(receiveEvent);
    
    // Initialize display
    tft.init();
    tft.setRotation(3);
    tft.fillScreen(COLOR_BG);
    
    // Initialize data structure with defaults
    memset(&currentData, 0, sizeof(DetectionData));
    currentData.fieldClear = 1; // Start with field clear
    
    drawInterface();
    
    Serial.println("Wio Terminal Wild Boar Display System Started");
    Serial.print("I2C Slave Address: 0x");
    Serial.println(I2C_SLAVE_ADDR, HEX);
}

void loop() {
    unsigned long currentTime = millis();
    
    // Update display every 500ms
    if (currentTime - lastDisplayUpdate > 500) {
        updateDisplay();
        lastDisplayUpdate = currentTime;
        blinkState = !blinkState; // Toggle blink state
    }
    
    // Check for I2C communication timeout
    if (dataReceived && (currentTime - lastI2CUpdate > 10000)) {
        // No data received for 10 seconds - show communication error
        Serial.println("I2C communication timeout!");
    }
    
    delay(50);
}

void receiveEvent(int bytes) {
    if (bytes == sizeof(DetectionData)) {
        Wire.readBytes((uint8_t*)&currentData, sizeof(DetectionData));
        lastI2CUpdate = millis();
        dataReceived = true;
        
        Serial.println("--- I2C Data Received ---");
        Serial.print("Field Status: ");
        Serial.println(currentData.fieldClear ? "CLEAR" : "BOAR DETECTED");
        Serial.print("Total Detections: ");
        Serial.println(currentData.totalDetections);
        Serial.print("RSSI: ");
        Serial.print(currentData.rssi);
        Serial.println(" dBm");
        Serial.print("SNR: ");
        Serial.print(currentData.snr);
        Serial.println(" dB");
        Serial.print("Comm Failures: ");
        Serial.println(currentData.commFailures);
        Serial.print("LoRa Connected: ");
        Serial.println(currentData.loraConnected ? "YES" : "NO");
        Serial.println("------------------------");
    } else {
        Serial.print("Invalid I2C data size received: ");
        Serial.println(bytes);
        // Clear buffer
        while (Wire.available()) {
            Wire.read();
        }
    }
}

void drawInterface() {
    tft.fillScreen(COLOR_BG);
    
    // Title
    tft.setTextColor(COLOR_TEXT);
    tft.setTextSize(2);
    tft.drawString("WILD BOAR DETECTION", 20, 10);
    
    // Draw border
    tft.drawRect(5, 5, SCREEN_WIDTH-10, SCREEN_HEIGHT-10, COLOR_GRAY);
    
    // Status area background
    tft.fillRoundRect(15, 40, SCREEN_WIDTH-30, 80, 10, COLOR_GRAY);
}

void updateDisplay() {
    // Clear status area
    tft.fillRoundRect(16, 41, SCREEN_WIDTH-32, 78, 9, COLOR_BG);
    
    // Check if we have valid data
    if (!dataReceived) {
        // Waiting for data
        tft.fillRoundRect(16, 41, SCREEN_WIDTH-32, 78, 9, COLOR_GRAY);
        tft.setTextColor(COLOR_TEXT);
        tft.setTextSize(2);
        tft.drawString("WAITING FOR DATA...", 40, 70);
        return;
    }
    
    // Check I2C communication status
    bool i2cTimeout = (millis() - lastI2CUpdate > 10000);
    
    if (i2cTimeout) {
        // Communication error
        tft.fillRoundRect(16, 41, SCREEN_WIDTH-32, 78, 9, COLOR_WARNING);
        tft.setTextColor(COLOR_BG);
        tft.setTextSize(2);
        tft.drawString("COMM ERROR!", 70, 70);
        drawConnectionStatus(false, false);
        return;
    }
    
    // Main status display
    if (currentData.fieldClear) {
        // Field is safe
        tft.fillRoundRect(16, 41, SCREEN_WIDTH-32, 78, 9, COLOR_SAFE);
        tft.setTextColor(COLOR_BG);
        tft.setTextSize(3);
        tft.drawString("FIELD CLEAR", 50, 60);
        
        // Draw safe icon (checkmark)
        drawCheckmark(250, 65, COLOR_BG);
        
    } else {
        // Wild boar detected
        uint16_t bgColor = (blinkState) ? COLOR_DANGER : COLOR_WARNING;
        tft.fillRoundRect(16, 41, SCREEN_WIDTH-32, 78, 9, bgColor);
        tft.setTextColor(COLOR_TEXT);
        tft.setTextSize(3);
        tft.drawString("BOAR DETECTED!", 30, 60);
        
        // Draw warning icon
        drawWarningIcon(250, 65, COLOR_TEXT);
    }
    
    // Information panel
    drawInfoPanel();
    
    // Connection status
    drawConnectionStatus(currentData.loraConnected, true);
}

void drawInfoPanel() {
    // Info background
    tft.fillRoundRect(15, 130, SCREEN_WIDTH-30, 85, 5, COLOR_GRAY);
    
    tft.setTextColor(COLOR_TEXT);
    tft.setTextSize(1);
    
    // System uptime
    String uptimeStr = "Uptime: " + formatTime(currentData.systemUptime);
    tft.drawString(uptimeStr, 25, 140);
    
    // Total detections
    String countStr = "Total Detections: " + String(currentData.totalDetections);
    tft.drawString(countStr, 25, 155);
    
    // Last detection time
    if (currentData.lastDetectionTime > 0) {
        String lastStr = "Last Detection: " + formatTime(currentData.lastDetectionTime) + " ago";
        tft.drawString(lastStr, 25, 170);
    } else {
        tft.drawString("Last Detection: None", 25, 170);
    }
    
    // Signal quality
    String signalStr = "Signal: RSSI=" + String(currentData.rssi) + "dBm, SNR=" + String(currentData.snr) + "dB";
    tft.drawString(signalStr, 25, 185);
    
    // Communication failures
    if (currentData.commFailures > 0) {
        String failStr = "Comm Failures: " + String(currentData.commFailures);
        tft.drawString(failStr, 25, 200);
    } else {
        tft.drawString("Communication: OK", 25, 200);
    }
}

void drawConnectionStatus(bool loraConnected, bool i2cConnected) {
    // LoRa connection indicator
    tft.fillCircle(280, 220, 8, loraConnected ? COLOR_SAFE : COLOR_DANGER);
    tft.setTextColor(COLOR_TEXT);
    tft.setTextSize(1);
    tft.drawString("LoRa", 250, 215);
    
    // I2C connection indicator
    tft.fillCircle(300, 220, 8, i2cConnected ? COLOR_SAFE : COLOR_DANGER);
    tft.drawString("I2C", 275, 215);
}

void drawCheckmark(int x, int y, uint16_t color) {
    // Draw checkmark symbol
    tft.drawLine(x-8, y, x-3, y+5, color);
    tft.drawLine(x-3, y+5, x+8, y-6, color);
    tft.drawLine(x-8, y+1, x-3, y+6, color);
    tft.drawLine(x-3, y+6, x+8, y-5, color);
}

void drawWarningIcon(int x, int y, uint16_t color) {
    // Draw triangle warning icon
    tft.drawTriangle(x, y-8, x-8, y+6, x+8, y+6, color);
    tft.fillTriangle(x, y-6, x-6, y+4, x+6, y+4, color);
    
    // Exclamation mark
    tft.fillRect(x-1, y-4, 3, 6, COLOR_BG);
    tft.fillRect(x-1, y+3, 3, 2, COLOR_BG);
}

String formatTime(uint32_t seconds) {
    uint32_t hours = seconds / 3600;
    uint32_t minutes = (seconds % 3600) / 60;
    uint32_t secs = seconds % 60;
    
    if (hours > 0) {
        return String(hours) + "h " + String(minutes) + "m " + String(secs) + "s";
    } else if (minutes > 0) {
        return String(minutes) + "m " + String(secs) + "s";
    } else {
        return String(secs) + "s";
    }
}