/*
 * ESP32-S3 TEST MIC INMP441 + SD CARD
 * 
 * Mục đích: Test ghi âm từ mic INMP441 và lưu vào thẻ SD
 * 
 * Chức năng:
 * 1. Khởi tạo I2S cho mic INMP441
 * 2. Khởi tạo SD Card
 * 3. Ghi âm 10 giây và lưu vào file /record.wav
 * 4. File WAV có thể phát trên PC
 */

#include <Arduino.h>
#include <driver/i2s.h>
#include <SPI.h>
#include <SD.h>

// ==== I2S PINS FOR ESP32-S3 + INMP441 ====
#define I2S_WS   8   // LRCL / WS
#define I2S_SD   7   // DOUT from INMP441
#define I2S_SCK  9   // BCLK / SCK

// ==== SD CARD PINS (SPI) ====
#define SD_MOSI  11
#define SD_MISO  13
#define SD_SCK   12
#define SD_CS    10

// ==== AUDIO SETTINGS ====
#define I2S_PORT        I2S_NUM_0
#define SAMPLE_RATE     16000          // 16 kHz
#define SAMPLE_BITS_MIC I2S_BITS_PER_SAMPLE_32BIT
#define SAMPLE_BITS_WAV 16             // 16-bit PCM
#define CHANNELS        1
#define RECORD_TIME_SEC 10             // Ghi 10 giây

// Tính số mẫu và số byte data
const uint32_t NUM_SAMPLES = SAMPLE_RATE * RECORD_TIME_SEC;
const uint32_t DATA_BYTES   = NUM_SAMPLES * (SAMPLE_BITS_WAV / 8);

// --------------------------------------------------------
// Ghi header WAV 44 bytes đầu file
// --------------------------------------------------------
void writeWavHeader(File &file,
                    uint32_t sampleRate,
                    uint16_t bitsPerSample,
                    uint16_t channels,
                    uint32_t dataBytes) {
  uint32_t chunkSize = 36 + dataBytes;
  uint16_t audioFormat = 1; // PCM
  
  // "RIFF"
  file.write((uint8_t*)"RIFF", 4);
  file.write((uint8_t*)&chunkSize, 4);
  file.write((uint8_t*)"WAVE", 4);
  
  // "fmt " chunk
  uint32_t subchunk1Size = 16;
  file.write((uint8_t*)"fmt ", 4);
  file.write((uint8_t*)&subchunk1Size, 4);
  file.write((uint8_t*)&audioFormat, 2);
  file.write((uint8_t*)&channels, 2);
  file.write((uint8_t*)&sampleRate, 4);
  
  uint32_t byteRate = sampleRate * channels * bitsPerSample / 8;
  uint16_t blockAlign = channels * bitsPerSample / 8;
  file.write((uint8_t*)&byteRate, 4);
  file.write((uint8_t*)&blockAlign, 2);
  file.write((uint8_t*)&bitsPerSample, 2);
  
  // "data" chunk
  file.write((uint8_t*)"data", 4);
  file.write((uint8_t*)&dataBytes, 4);
}

// --------------------------------------------------------
// Cấu hình I2S cho INMP441
// --------------------------------------------------------
void setupI2S() {
  i2s_config_t i2s_config = {
    .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),
    .sample_rate = SAMPLE_RATE,
    .bits_per_sample = SAMPLE_BITS_MIC,
    .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT, // L/R nối GND -> kênh LEFT
    .communication_format = I2S_COMM_FORMAT_STAND_I2S,
    .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
    .dma_buf_count = 8,
    .dma_buf_len = 256,
    .use_apll = true,
    .tx_desc_auto_clear = false,
    .fixed_mclk = 0
  };

  i2s_pin_config_t pin_config = {
    .bck_io_num = I2S_SCK,
    .ws_io_num = I2S_WS,
    .data_out_num = I2S_PIN_NO_CHANGE,
    .data_in_num = I2S_SD
  };

  esp_err_t err;
  
  err = i2s_driver_install(I2S_PORT, &i2s_config, 0, NULL);
  if (err != ESP_OK) {
    Serial.printf("i2s_driver_install error: %d\n", err);
  }

  err = i2s_set_pin(I2S_PORT, &pin_config);
  if (err != ESP_OK) {
    Serial.printf("i2s_set_pin error: %d\n", err);
  }

  err = i2s_set_clk(I2S_PORT, SAMPLE_RATE, SAMPLE_BITS_MIC, I2S_CHANNEL_MONO);
  if (err != ESP_OK) {
    Serial.printf("i2s_set_clk error: %d\n", err);
  }
}

