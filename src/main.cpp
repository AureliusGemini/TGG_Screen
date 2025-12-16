#include <Arduino.h>
#include <Arduino_GFX_Library.h>
#include <XPT2046_Touchscreen.h>
#include <SPI.h>
#include <lvgl.h>
#include <HardwareSerial.h>
#include <WiFi.h>
#include <esp_now.h>
#include <WiFiManager.h>
#include "ui/ui.h" // Links to your src/ui folder

// ==========================================
// 1. PIN DEFINITIONS (Sunton ESP32-4827S043R)
// ==========================================
#define TFT_BL 2
Arduino_ESP32RGBPanel *bus = new Arduino_ESP32RGBPanel(
    40 /* DE */, 41 /* VSYNC */, 39 /* HSYNC */, 42 /* PCLK */,
    45 /* R0 */, 48 /* R1 */, 47 /* R2 */, 21 /* R3 */, 14 /* R4 */,
    5 /* G0 */, 6 /* G1 */, 7 /* G2 */, 15 /* G3 */, 16 /* G4 */, 4 /* G5 */,
    8 /* B0 */, 3 /* B1 */, 46 /* B2 */, 9 /* B3 */, 1 /* B4 */,
    0, 8, 4, 43, 0, 8, 4, 12, 1, 16000000);
Arduino_RGB_Display *gfx = new Arduino_RGB_Display(480, 272, bus, 0, true);

#define XPT_CS 38
#define XPT_IRQ 18
#define XPT_MOSI 11
#define XPT_MISO 13
#define XPT_CLK 12
SPIClass mySpi = SPIClass(SPI);
XPT2046_Touchscreen ts(XPT_CS, XPT_IRQ);

// ==========================================
// 2. ESP-NOW DATA STRUCTURE
// ==========================================
typedef struct
{
  float temperature;   // Baris 1, Kolom 1
  float humidity;      // Baris 1, Kolom 2
  float pressure;      // Baris 1, Kolom 3
  float lux;           // Baris 2, Kolom 1
  float fertilizer;    // Baris 2, Kolom 2
  float rainIntensity; // Baris 2, Kolom 3
} SensorData;

SensorData receivedData = {0, 0, 0, 0, 0, 0};
unsigned long lastDataTime = 0;
const unsigned long DATA_TIMEOUT = 5000; // 5 detik timeout

// ==========================================
// 3. UART SETUP (To Waveshare Hub)
// ==========================================
HardwareSerial &UartHub = Serial2;
#define HUB_RX_PIN 18
#define HUB_TX_PIN 17

// ==========================================
// 3. LVGL DRIVERS
// ==========================================
static lv_disp_draw_buf_t draw_buf;
static lv_color_t *disp_draw_buf;

void my_disp_flush(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p)
{
  uint32_t w = (area->x2 - area->x1 + 1);
  uint32_t h = (area->y2 - area->y1 + 1);
  gfx->draw16bitRGBBitmap(area->x1, area->y1, (uint16_t *)&color_p->full, w, h);
  lv_disp_flush_ready(disp);
}

void my_touch_read(lv_indev_drv_t *drv, lv_indev_data_t *data)
{
  if (ts.touched())
  {
    TS_Point p = ts.getPoint();
    // CALIBRATION: Map raw hardware (200-3800) to screen pixels (0-480)
    data->point.x = map(p.x, 200, 3800, 0, 480);
    data->point.y = map(p.y, 200, 3700, 0, 272);
    data->state = LV_INDEV_STATE_PR;
  }
  else
  {
    data->state = LV_INDEV_STATE_REL;
  }
}

// ==========================================
// 4. BUTTON LOGIC (Defined but unused for now)
// ==========================================

// ESP-NOW Receive Callback
void onDataReceived(const uint8_t *mac_addr, const uint8_t *incomingData, int len)
{
  if (len == sizeof(SensorData))
  {
    memcpy(&receivedData, incomingData, sizeof(SensorData));
    lastDataTime = millis();
    Serial.println("ESP-NOW Data Received:");
    Serial.printf("  Temp: %.2f°C, Humid: %.2f%%, Pressure: %.2f hPa\n",
                  receivedData.temperature, receivedData.humidity, receivedData.pressure);
    Serial.printf("  Lux: %.2f, Fertilizer: %.2f, Rain: %.2f%%\n",
                  receivedData.lux, receivedData.fertilizer, receivedData.rainIntensity);
  }
}

static void pump_button_event_handler(lv_event_t *e)
{
  lv_event_code_t code = lv_event_get_code(e);
  if (code == LV_EVENT_CLICKED)
  {
    static bool isPumpOn = false;
    isPumpOn = !isPumpOn;
    if (isPumpOn)
    {
      UartHub.println("PUMP:ON");
      Serial.println("Action: Sent PUMP:ON");
    }
    else
    {
      UartHub.println("PUMP:OFF");
      Serial.println("Action: Sent PUMP:OFF");
    }
  }
}

// ==========================================
// 5. UI UPDATE FUNCTION
// ==========================================
void updateSensorDisplay()
{
  // Baris 1: Temp, Humidity, Pressure
  if (ui_Label1 != NULL)
  {
    char buff1[32];
    sprintf(buff1, "%.1f°C", receivedData.temperature);
    lv_label_set_text(ui_Label1, buff1);
  }

  if (ui_Label2 != NULL)
  {
    char buff2[32];
    sprintf(buff2, "%.1f%%", receivedData.humidity);
    lv_label_set_text(ui_Label2, buff2);
  }

  if (ui_Label3 != NULL)
  {
    char buff3[32];
    sprintf(buff3, "%.1f hPa", receivedData.pressure);
    lv_label_set_text(ui_Label3, buff3);
  }

  // Baris 2: Lux, Fertilizer, Rain Intensity
  // NOTE: Jika UI tidak memiliki Label4-6, buat tambahan atau gunakan Panel untuk text
  // Untuk sekarang, gunakan Serial untuk debugging
  Serial.printf("Row2 - Lux: %.2f, Fert: %.2f, Rain: %.2f%%\n",
                receivedData.lux, receivedData.fertilizer, receivedData.rainIntensity);
}

