# TGG Screen - ESP32 LCD dengan ESP-NOW & WiFi Manager

## 📋 Ringkasan Perubahan

Kode ESP32 LCD ini sudah diupdate dengan fitur:

### ✅ Fitur Baru

1. **WiFi Manager** - Setup WiFi otomatis tanpa hardcode
2. **ESP-NOW Receiver** - Menerima data sensor wireless dari ESP1
3. **Sensor Display** - Layout 2 baris × 3 kolom untuk 6 sensor
4. **Data Validation** - Auto "No Signal" jika data timeout

### 📡 Data yang Diterima

```
LCD 2x3 Grid:
┌─────────────┬──────────────┬─────────────┐
│ Suhu (°C)   │ Kelembaban(%)│ Tekanan(hPa)│  ← Baris 1
├─────────────┼──────────────┼─────────────┤
│ Lux (lux)   │ Pupuk (unit) │ Hujan (%)   │  ← Baris 2
└─────────────┴──────────────┴─────────────┘
```

## 🔧 Setup Instructions

### Step 1: Dapatkan MAC Address LCD

Sebelum setup ESP1, kita perlu tahu MAC address ESP32 LCD ini.

Temporary code untuk testing:

```cpp
void setup() {
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);
  delay(1000);
  Serial.println(WiFi.macAddress());
}
void loop() { delay(1000); }
```

**Output akan seperti**: `E4:65:B8:C3:2A:F0` (format heksadecimal)

### Step 2: WiFi Connection

