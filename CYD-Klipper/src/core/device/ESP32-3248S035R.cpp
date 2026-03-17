#ifdef CYD_SCREEN_DRIVER_ESP32_3248S035R
#include "../screen_driver.h"

#include "lvgl.h"
#include <SPI.h>
#include <TFT_eSPI.h>
#include <XPT2046_Touchscreen.h>
#include "../../conf/global_config.h"
#include "../lv_setup.h"

#define XPT2046_IRQ 36
#define XPT2046_MOSI 32
#define XPT2046_MISO 39
#define XPT2046_CLK 25
#define XPT2046_CS 33
#define CPU_FREQ_HIGH 240
#define CPU_FREQ_LOW 80

#define LED_PIN_R 4
#define LED_PIN_G 16
#define LED_PIN_B 17

SPIClass touchscreen_spi = SPIClass(VSPI);
XPT2046_Touchscreen touchscreen(XPT2046_CS, XPT2046_IRQ);

static lv_disp_draw_buf_t draw_buf;
static lv_color_t buf[CYD_SCREEN_HEIGHT_PX * CYD_SCREEN_WIDTH_PX / 10];

TFT_eSPI tft = TFT_eSPI();

void screen_setBrightness(byte brightness)
{
    // calculate duty, 4095 from 2 ^ 12 - 1
    uint32_t duty = (4095 / 255) * brightness;

    // write duty to LEDC
    ledcWrite(0, duty);
}

void screen_lv_flush(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p)
{
    uint32_t w = (area->x2 - area->x1 + 1);
    uint32_t h = (area->y2 - area->y1 + 1);

    tft.startWrite();
    tft.setAddrWindow(area->x1, area->y1, w, h);
    tft.pushColors((uint16_t *)&color_p->full, w * h, true);
    tft.endWrite();

    lv_disp_flush_ready(disp);
}

void screen_lv_touchRead(lv_indev_drv_t *indev_driver, lv_indev_data_t *data)
{
    if (touchscreen.tirqTouched() && touchscreen.touched())
    {
        TS_Point p = touchscreen.getPoint();
        data->state = LV_INDEV_STATE_PR;
        data->point.x = p.x;
        data->point.y = p.y;
    }
    else
    {
        data->state = LV_INDEV_STATE_REL;
    }
}

void set_invert_display()
{
    tft.invertDisplay(global_config.printer_config[global_config.printer_index].invert_colors);
}

void set_LED_color(uint8_t rgbVal[3])
{
    analogWrite(LED_PIN_R, 255 - rgbVal[0]);
    analogWrite(LED_PIN_G, 255 - rgbVal[1]);
    analogWrite(LED_PIN_B, 255 - rgbVal[2]);
}

void LED_init()
{
    pinMode(LED_PIN_R, OUTPUT);
    pinMode(LED_PIN_G, OUTPUT);
    pinMode(LED_PIN_B, OUTPUT);
    uint8_t rgbVal[3] = {0, 0, 0};
    set_LED_color(rgbVal);
}

void screen_setup()
{
    // Initialize the touchscreen
    touchscreen_spi.begin(XPT2046_CLK, XPT2046_MISO, XPT2046_MOSI, XPT2046_CS);
    touchscreen.begin(touchscreen_spi);
    
    // Initialize LVGL
    lv_init();
    // Initialize the display
    tft.init();
    ledcSetup(0, 5000, 12);
    ledcAttachPin(TFT_BL, 0);
    tft.fillScreen(TFT_BLACK);
    set_invert_display();
    LED_init();

    lv_disp_draw_buf_init(&draw_buf, buf, NULL, TFT_WIDTH * TFT_HEIGHT / 10);
    static lv_disp_drv_t disp_drv;
    lv_disp_drv_init(&disp_drv);


    #ifdef CYD_SCREEN_VERTICAL
        disp_drv.hor_res = TFT_WIDTH;
        disp_drv.ver_res = TFT_HEIGHT;
        
        tft.setRotation(global_config.rotate_screen ? 2 : 0);
        touchscreen.setRotation(global_config.rotate_screen ? 0 : 2);
    #else
        disp_drv.hor_res = TFT_HEIGHT;
        disp_drv.ver_res = TFT_WIDTH;
        
        tft.setRotation(global_config.rotate_screen ? 3 : 1);
        touchscreen.setRotation(global_config.rotate_screen ? 3 : 1);
    #endif


    disp_drv.flush_cb = screen_lv_flush;
    disp_drv.draw_buf = &draw_buf;
    lv_disp_drv_register(&disp_drv);

    // Initialize the (dummy) input device driver
    static lv_indev_drv_t indev_drv;
    lv_indev_drv_init(&indev_drv);
    indev_drv.type = LV_INDEV_TYPE_POINTER;
    indev_drv.read_cb = screen_lv_touchRead;
    lv_indev_drv_register(&indev_drv);
}

#endif // CYD_SCREEN_DRIVER_ESP32_3248S035R