/**
 * TEMPLATE ESP1 (SENDER) - Mengirim data sensor ke ESP32 LCD via ESP-NOW
 *
 * Langkah setup:
 * 1. Ganti LCD_RECEIVER_MAC dengan MAC address LCD receiver Anda
 * 2. Replace fungsi readTemperature(), readHumidity(), dll dengan sensor Anda
 * 3. Adjust interval pengiriman data (delay(1000) = 1 detik)
 * 4. Upload ke ESP1
 */

#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>

// ==========================================
// DATA STRUCTURE (HARUS SAMA dengan LCD)
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
// MAC ADDRESS KONFIGURASI
// ==========================================
// UBAH INI dengan MAC address ESP32 LCD Anda
uint8_t broadcastAddress[] = {0xXX, 0xXX, 0xXX, 0xXX, 0xXX, 0xXX};

// ==========================================
// VARIABEL GLOBAL
// ==========================================
SensorData sensorData;
unsigned long lastSendTime = 0;
const unsigned long SEND_INTERVAL = 1000; // 1000ms = 1 detik

// ==========================================
// CALLBACK - Optional (uncomment jika perlu ACK)
// ==========================================
void onDataSent(const uint8_t *mac_addr, esp_now_send_status_t status)
{
    if (status == ESP_NOW_SEND_SUCCESS)
    {
        Serial.println("Data sent successfully!");
    }
    else
    {
        Serial.println("Data send failed!");
    }
}

// ==========================================
// SENSOR READING FUNCTIONS - CUSTOMIZE INI
// ==========================================

/**
 * Baca suhu dari sensor Anda
 * @return float temperature dalam °C
 */
float readTemperature()
{
    // TODO: Replace dengan kode sensor real Anda
    // Contoh: return bmp280.readTemperature();
    // Untuk testing: return random value
    return 20.0 + (random(0, 100) / 10.0); // 20-30°C
}

/**
 * Baca kelembaban dari sensor Anda
 * @return float humidity dalam %
 */
float readHumidity()
{
    // TODO: Replace dengan kode sensor real Anda
    // Contoh: return dht.readHumidity();
    return 40.0 + (random(0, 500) / 10.0); // 40-90%
}

/**
 * Baca tekanan dari sensor Anda
 * @return float pressure dalam hPa
 */
float readPressure()
{
    // TODO: Replace dengan kode sensor real Anda
    // Contoh: return bmp280.readPressure() / 100.0;
    return 1000.0 + (random(0, 200) / 10.0); // 1000-1020 hPa
}

/**
 * Baca light intensity (lux) dari sensor Anda
 * @return float lux value
 */
float readLux()
{
    // TODO: Replace dengan kode sensor real Anda
    // Contoh: return bh1750.readLightLevel();
    return 100.0 + (random(0, 9900)); // 100-10000 lux
}

/**
 * Baca nutrisi/pupuk dari sensor Anda
 * @return float fertilizer value
 */
float readFertilizer()
{
    // TODO: Replace dengan kode sensor real Anda
    // Contoh: return EC_sensor.readValue();
    return 1000.0 + (random(0, 2000)); // 1000-3000 uS/cm
}

/**
 * Baca intensitas hujan dari sensor Anda
 * @return float rain intensity dalam %
 */
float readRainIntensity()
{
    // TODO: Replace dengan kode sensor real Anda
    // Contoh: return rain_sensor.getIntensity();
    return random(0, 100); // 0-100%
}

// ==========================================
// SETUP
// ==========================================
void setup()
{
    Serial.begin(115200);
    delay(1000);

    Serial.println("\n\n=== ESP1 ESP-NOW SENDER ===");
    Serial.println("Setting up ESP-NOW...");

    // Set WiFi mode
    WiFi.mode(WIFI_STA);

    // Get local MAC address
    Serial.print("Local MAC: ");
    Serial.println(WiFi.macAddress());

    Serial.print("Target MAC: ");
    for (int i = 0; i < 6; i++)
    {
        Serial.printf("%02X", broadcastAddress[i]);
        if (i < 5)
            Serial.print(":");
    }
    Serial.println();

    // Init ESP-NOW
    if (esp_now_init() != ESP_OK)
    {
        Serial.println("Error initializing ESP-NOW");
        while (1)
            delay(1000);
    }

    // Register send callback
    esp_now_register_send_cb(onDataSent);

    // Add broadcast peer
    esp_now_peer_info_t peerInfo = {};
    memcpy(peerInfo.peer_addr, broadcastAddress, 6);
    peerInfo.channel = 0;
    peerInfo.encrypt = false;

    if (esp_now_add_peer(&peerInfo) != ESP_OK)
    {
        Serial.println("Failed to add peer");
        while (1)
            delay(1000);
    }

    Serial.println("ESP-NOW initialized. Ready to send data!");
}

// ==========================================
// LOOP
// ==========================================
void loop()
{
    unsigned long currentTime = millis();

    // Send data setiap SEND_INTERVAL milliseconds
    if (currentTime - lastSendTime >= SEND_INTERVAL)
    {
        lastSendTime = currentTime;

        // Read sensor values
        sensorData.temperature = readTemperature();
        sensorData.humidity = readHumidity();
        sensorData.pressure = readPressure();
        sensorData.lux = readLux();
        sensorData.fertilizer = readFertilizer();
        sensorData.rainIntensity = readRainIntensity();

        // Print untuk debug
        Serial.println("\n--- Sending Sensor Data ---");
        Serial.printf("Temp: %.2f°C, Humid: %.2f%%, Pressure: %.2f hPa\n",
                      sensorData.temperature, sensorData.humidity, sensorData.pressure);
        Serial.printf("Lux: %.2f, Fert: %.2f, Rain: %.2f%%\n",
                      sensorData.lux, sensorData.fertilizer, sensorData.rainIntensity);

        // Send via ESP-NOW
        esp_now_send(broadcastAddress, (uint8_t *)&sensorData, sizeof(sensorData));
    }

    delay(10); // Small delay untuk prevent watchdog timeout
}