1. Power up ESP32 LCD
2. Tunggu setup sequence
3. Dari smartphone/laptop, scan WiFi
4. Connect ke hotspot: **`ESP32-TGG-Screen`**
5. Portal browser otomatis muncul (jika tidak, buka <http://192.168.4.1>)
6. Pilih SSID WiFi yang ingin diconnect
7. Masukkan password WiFi
8. Click Save → ESP akan restart dan connect

**After setup:**

- WiFi settings tersimpan di flash
- Next boot akan auto connect ke WiFi yang tersimpan
- Jika WiFi unavailable, portal setup muncul lagi

### Step 3: Update MAC Address untuk ESP1

Edit `TEMPLATE_ESP1_SENDER.cpp`:

```cpp
// Ganti dari:
uint8_t broadcastAddress[] = { 0xXX, 0xXX, 0xXX, 0xXX, 0xXX, 0xXX };

// Menjadi (contoh dengan MAC: E4:65:B8:C3:2A:F0):
uint8_t broadcastAddress[] = { 0xE4, 0x65, 0xB8, 0xC3, 0x2A, 0xF0 };
```

Konversi: Pisahkan dengan `:` dan convert ke hex dengan `0x` prefix

### Step 4: Setup ESP1 Sender

1. Copy template: `TEMPLATE_ESP1_SENDER.cpp` ke project ESP1 Anda
2. Implementasikan fungsi sensor reading
3. Update MAC address
4. Upload ke ESP1

## 📊 Code Structure

### Main Components

#### 1. **SensorData Structure**

```cpp
typedef struct {
  float temperature;    // Baris 1, Kolom 1
  float humidity;       // Baris 1, Kolom 2
  float pressure;       // Baris 1, Kolom 3
  float lux;            // Baris 2, Kolom 1
  float fertilizer;     // Baris 2, Kolom 2
  float rainIntensity;  // Baris 2, Kolom 3
} SensorData;
```

#### 2. **ESP-NOW Callback**

```cpp
void onDataReceived(const uint8_t *mac_addr, const uint8_t *incomingData, int len)
```

- Dipanggil otomatis saat data diterima
- Copy data ke `receivedData` global variable
- Update timestamp untuk timeout detection

#### 3. **WiFi Manager**

```cpp
WiFiManager wifiManager;
wifiManager.autoConnect("ESP32-TGG-Screen");
```

- Portal setup di <http://192.168.4.1> (saat AP mode)
- Simpan settings ke SPIFFS/NVS
- Auto reconnect jika WiFi down

#### 4. **UI Update Function**

```cpp
void updateSensorDisplay()
```

- Update `ui_Label1`, `ui_Label2`, `ui_Label3` dengan data terbaru
- Format dengan unit (°C, %, hPa)
- Handle "No Signal" jika timeout

## 🔍 Monitoring & Debug

### Serial Output Expected

```
WiFi connected!
IP address: 192.168.1.100
ESP-NOW initialized and listening for data...
ESP-NOW Data Received:
  Temp: 25.50°C, Humid: 65.00%, Pressure: 1013.00 hPa
  Lux: 500.00, Fertilizer: 80.00, Rain: 40.00%
```

### Console Commands untuk Debug

Monitor via Serial dengan baud rate **115200**

## 🐛 Troubleshooting

| Problem | Solution |
|---------|----------|
| WiFi portal tidak muncul | Uncomment `wifiManager.resetSettings()` di setup(), reupload |
| ESP-NOW tidak terima data | Pastikan MAC address ESP1 benar, cek ESP1 sedang mengirim |
| LCD menampilkan "---" | Data timeout (ESP1 offline) atau belum ada transmisi pertama |
| WiFi disconnect-reconnect | Normal behavior, WiFi Manager auto reconnect |
| Touch tidak responsive | Calibrate ulang XPT2046 atau cek koneksi touchscreen |

## 📝 Customization

### Mengubah WiFi Hotspot Name

```cpp
wifiManager.autoConnect("CUSTOM_NAME");  // Ganti "ESP32-TGG-Screen"
```

### Mengubah Data Timeout

```cpp
const unsigned long DATA_TIMEOUT = 10000; // Ubah dari 5000ms ke 10000ms
```

### Menambah Label untuk Baris 2

Saat ini Baris 2 hanya print ke Serial (komentar di `updateSensorDisplay()`).
Jika UI Anda punya Label4, Label5, Label6:

```cpp
if (ui_Label4 != NULL) {
  char buff4[32];
  sprintf(buff4, "%.2f lux", receivedData.lux);
  lv_label_set_text(ui_Label4, buff4);
}
```

## 📦 Files Modified/Created

| File | Status | Purpose |
|------|--------|---------|
| `src/main.cpp` | ✏️ Modified | Main receiver logic |
| `include/sensor_data.h` | ✨ New | Shared data structure |
| `TEMPLATE_ESP1_SENDER.cpp` | ✨ New | Template untuk ESP1 |
| `ESP-NOW_SETUP.md` | ✨ New | Setup guide detail |
| `README_ESP32.md` | ✨ New | File ini |

## 🚀 Next Steps

1. **Upload code ke ESP32 LCD** → pastikan compile success
2. **Setup WiFi** → gunakan portal WiFi Manager
3. **Catat MAC address** → dari Serial monitor
4. **Implementasikan ESP1** → gunakan `TEMPLATE_ESP1_SENDER.cpp`
5. **Test komunikasi** → monitor Serial, lihat data di LCD

## 📚 References

- [WiFiManager Library](https://github.com/tzapu/WiFiManager)
- [ESP-NOW Documentation](https://docs.espressif.com/projects/esp-idf/en/latest/api-reference/network/esp_now.html)
- [LVGL Documentation](https://docs.lvgl.io/)
- [Arduino_GFX Library](https://github.com/moononournation/Arduino_GFX)

## 💡 Tips & Best Practices

1. **WiFi & ESP-NOW**: Kedua-duanya berjalan simultaneously (no conflict)
2. **Power**: Pastikan power supply stabil (especially saat WiFi scanning)
3. **Sensor Interval**: Default ESP1 kirim setiap 1 detik (adjustable)
4. **LCD Update**: 60 FPS LVGL refresh, safe untuk update UI dari callback
5. **Error Handling**: Serial monitor show semua error untuk debugging

---

**Created**: December 16, 2025  
**Status**: Ready for ESP1 implementation
