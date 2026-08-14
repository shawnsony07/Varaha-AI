#include <Arduino.h>
#include <Wire.h>

// Pin definitions for XIAO ESP32-C6/S3
#define TX_PIN 16  // D6 - Connect to Wio-E5 RX
#define RX_PIN 17  // D7 - Connect to Wio-E5 TX

// I2C Configuration
#define I2C_SDA_PIN 22  // D4 - Connect to Wio Terminal SDA
#define I2C_SCL_PIN 23  // D5 - Connect to Wio Terminal SCL
#define WIO_TERMINAL_I2C_ADDR 0x42

// Master node configuration
static char recv_buf[512];
static bool is_exist = false;
static unsigned long lastHeartbeat = 0;
static unsigned long lastDetectionTime = 0;
static int consecutiveFailures = 0;
static int totalDetections = 0;
static bool currentFieldStatus = true; // true = clear, false = boar detected
static int currentRSSI = 0;
static int currentSNR = 0;

// I2C Data Structure
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

static int at_send_check_response(char *p_ack, int timeout_ms, char*p_cmd, ...)
{
    int ch = 0;
    int index = 0;
    int startMillis = 0;
    va_list args;
    memset(recv_buf, 0, sizeof(recv_buf));
    va_start(args, p_cmd);
    Serial1.printf(p_cmd, args);
    va_end(args);
    delay(200);
    startMillis = millis();

    if (p_ack == NULL)
    {
        return 0;
    }

    do
    {
        while (Serial1.available() > 0)
        {
            ch = Serial1.read();
            recv_buf[index++] = ch;
            delay(2);
        }

        if (strstr(recv_buf, p_ack) != NULL)
        {
            return 1;
        }

    } while (millis() - startMillis < timeout_ms);
    return 0;
}

static int recv_parse_detection(void)
{
    char ch;
    int index = 0;
    memset(recv_buf, 0, sizeof(recv_buf));
    while (Serial1.available() > 0)
    {
        ch = Serial1.read();
        recv_buf[index++] = ch;
        delay(2);
    }

    if (index)
    {
        char *p_start = NULL;
        char data[32] = {0};
        int rssi = 0;
        int snr = 0;

        p_start = strstr(recv_buf, "+TEST: RX \"5345454544");
        if (p_start)
        {
            p_start = strstr(recv_buf, "5345454544");
            if (p_start && (1 == sscanf(p_start, "5345454544%s", data)))
            {
                data[4] = 0;
                
                // Parse detection status
                int detection_status = (int)strtol(data, NULL, 16);
                
                // Get signal quality
                p_start = strstr(recv_buf, "RSSI:");
                if (p_start && (1 == sscanf(p_start, "RSSI:%d,", &rssi)))
                {
                    currentRSSI = rssi;
                }
                
                p_start = strstr(recv_buf, "SNR:");
                if (p_start && (1 == sscanf(p_start, "SNR:%d", &snr)))
                {
                    currentSNR = snr;
                }

                // Update status
                if (detection_status == 1)
                {
                    currentFieldStatus = false; // Boar detected
                    totalDetections++;
                    lastDetectionTime = millis();
                    Serial.print("🐗 WILD BOAR DETECTED! ");
                }
                else
                {
                    currentFieldStatus = true; // Field clear
                    Serial.print("✅ Area Clear ");
                }

                // Print meaningful information
                Serial.print("[");
                Serial.print(millis()/1000);
                Serial.print("s] ");
                Serial.print("| Signal: RSSI=");
                Serial.print(rssi);
                Serial.print("dBm, SNR=");
                Serial.print(snr);
                Serial.println("dB");
                
                consecutiveFailures = 0; // Reset failure counter on successful reception
                
                // Send data to Wio Terminal via I2C
                sendDataToWioTerminal();
                
                return 1;
            }
        }
    }
    return 0;
}

static int node_recv(uint32_t timeout_ms)
{
    at_send_check_response("+TEST: RXLRPKT", 1000, "AT+TEST=RXLRPKT\r\n");
    int startMillis = millis();
    do
    {
        if (recv_parse_detection())
        {
            return 1;
        }
    } while (millis() - startMillis < timeout_ms);
    return 0;
}

