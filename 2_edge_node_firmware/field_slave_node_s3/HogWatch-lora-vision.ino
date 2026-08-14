#include <Arduino.h>
#include <Wire.h>
#include <Seeed_Arduino_SSCMA.h>

SSCMA AI;

#define SSCMA_CONNECT_TO_XIAO_S3         1
#define SSCMA_CONNECT_TO_GORVE_VISION_AI 0
#define RELAY_PIN                        21 //  RELAY PIN

// LoRa Pin definitions for XIAO ESP32-C6/S3
#define TX_PIN 43  // D6 - Connect to Wio-E5 RX
#define RX_PIN 44  // D7 - Connect to Wio-E5 TX

// LoRa communication variables
static char recv_buf[512];
static bool is_lora_ready = false;
static bool wild_boar_detected = false;

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

static int lora_send_detection(bool detected)
{
    static uint16_t count = 0;
    int ret = 0;
    char data[32];
    char cmd[128];

    memset(data, 0, sizeof(data));
    // Send detection status: 0001 = detected, 0000 = not detected
    sprintf(data, "%04X", detected ? 1 : 0);
    sprintf(cmd, "AT+TEST=TXLRPKT,\"5345454544%s\"\r\n", data);

    ret = at_send_check_response("TX DONE", 2000, cmd);
    if (ret == 1)
    {
        count++;
        Serial.print("LoRa TX: Wild boar ");
        Serial.println(detected ? "DETECTED" : "NOT DETECTED");
        return 1;
    }
    else
    {
        Serial.println("LoRa TX: Send failed!");
        return 0;
    }
}

void setup()
{
    // Initialize serial communication
    Serial.begin(115200);
    while (!Serial) delay(1000); // Wait for serial connection

    Serial.println("=== Grove Vision AI + LoRa Wild Boar Detection ===");

    // Initialize I2C and SSCMA
#if SSCMA_CONNECT_TO_XIAO_S3
    Wire.begin();
    AI.begin(&Wire);
#endif

#if SSCMA_CONNECT_TO_GORVE_VISION_AI
    Wire.begin();
    AI.begin(&Wire, D3);
#endif

    // Initialize LED pin
    pinMode(RELAY_PIN, OUTPUT);
    digitalWrite(RELAY_PIN, HIGH); 

    // Initialize LoRa module
    Serial1.begin(9600, SERIAL_8N1, RX_PIN, TX_PIN);
    Serial.println("Initializing LoRa module...");
    
    if (at_send_check_response("+AT: OK", 1000, "AT\r\n"))
    {
        is_lora_ready = true;
        Serial.println("LoRa: Wio-E5 module found!");
        
        // Configure LoRa parameters
        at_send_check_response("+MODE: TEST", 1000, "AT+MODE=TEST\r\n");
        at_send_check_response("+TEST: RFCFG", 1000, "AT+TEST=RFCFG,866,SF12,125,12,15,14,ON,OFF,OFF\r\n");
        delay(200);
        
        Serial.println("LoRa: Module configured successfully!");
    }
    else
    {
        is_lora_ready = false;
        Serial.println("LoRa: No Wio-E5 module found! Running in vision-only mode.");
    }

    Serial.println("System ready. Starting detection...\n");
}

void loop()
{
    bool personDetected = false; // Flag to track person detection
    static bool lastDetectionState = false;
    static unsigned long lastTransmission = 0;
    const unsigned long TRANSMISSION_INTERVAL = 5000; // Send every 5 seconds

    if (!AI.invoke(1, false, false))
    {
        // Check for person detection in bounding boxes
        for (int i = 0; i < AI.boxes().size(); i++)
        {
            // Set flag if person (target == 0) is detected
            if (AI.boxes()[i].target == 0)
            {
                personDetected = true;
                Serial.print("Vision: Wild boar detected! Score=");
                Serial.print(AI.boxes()[i].score);
                Serial.print(", Box=[");
                Serial.print(AI.boxes()[i].x);
                Serial.print(",");
                Serial.print(AI.boxes()[i].y);
                Serial.print(",");
                Serial.print(AI.boxes()[i].w);
                Serial.print(",");
                Serial.print(AI.boxes()[i].h);
                Serial.println("]");
                break; // Only need to detect one
            }
        }

        // Control LED based on person detection
        if (personDetected)
        {
            digitalWrite(RELAY_PIN, LOW); 
            wild_boar_detected = true;
        }
        else
        {
            digitalWrite(RELAY_PIN, HIGH); 
            wild_boar_detected = false;
        }

        // Send LoRa message more frequently when detection is active
        unsigned long currentInterval = personDetected ? 2000 : TRANSMISSION_INTERVAL; // 2s when detected, 5s when clear
        
        if (is_lora_ready && 
            ((personDetected != lastDetectionState) || 
             (millis() - lastTransmission > currentInterval)))
        {
            if (lora_send_detection(personDetected))
            {
                lastTransmission = millis();
                lastDetectionState = personDetected;
            }
        }
    }
    else
    {
        // AI invoke failed
        digitalWrite(RELAY_PIN, HIGH); 
        wild_boar_detected = false;
        
        // Send "no detection" message periodically even if AI fails
        if (is_lora_ready && (millis() - lastTransmission > TRANSMISSION_INTERVAL))
        {
            if (lora_send_detection(false))
            {
                lastTransmission = millis();
                lastDetectionState = false;
            }
        }
        
        Serial.println("Vision: AI invoke failed");
    }

    // Optional: Add delay to control inference rate
    delay(100); // Adjust based on desired frame rate (e.g., 50-500ms)
}