// --------------------------------------------------------
// Cấu hình SD
// --------------------------------------------------------
bool setupSD() {
  SPI.begin(SD_SCK, SD_MISO, SD_MOSI, SD_CS);
  if (!SD.begin(SD_CS)) {
    Serial.println("❌ SD init FAILED!");
    return false;
  }
  Serial.println("✔ SD init OK.");
  return true;
}

// --------------------------------------------------------
// Ghi 10 giây audio vào /record.wav
// --------------------------------------------------------
void recordToSD() {
  File wavFile = SD.open("/record.wav", FILE_WRITE);
  if (!wavFile) {
    Serial.println("❌ Không mở được file /record.wav để ghi");
    return;
  }

  // Ghi header WAV (đã biết trước kích thước data)
  writeWavHeader(wavFile,
                 SAMPLE_RATE,
                 SAMPLE_BITS_WAV,
                 CHANNELS,
                 DATA_BYTES);

  Serial.println("=== BẮT ĐẦU GHI ÂM 10 GIÂY ===");
  delay(200);

  const size_t i2sReadSamples = 512;  // size mỗi block
  int32_t i2sBuffer[i2sReadSamples];
  int16_t wavBuffer[i2sReadSamples];
  uint32_t samplesWritten = 0;

  while (samplesWritten < NUM_SAMPLES) {
    size_t bytesRead = 0;
    esp_err_t res = i2s_read(
      I2S_PORT,
      (void*)i2sBuffer,
      i2sReadSamples * sizeof(int32_t),
      &bytesRead,
      portMAX_DELAY
    );

    if (res != ESP_OK || bytesRead == 0) {
      Serial.println("❌ Lỗi đọc I2S");
      break;
    }

    size_t samplesRead = bytesRead / sizeof(int32_t);

    // Convert 32-bit (INMP441) -> 16-bit PCM
    for (size_t i = 0; i < samplesRead; i++) {
      int32_t s32 = i2sBuffer[i];
      // INMP441: thường là 24-bit trong 32-bit word
      int16_t s16 = (int16_t)(s32 >> 11); // giảm biên độ cho đỡ clip
      wavBuffer[i] = s16;
    }

    // Giới hạn nếu sắp đủ NUM_SAMPLES
    uint32_t samplesLeft = NUM_SAMPLES - samplesWritten;
    if (samplesRead > samplesLeft) {
      samplesRead = samplesLeft;
    }

    // Ghi ra file
    wavFile.write((uint8_t*)wavBuffer, samplesRead * sizeof(int16_t));
    samplesWritten += samplesRead;
  }

  wavFile.flush();
  wavFile.close();

  Serial.println("=== NGỪNG GHI ÂM ===");
  Serial.println("Đã lưu file /record.wav trên thẻ SD.");
}

// --------------------------------------------------------
// SETUP & LOOP
// --------------------------------------------------------
void setup() {
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("\nESP32-S3 + INMP441 + SD Card Recorder");
  
  setupI2S();
  
  if (!setupSD()) {
    Serial.println("Dừng do lỗi SD.");
    while (1) delay(1000);
  }
  
  Serial.println("Chuẩn bị ghi...");
  delay(1000);
  
  recordToSD();
  
  Serial.println("Hoàn thành. Có thể rút thẻ, cắm PC nghe file record.wav.");
}

void loop() {
  // Không làm gì thêm
}