void sendDataToWioTerminal()
{
    DetectionData data;
    
    // Prepare data structure
    data.fieldClear = currentFieldStatus ? 1 : 0;
    data.totalDetections = min(totalDetections, 255); // Limit to uint8_t range
    data.rssi = constrain(currentRSSI, -128, 127);
    data.snr = constrain(currentSNR, -128, 127);
    data.commFailures = min(consecutiveFailures, 255);
    data.lastDetectionTime = lastDetectionTime > 0 ? (millis() - lastDetectionTime) / 1000 : 0;
    data.systemUptime = millis() / 1000;
    data.loraConnected = is_exist ? 1 : 0;
    
    // Send data via I2C
    Wire.beginTransmission(WIO_TERMINAL_I2C_ADDR);
    Wire.write((uint8_t*)&data, sizeof(DetectionData));
    int result = Wire.endTransmission();
    
    if (result != 0) {
        Serial.print("I2C transmission error: ");
        Serial.println(result);
    } else {
        Serial.println("📱 Data sent to Wio Terminal");
    }
}

static void print_status_summary()
{
    unsigned long currentTime = millis();
    Serial.println("\n=== WILD BOAR MONITORING STATUS ===");
    Serial.print("System Uptime: ");
    Serial.print(currentTime / 1000);
    Serial.println(" seconds");
    
    Serial.print("Total Detections: ");
    Serial.println(totalDetections);
    
    if (lastDetectionTime > 0)
    {
        Serial.print("Last Detection: ");
        Serial.print((currentTime - lastDetectionTime) / 1000);
        Serial.println(" seconds ago");
    }
    else
    {
        Serial.println("Last Detection: None since startup");
    }
    
    Serial.print("Current Status: ");
    Serial.println(currentFieldStatus ? "Field Clear ✅" : "Boar Detected 🐗");
    
    if (consecutiveFailures > 0)
    {
        Serial.print("⚠️ Communication Issues: ");
        Serial.print(consecutiveFailures);
        Serial.println(" consecutive failures");
    }
    else
    {
        Serial.println("📡 Communication: OK");
    }
    Serial.println("====================================\n");
    
    // Send periodic update to Wio Terminal
    sendDataToWioTerminal();
}

void setup(void)
{
    Serial.begin(115200);
    Serial1.begin(9600, SERIAL_8N1, RX_PIN, TX_PIN);
    
    // Initialize I2C as master
    Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);
    Wire.setClock(100000); // 100kHz
    
    Serial.println("=== Wild Boar Detection Master Station ===");
    Serial.println("Initializing I2C communication...");
    Serial.print("I2C SDA: Pin ");
    Serial.print(I2C_SDA_PIN);
    Serial.print(", SCL: Pin ");
    Serial.println(I2C_SCL_PIN);
    Serial.print("Wio Terminal I2C Address: 0x");
    Serial.println(WIO_TERMINAL_I2C_ADDR, HEX);
    
    Serial.println("Connecting to LoRa module...");
    
    // Initialize LoRa module
    if (at_send_check_response("+AT: OK", 1000, "AT\r\n"))
    {
        is_exist = true;
        Serial.println("✅ LoRa module connected successfully!");
        
        // Configure LoRa parameters
        at_send_check_response("+MODE: TEST", 1000, "AT+MODE=TEST\r\n");
        at_send_check_response("+TEST: RFCFG", 1000, "AT+TEST=RFCFG,866,SF12,125,12,15,14,ON,OFF,OFF\r\n");
        delay(200);
        
        Serial.println("📡 LoRa parameters configured");
        Serial.println("🔍 Starting wild boar monitoring...\n");
    }
    else
    {
        is_exist = false;
        Serial.println("❌ LoRa module not found!");
    }
    
    lastHeartbeat = millis();
    
    // Send initial data to Wio Terminal
    sendDataToWioTerminal();
}

void loop(void)
{
    if (is_exist)
    {
        // Listen for detection messages with longer timeout for better reliability
        if (!node_recv(5000))  // Increased from 3000 to 5000ms
        {
            consecutiveFailures++;
            Serial.print("[");
            Serial.print(millis()/1000);
            Serial.print("s] ");
            Serial.print("📶 No signal from field sensor (timeout #");
            Serial.print(consecutiveFailures);
            Serial.println(")");
            
            // Send timeout update to Wio Terminal
            sendDataToWioTerminal();
        }

        // Print periodic status summary
        if (millis() - lastHeartbeat > 60000) // Every 60 seconds
        {
            print_status_summary();
            lastHeartbeat = millis();
        }
    }
    else
    {
        Serial.println("❌ LoRa module not available - check connections");
        sendDataToWioTerminal(); // Update Wio Terminal with disconnected status
        delay(5000);
    }
    
    delay(100); // Small delay to prevent overwhelming I2C
}