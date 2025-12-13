#include <Arduino.h>
#include <Arduino_GFX_Library.h>
#include <XPT2046_Touchscreen.h>
#include <SPI.h>
#include <lvgl.h>
#include <HardwareSerial.h>
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
// 2. UART SETUP (To Waveshare Hub)
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
// 5. SETUP
// ==========================================
void setup()
{
  Serial.begin(115200);                                      // USB Debug
  UartHub.begin(115200, SERIAL_8N1, HUB_RX_PIN, HUB_TX_PIN); // UART to Hub

  // Init Display & Touch
  gfx->begin();
  gfx->fillScreen(BLACK);
  pinMode(TFT_BL, OUTPUT);
  digitalWrite(TFT_BL, HIGH);
  mySpi.begin(XPT_CLK, XPT_MISO, XPT_MOSI, XPT_CS);
  ts.begin(mySpi);
  ts.setRotation(1);

  // Init LVGL
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
// 6. LOOP
// ==========================================
void loop()
{
  lv_timer_handler(); // Refresh UI

  // Check for incoming data from Hub
  if (UartHub.available())
  {
    String data = UartHub.readStringUntil('\n');
    data.trim();
    if (data.length() > 0)
      Serial.println("RX: " + data);

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