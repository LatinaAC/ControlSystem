#include <Arduino.h>
#include <driver/i2s.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <U8g2lib.h>
#include <Wire.h>

// --- Network Settings ---
const char* WIFI_SSID = "Converge_2.4GHz_f5WV"; 
const char* WIFI_PASS = "hUFgvn8G"; 
const char* SERVER_URL = "http://192.168.100.3:5006/translate"; // Port changed to 5006 to match server

// --- Pin Map ---
#define BUTTON_PIN 13  
#define SCK_PIN    26  
#define WS_PIN     25  
#define MIC_SD     33  
#define I2C_SDA    21
#define I2C_SCL    22

U8G2_ST7567_ENH_DG128064I_F_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE); 

const i2s_port_t I2S_PORT = I2S_NUM_0;
const size_t SAMPLES_PER_SEC = 16000;
const size_t RECORD_TIME_SECS = 3;

const size_t CHUNK_SAMPLES = 256; 
int16_t transfer_chunk_buf[CHUNK_SAMPLES];

bool i2s_is_installed = false;

void stopI2S_Microphone() {
    if (i2s_is_installed) {
        i2s_driver_uninstall(I2S_PORT);
        i2s_is_installed = false;
    }
}

void initI2S_Record() {
    stopI2S_Microphone();

    i2s_config_t i2s_config = {
        .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX), 
        .sample_rate = SAMPLES_PER_SEC,
        .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT, 
        .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
        .communication_format = I2S_COMM_FORMAT_STAND_I2S, 
        .intr_alloc_flags = 0, // Auto-allocate safe vector
        .dma_buf_count = 8,
        .dma_buf_len = 128
    };
    
    i2s_driver_install(I2S_PORT, &i2s_config, 0, NULL);
    
    i2s_pin_config_t pin_config = {
        .bck_io_num = SCK_PIN,
        .ws_io_num = WS_PIN,
        .data_out_num = I2S_PIN_NO_CHANGE,
        .data_in_num = MIC_SD
    };
    
    i2s_set_pin(I2S_PORT, &pin_config);
    i2s_is_installed = true;
}

void updateScreen(String title, String body) {
    u8g2.clearBuffer();
    u8g2.drawFrame(0, 0, 128, 64);
    u8g2.setFont(u8g2_font_6x12_tf);
    u8g2.drawStr(8, 15, title.c_str());
    u8g2.setFont(u8g2_font_5x8_tf);
    u8g2.drawStr(8, 35, body.substring(0, 24).c_str());
    u8g2.drawStr(8, 48, body.substring(24, 48).c_str());
    u8g2.sendBuffer();
}

void setup() {
    Serial.begin(115200);
    delay(1000); 
    Serial.println("\n==================================================");
    Serial.println("[SYSTEM START] Initializing AI Laptop Node Mode...");
    Serial.println("==================================================");
    
    pinMode(BUTTON_PIN, INPUT_PULLUP);
    
    Wire.begin(I2C_SDA, I2C_SCL);
    u8g2.setI2CAddress(0x3F * 2);
    u8g2.begin();
    u8g2.setContrast(-10); 
    
    updateScreen("SYSTEM BOOT", "Connecting to Wi-Fi...");
    
    WiFi.disconnect(true);
    delay(200);
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASS);
    
    while (WiFi.status() != WL_CONNECTED) { 
        delay(500); 
        Serial.print("."); 
    }
    
    Serial.println("\n[Wi-Fi Link] Status: CONNECTED");
    updateScreen("AI TRANSLATOR", "Ready. Press G13!");
}