// ==========================================
// 6. SETUP
// ==========================================
void setup()
{
  Serial.begin(115200);                                      // USB Debug
  UartHub.begin(115200, SERIAL_8N1, HUB_RX_PIN, HUB_TX_PIN); // UART to Hub

  // ==========================================
  // INITIALIZE DISPLAY & TOUCH
  // ==========================================
  gfx->begin();
  gfx->fillScreen(BLACK);
  pinMode(TFT_BL, OUTPUT);
  digitalWrite(TFT_BL, HIGH);
  mySpi.begin(XPT_CLK, XPT_MISO, XPT_MOSI, XPT_CS);
  ts.begin(mySpi);
  ts.setRotation(1);

  // Display loading message
  gfx->setTextColor(WHITE);
  gfx->setTextSize(2);
  gfx->setCursor(50, 130);
  gfx->println("Initializing WiFi...");

  // ==========================================
  // INITIALIZE WIFI MANAGER
  // ==========================================
  WiFiManager wifiManager;

  // Uncomment to reset WiFi settings for testing
  // wifiManager.resetSettings();

  // Set AP name and password
  wifiManager.autoConnect("ESP32-TGG-Screen");

  Serial.println("\n\nWiFi connected!");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  // Update display
  gfx->fillScreen(BLACK);
  gfx->setCursor(50, 130);
  gfx->println("WiFi Connected!");
  delay(1000);

  // ==========================================
  // INITIALIZE ESP-NOW
  // ==========================================
  if (esp_now_init() != ESP_OK)
  {
    Serial.println("Error initializing ESP-NOW");
    gfx->fillScreen(BLACK);
    gfx->setCursor(50, 130);
    gfx->println("ESP-NOW Init Failed!");
    while (1)
      delay(1000);
  }

  // Register receive callback
  esp_now_register_recv_cb(onDataReceived);
  Serial.println("ESP-NOW initialized and listening for data...");

  // ==========================================
  // INITIALIZE LVGL
  // ==========================================
  lv_init();
  disp_draw_buf = (lv_color_t *)heap_caps_malloc(sizeof(lv_color_t) * 480 * 30, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
  lv_disp_draw_buf_init(&draw_buf, disp_draw_buf, NULL, 480 * 30);

  static lv_disp_drv_t disp_drv;
  lv_disp_drv_init(&disp_drv);
  disp_drv.hor_res = 480;
  disp_drv.ver_res = 272;
  disp_drv.flush_cb = my_disp_flush;
  disp_drv.draw_buf = &draw_buf;
  lv_disp_drv_register(&disp_drv);

  static lv_indev_drv_t indev_drv;
  lv_indev_drv_init(&indev_drv);
  indev_drv.type = LV_INDEV_TYPE_POINTER;
  indev_drv.read_cb = my_touch_read;
  lv_indev_drv_register(&indev_drv);

  // LOAD YOUR UI
  ui_init();

  // ATTACH BUTTON EVENT
  // TODO: Check src/ui/ui.h for the real button name (e.g. ui_btnPump) and uncomment this block
  /*
  if (ui_Button1 != NULL) {
      lv_obj_add_event_cb(ui_Button1, pump_button_event_handler, LV_EVENT_ALL, NULL);
  }
  */
}

// ==========================================
// 7. LOOP
// ==========================================
void loop()
{
  lv_timer_handler(); // Refresh UI

  // Update sensor display from ESP-NOW data
  unsigned long currentTime = millis();
  if (currentTime - lastDataTime < DATA_TIMEOUT)
  {
    // Data still valid, update display
    updateSensorDisplay();
  }
  else if (lastDataTime > 0)
  {
    // Data timeout - show "No Signal"
    if (ui_Label1 != NULL)
      lv_label_set_text(ui_Label1, "---");
    if (ui_Label2 != NULL)
      lv_label_set_text(ui_Label2, "---");
    if (ui_Label3 != NULL)
      lv_label_set_text(ui_Label3, "---");
  }

  // Check for incoming data from Hub (optional UART fallback)
  if (UartHub.available())
  {
    String data = UartHub.readStringUntil('\n');
    data.trim();
    if (data.length() > 0)
      Serial.println("RX UART: " + data);

    // -- PARSE TEMP --
    int idxT = data.indexOf("T=");
    if (idxT != -1)
    {
      int end = data.indexOf(";", idxT);
      if (end == -1)
        end = data.length();
      String val = data.substring(idxT + 2, end);

      // TODO: Check src/ui/ui.h for real Temp Label name and uncomment
      // if (ui_Label1 != NULL) lv_label_set_text(ui_Label1, (val + " C").c_str());
    }

    // -- PARSE HUMIDITY --
    int idxH = data.indexOf("H=");
    if (idxH != -1)
    {
      int end = data.indexOf(";", idxH);
      if (end == -1)
        end = data.length();
      String val = data.substring(idxH + 2, end);

      // TODO: Check src/ui/ui.h for real Humid Label name and uncomment
      // if (ui_Label2 != NULL) lv_label_set_text(ui_Label2, (val + " %").c_str());
    }
  }
  delay(5);
}