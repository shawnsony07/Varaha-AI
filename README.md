# Varaha AI — Wiring Diagrams

> **Note:** GitHub renders Mermaid diagrams natively. Open this file on GitHub to see the interactive, image-embedded diagrams below.

---

## 1. Field Node Pictorial Wiring Diagram

```mermaid
graph TD
    classDef default fill:#ffffff,stroke:#333,stroke-width:2px,color:#000000;
    linkStyle default stroke:#333,stroke-width:3px;

    BATT["<img src='https://raw.githubusercontent.com/shawnsony07/Varaha-AI/bac9921a6bf9239d70827d1d07278e7497fe2a8e/images/14.8V_Li-ion_Battery.png' width='100' height='100'/><br><b>14.8V Li-ion Battery</b>"]
    BUCK["<img src='https://raw.githubusercontent.com/shawnsony07/Varaha-AI/bac9921a6bf9239d70827d1d07278e7497fe2a8e/images/Buck_Converter.png' width='100' height='100'/><br><b>Buck Converter</b>"]
    RELAY["<img src='https://raw.githubusercontent.com/shawnsony07/Varaha-AI/bac9921a6bf9239d70827d1d07278e7497fe2a8e/images/Relay_Module.png' width='100' height='100'/><br><b>Relay Module</b>"]
    S3["<img src='https://raw.githubusercontent.com/shawnsony07/Varaha-AI/bac9921a6bf9239d70827d1d07278e7497fe2a8e/images/XIAO_ESP32-S3_Plus.png' width='100' height='100'/><br><b>XIAO ESP32-S3 Plus</b>"]
    VAI["<img src='https://raw.githubusercontent.com/shawnsony07/Varaha-AI/bac9921a6bf9239d70827d1d07278e7497fe2a8e/images/Seeed-Grove-Vision-AI.png' width='100' height='100'/><br><b>Grove Vision AI V2</b>"]
    LORA["<img src='https://raw.githubusercontent.com/shawnsony07/Varaha-AI/bac9921a6bf9239d70827d1d07278e7497fe2a8e/images/Grove_LoRa_E5.png' width='100' height='100'/><br><b>Grove LoRa E5 (TX)</b>"]

    BATT -- "14.8V Power" --> RELAY
    RELAY -- "Switched Power" --> ULTRASONIC["<b>Analog Ultrasonic Circuit</b>"]
    BATT -- "14.8V Input" --> BUCK
    BUCK -- "Regulated Power (5V)" --> S3
    S3 -- "Power Rail (3.3V/5V)" --> VAI

    VAI -- "I2C SDA / SCL" --> S3
    S3 -- "Pin 43 (TX) ➜ LoRa RX<br>Pin 44 (RX) ⟵ LoRa TX" --> LORA
    S3 -- "Pin 21 ➜ Relay Trigger" --> RELAY
```

---

## 2. Base Station Pictorial Wiring Diagram

```mermaid
graph TD
    classDef default fill:#ffffff,stroke:#333,stroke-width:2px,color:#000000;
    linkStyle default stroke:#333,stroke-width:3px;

    BATT2["<img src='https://raw.githubusercontent.com/shawnsony07/Varaha-AI/bac9921a6bf9239d70827d1d07278e7497fe2a8e/images/Dual_3.7V_18650_Batteries.png' width='100' height='100'/><br><b>Dual 3.7V 18650</b>"]
    BUCK2["<img src='https://raw.githubusercontent.com/shawnsony07/Varaha-AI/bac9921a6bf9239d70827d1d07278e7497fe2a8e/images/Buck_Converter.png' width='100' height='100'/><br><b>Buck Converter</b>"]
    LORA2["<img src='https://raw.githubusercontent.com/shawnsony07/Varaha-AI/bac9921a6bf9239d70827d1d07278e7497fe2a8e/images/Grove_LoRa_E5.png' width='100' height='100'/><br><b>Grove LoRa E5 (RX)</b>"]
    C6["<img src='https://raw.githubusercontent.com/shawnsony07/Varaha-AI/bac9921a6bf9239d70827d1d07278e7497fe2a8e/images/XIAO_ESP32-C6.png' width='100' height='100'/><br><b>XIAO ESP32-C6</b>"]
    WIO["<img src='https://raw.githubusercontent.com/shawnsony07/Varaha-AI/bac9921a6bf9239d70827d1d07278e7497fe2a8e/images/Wio_Terminal.png' width='100' height='100'/><br><b>Wio Terminal Display</b>"]

    BATT2 -- "VCC / GND" --> BUCK2
    BUCK2 -- "5V / GND" --> C6
    BUCK2 -- "5V / GND" --> WIO

    LORA2 -- "LoRa TX ➜ Pin 17 (RX)<br>LoRa RX ⟵ Pin 16 (TX)" --> C6
    C6 -- "Pin 22 (SDA) ➜ Wio SDA<br>Pin 23 (SCL) ➜ Wio SCL" --> WIO
```
