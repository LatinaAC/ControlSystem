# PACKET TALK
### A Portable AI-Driven Speech-to-Speech Translation System

Computer Engineering Student  
University of Batangas – Lipa Campus

Packet Talk is a portable AI translator that treats conversation like network data. Using an ESP32, I2S hardware, and a local AI engine, it captures speech "packets," translates them through an AI-driven agent, and plays back the result in real-time. It transforms complex human language into a streamlined, low-latency data exchange, bridging language barriers with a compact, efficient, and privacy-first design.

---

## Design Decisions & Hardware Notes

- **AI Engine Choice (Ollama vs. LM Studio):** I initially explored LM Studio for local AI inference. However, we transitioned to **Ollama** because its lightweight, command-line-first architecture allows for seamless API integration with our Flask server. This background service approach provides a much faster "packet" processing time compared to a GUI-heavy application.
- **Audio Output (Laptop Speaker vs. External Amplifier):** The original design included an external amplifier and speaker module. However, during testing, these components were prone to overloading and distortion at the required volume levels. To guarantee audio clarity, high fidelity, and system longevity, I opted to route the final translated audio output directly through the host laptop's internal speaker system.

---

## Features

- Real-time bidirectional translation (English ↔ Tagalog)
- Speech-to-Speech (S2S) processing pipeline
- Local AI processing using Ollama and DeepSeek-R1
- Faster-Whisper integration for accurate speech-to-text
- Compact ESP32-based hardware architecture
- Real-time translation display on 16×2 I2C LCD
- Python-based Flask middleware server

---

## Hardware Components

- ESP32 Development Board
- INMP441 I2S Microphone
- 16×2 I2C LCD
- Push Button (for Push-to-Talk)
- Breadboard
- Jumper Wires
- 5V Power Supply

---

## Software Requirements

### ESP32 Libraries

Install the following libraries in your Arduino IDE:

- `WiFi.h`
- `HTTPClient.h`
- `ArduinoJson`
- `LiquidCrystal_I2C`
- `driver/i2s.h` (Internal ESP-IDF)

### Python Packages

Install the required Python packages via terminal:

```bash
pip install flask ollama faster-whisper pyttsx3 pygame numpy