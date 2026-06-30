#ifndef LGFX_CONFIG_H
#define LGFX_CONFIG_H

#include <LovyanGFX.hpp>

#define LGFX_USE_V1

class LGFX : public lgfx::LGFX_Device
{
    lgfx::Panel_ST7796  _panel_instance;
    lgfx::Bus_Parallel8 _bus_instance;
    lgfx::Light_PWM     _light_instance;
    lgfx::Touch_FT5x06  _touch_instance;

public:
    LGFX(void)
    {
        // Parallel8 шина
        {
            auto cfg = _bus_instance.config();
            cfg.freq_write = 40000000;
            cfg.pin_wr = 47; cfg.pin_rd = -1; cfg.pin_rs = 0;
            cfg.pin_d0 = 9;  cfg.pin_d1 = 46; cfg.pin_d2 = 3; cfg.pin_d3 = 8;
            cfg.pin_d4 = 18; cfg.pin_d5 = 17; cfg.pin_d6 = 16; cfg.pin_d7 = 15;
            _bus_instance.config(cfg);
            _panel_instance.setBus(&_bus_instance);
        }

        // Панель
        {
            auto cfg = _panel_instance.config();
            cfg.pin_cs = -1; cfg.pin_rst = 4;
            cfg.memory_width  = 320; cfg.memory_height = 480;
            cfg.panel_width   = 320; cfg.panel_height  = 480;
            cfg.invert = true; cfg.rgb_order = false;
            _panel_instance.config(cfg);
        }

        // Подсветка
        {
            auto cfg = _light_instance.config();
            cfg.pin_bl = 45; cfg.invert = false; cfg.freq = 44100; cfg.pwm_channel = 7;
            _light_instance.config(cfg);
            _panel_instance.setLight(&_light_instance);
        }

        // Touch
        {
            auto cfg = _touch_instance.config();
            cfg.i2c_port = 1; cfg.i2c_addr = 0x38;
            cfg.pin_sda = 6; cfg.pin_scl = 5;
            cfg.freq = 400000; cfg.x_max = 320; cfg.y_max = 480;
            _touch_instance.config(cfg);
            _panel_instance.setTouch(&_touch_instance);
        }

        setPanel(&_panel_instance);
    }
};

extern LGFX tft;
extern int screen_brightness;

#endif