void loop() {
    if (WiFi.status() != WL_CONNECTED) {
        WiFi.begin(WIFI_SSID, WIFI_PASS);
        while(WiFi.status() != WL_CONNECTED) { delay(100); }
    }

    // Active Low press detection
    if (digitalRead(BUTTON_PIN) == LOW) {
        delay(150); // Debounce
        
        Serial.println("\n--------------------------------------------------");
        Serial.println("[Button Event] -> Recording Voice Activity triggered.");

        HTTPClient http;
        http.setTimeout(45000); 
        http.begin(SERVER_URL);
        
        const char* headerKeys[] = {"X-Translation-Text"};
        http.collectHeaders(headerKeys, 1);
        http.addHeader("Content-Type", "application/octet-stream");
        
        size_t total_bytes_to_send = RECORD_TIME_SECS * SAMPLES_PER_SEC * sizeof(int16_t);
        http.addHeader("Content-Length", String(total_bytes_to_send));
        
        updateScreen("RECORDING", "Speak clear...");
        initI2S_Record(); 
        
        size_t total_samples_sent = 0;
        size_t max_samples_needed = RECORD_TIME_SECS * SAMPLES_PER_SEC;
        int16_t rx_raw_buf[64];
        size_t bytes_read = 0;
        unsigned long last_vu_print = 0;

        uint8_t* raw_send_buffer = (uint8_t*)malloc(total_bytes_to_send);
        if (raw_send_buffer == NULL) {
            Serial.println("[ERROR] Memory Overload Allocation Failed!");
            return;
        }

        size_t memory_offset = 0;

        // Recording Loop
        while (total_samples_sent < max_samples_needed) {
            size_t chunk_collected = 0;
            int32_t chunk_max_volume = 0;

            while (chunk_collected < CHUNK_SAMPLES && total_samples_sent < max_samples_needed) {
                i2s_read(I2S_PORT, rx_raw_buf, sizeof(rx_raw_buf), &bytes_read, portMAX_DELAY);
                size_t samples_read = bytes_read / 2;
                
                for (size_t i = 0; i < samples_read; i++) {
                    if (chunk_collected >= CHUNK_SAMPLES) break;
                    
                    int16_t raw_sample = rx_raw_buf[i];
                    int32_t sample_boosted = (int32_t)raw_sample * 4;  // Apply mic hardware gain boost
                    
                    if (sample_boosted > 32767)  sample_boosted = 32767;
                    if (sample_boosted < -32768) sample_boosted = -32768;
                    
                    int32_t abs_sample = abs(sample_boosted);
                    if(abs_sample > chunk_max_volume) chunk_max_volume = abs_sample;
                    
                    transfer_chunk_buf[chunk_collected++] = (int16_t)sample_boosted;
                    total_samples_sent++;
                }
            }
            
            memcpy(raw_send_buffer + memory_offset, transfer_chunk_buf, chunk_collected * sizeof(int16_t));
            memory_offset += (chunk_collected * sizeof(int16_t));

            if (millis() - last_vu_print > 150) {
                last_vu_print = millis();
                int bar_length = (chunk_max_volume * 15) / 32768;
                Serial.print("[Mic VU] [");
                for(int b=0; b<15; b++) Serial.print(b < bar_length ? "#" : ".");
                Serial.print("] Peak: ");
                Serial.println(chunk_max_volume);
            }
            vTaskDelay(1); 
        }

        stopI2S_Microphone(); // Close Mic hardware lines before networking
        updateScreen("THINKING", "Processing AI...");
        
        int httpResponseCode = http.sendRequest("POST", raw_send_buffer, total_bytes_to_send);
        free(raw_send_buffer); 

        Serial.print("[HTTP Client] Code response received: ");
        Serial.println(httpResponseCode);

        if (httpResponseCode == 200) {
            String translationText = http.header("X-Translation-Text");
            if(translationText.length() == 0) {
                translationText = http.getString(); // Fallback to raw text string payload body
            }
            
            Serial.print("[AI Response Displayed] -> \"");
            Serial.print(translationText);
            Serial.println("\"");
            
            // Output translation straight onto screen
            updateScreen("TRANSLATION", translationText);
            Serial.println("[System Info] -> Audio output routing running via Laptop speakers.");
        } else {
            Serial.print("[HTTP Error] Code: ");
            Serial.println(httpResponseCode);
            updateScreen("SERVER ERROR", "Check Python Log");
        }
        http.end();
        
        delay(4000); // Leave it on screen long enough to read
        updateScreen("AI TRANSLATOR", "Ready. Press G13!");
    }
}