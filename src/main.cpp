#include <Arduino.h>
#include <Arduino_GFX_Library.h>
#include <XPT2046_Touchscreen.h>
#include <SPI.h>
#include <WiFiManager.h>
#include <lvgl.h> // Include LVGL

// --- HARDWARE PIN DEFINITIONS ---
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
SPIClass mySpi = SPIClass(SPI);
XPT2046_Touchscreen ts(XPT_CS, XPT_IRQ);
WiFiManager wm;

// --- LVGL BUFFERS ---
static lv_disp_draw_buf_t draw_buf;
static lv_color_t *disp_draw_buf;

// --- UI ELEMENTS ---
lv_obj_t *label_temp;
lv_obj_t *label_humid;
lv_obj_t *btn_pump;
lv_obj_t *label_btn;
bool pump_status = false;

/* * 1. FLUSH CALLBACK:
 * This function tells LVGL how to paint pixels onto your specific screen.
 */
void my_disp_flush(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p)
{
  uint32_t w = (area->x2 - area->x1 + 1);
  uint32_t h = (area->y2 - area->y1 + 1);

  gfx->draw16bitRGBBitmap(area->x1, area->y1, (uint16_t *)&color_p->full, w, h);
  lv_disp_flush_ready(disp); // Tell LVGL we are done
}

/* * 2. INPUT CALLBACK:
 * This function tells LVGL where you are touching.
 */
void my_touch_read(lv_indev_drv_t *drv, lv_indev_data_t *data)
{
  if (ts.touched())
  {
    TS_Point p = ts.getPoint();
    // Calibrate these numbers using Serial Monitor!
    // X: 200->3800, Y: 200->3700 mapped to 480x272
    data->point.x = map(p.x, 200, 3800, 0, 480);
    data->point.y = map(p.y, 200, 3700, 0, 272);
    data->state = LV_INDEV_STATE_PR;
  }
  else
  {
    data->state = LV_INDEV_STATE_REL;
  }
}

// --- BUTTON EVENT HANDLER ---
static void btn_event_handler(lv_event_t *e)
{
  lv_event_code_t code = lv_event_get_code(e);
  if (code == LV_EVENT_CLICKED)
  {
    pump_status = !pump_status;
    if (pump_status)
    {
      lv_label_set_text(label_btn, "PUMP: ON");
      lv_obj_set_style_bg_color(btn_pump, lv_palette_main(LV_PALETTE_GREEN), 0);
      // TODO: Send MQTT "ON" here
    }
    else
    {
      lv_label_set_text(label_btn, "PUMP: OFF");
      lv_obj_set_style_bg_color(btn_pump, lv_palette_main(LV_PALETTE_RED), 0);
      // TODO: Send MQTT "OFF" here
    }
  }
}

// --- BUILD THE UI ---
void setupLVGLUI()
{
  // 1. Header Style
  static lv_style_t style_header;
  lv_style_init(&style_header);
  lv_style_set_bg_color(&style_header, lv_palette_main(LV_PALETTE_GREEN));
  lv_style_set_bg_opa(&style_header, LV_OPA_COVER);
  lv_style_set_text_color(&style_header, lv_color_white());

  // 2. Header Object
  lv_obj_t *header = lv_obj_create(lv_scr_act());
  lv_obj_set_size(header, 480, 50);
  lv_obj_align(header, LV_ALIGN_TOP_MID, 0, 0);
  lv_obj_add_style(header, &style_header, 0);

  lv_obj_t *title = lv_label_create(header);
  lv_label_set_text(title, "GREEN GUARDIAN");
  lv_obj_align(title, LV_ALIGN_CENTER, 0, 0);

  // 3. Sensor Panels
  // Temp
  lv_obj_t *panel1 = lv_obj_create(lv_scr_act());
  lv_obj_set_size(panel1, 140, 100);
  lv_obj_align(panel1, LV_ALIGN_LEFT_MID, 20, 0);

  lv_obj_t *label1 = lv_label_create(panel1);
  lv_label_set_text(label1, "TEMP");
  lv_obj_align(label1, LV_ALIGN_TOP_MID, 0, 0);

  label_temp = lv_label_create(panel1); // Global var to update later
  lv_label_set_text(label_temp, "24.5 C");
  lv_obj_set_style_text_font(label_temp, &lv_font_montserrat_28, 0);
  lv_obj_align(label_temp, LV_ALIGN_CENTER, 0, 10);

  // Pump Button
  btn_pump = lv_btn_create(lv_scr_act());
  lv_obj_set_size(btn_pump, 140, 60);
  lv_obj_align(btn_pump, LV_ALIGN_BOTTOM_RIGHT, -20, -20);
  lv_obj_add_event_cb(btn_pump, btn_event_handler, LV_EVENT_ALL, NULL);
  lv_obj_set_style_bg_color(btn_pump, lv_palette_main(LV_PALETTE_RED), 0);

  label_btn = lv_label_create(btn_pump);
  lv_label_set_text(label_btn, "PUMP: OFF");
  lv_obj_center(label_btn);
}

void setup()
{
  Serial.begin(115200);

  // 1. Init Hardware
  gfx->begin();
  gfx->fillScreen(BLACK);
  pinMode(TFT_BL, OUTPUT);
  digitalWrite(TFT_BL, HIGH);

  mySpi.begin(12, 13, 11, 38); // CLK, MISO, MOSI, CS
  ts.begin(mySpi);
  ts.setRotation(1);

  // 2. Init LVGL
  lv_init();

  // 3. Allocate Buffer (1/10th of screen size)
  // 480 * 272 * 2 bytes (16bit color) / 10 = ~26KB
  disp_draw_buf = (lv_color_t *)heap_caps_malloc(sizeof(lv_color_t) * 480 * 30, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
  lv_disp_draw_buf_init(&draw_buf, disp_draw_buf, NULL, 480 * 30);

  // 4. Register Display Driver
  static lv_disp_drv_t disp_drv;
  lv_disp_drv_init(&disp_drv);
  disp_drv.hor_res = 480;
  disp_drv.ver_res = 272;
  disp_drv.flush_cb = my_disp_flush;
  disp_drv.draw_buf = &draw_buf;
  lv_disp_drv_register(&disp_drv);

  // 5. Register Touch Driver
  static lv_indev_drv_t indev_drv;
  lv_indev_drv_init(&indev_drv);
  indev_drv.type = LV_INDEV_TYPE_POINTER;
  indev_drv.read_cb = my_touch_read;
  lv_indev_drv_register(&indev_drv);

  // 6. Start WiFiManager (Blocking)
  // Note: In a real app, you might want to run this non-blocking
  // or draw a "Connecting" LVGL screen first.
  if (!wm.autoConnect("GreenGuardian_Setup"))
  {
    Serial.println("WiFi Failed");
  }
  else
  {
    Serial.println("WiFi Connected");
  }

  // 7. Create UI
  setupLVGLUI();
}

void loop()
{
  // LVGL Manager
  lv_timer_handler();

  // Add MQTT check logic here
  // client.loop();

  delay(5);
}