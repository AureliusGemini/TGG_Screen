/**
 * @file sensor_data.h
 * @brief Data structure dan fungsi untuk ESP-NOW communication
 *
 * ESP-NOW Receiver (ESP32 LCD - ini)
 * - Menerima data dari 1 sender (ESP1)
 * - Menampilkan di LCD 2 baris x 3 kolom
 * - Format: Baris 1 (Suhu, Kelembaban, Tekanan)
 *          Baris 2 (Lux, Pupuk, Intensitas Hujan)
 *
 * ESP-NOW Sender (ESP1)
 * - Kirim struktur SensorData setiap interval waktu
 * - Gunakan esp_now_send() untuk transmit
 * - MAC address receiver harus ditambahkan dengan esp_now_add_peer()
 */

#ifndef SENSOR_DATA_H
#define SENSOR_DATA_H

#include <cstring>

// ==========================================
// SENSOR DATA STRUCTURE (DO NOT CHANGE)
// ==========================================
typedef struct
{
    float temperature;   // Baris 1, Kolom 1 (°C)
    float humidity;      // Baris 1, Kolom 2 (%)
    float pressure;      // Baris 1, Kolom 3 (hPa)
    float lux;           // Baris 2, Kolom 1 (lux)
    float fertilizer;    // Baris 2, Kolom 2 (value)
    float rainIntensity; // Baris 2, Kolom 3 (%)
} SensorData;

// ==========================================
// MAC ADDRESS CONFIG
// ==========================================
// Ganti dengan MAC address ESP32 LCD (receiver)
// Dapatkan dengan menjalankan ini di ESP:
// Serial.println(WiFi.macAddress());
#define LCD_RECEIVER_MAC {0xXX, 0xXX, 0xXX, 0xXX, 0xXX, 0xXX}

// ==========================================
// CONTOH PENGGUNAAN DI ESP1 (SENDER)
// ==========================================
/*
#include "sensor_data.h"
#include <esp_now.h>
#include <WiFi.h>

uint8_t broadcastAddress[] = LCD_RECEIVER_MAC;

void setup() {
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);

  if (esp_now_init() != ESP_OK) {
    Serial.println("Error init ESP-NOW");
    return;
  }

  esp_now_peer_info_t peerInfo = {};
  memcpy(peerInfo.peer_addr, broadcastAddress, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;

  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("Failed to add peer");
    return;
  }
}

void loop() {
  // Baca sensor
  SensorData data;
  data.temperature = readTemperature();   // Ganti dengan fungsi sensor Anda
  data.humidity = readHumidity();
  data.pressure = readPressure();
  data.lux = readLux();
  data.fertilizer = readFertilizer();
  data.rainIntensity = readRain();

  // Kirim via ESP-NOW
  esp_now_send(broadcastAddress, (uint8_t *)&data, sizeof(data));

  delay(1000); // Kirim setiap 1 detik
}
*/

#endif // SENSOR_DATA_H
