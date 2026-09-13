#pragma once
#define LGFX_USE_V1
#include <LovyanGFX.hpp>

// Configuración de Hardware de la pantalla circular GC9A01 en ESP32
class LGFX_GC9A01 : public lgfx::LGFX_Device {
    lgfx::Panel_GC9A01 _panel_instance;
    lgfx::Bus_SPI      _bus_instance;

public:
    LGFX_GC9A01(void) {
        {
            auto cfg = _bus_instance.config();
            cfg.spi_host = VSPI_HOST;      // Bus SPI del ESP32
            cfg.spi_mode = 0;              // Modo SPI 0
            cfg.freq_write = 27000000;     // 27 MHz (óptimo y estable para protoboard)
            cfg.freq_read  = 16000000;
            cfg.pin_sclk = 18;             // SCK / SCL
            cfg.pin_mosi = 23;             // MOSI / SDA
            cfg.pin_miso = -1;             // No se usa MISO
            cfg.pin_dc   = 2;              // DC
            cfg.dma_channel = SPI_DMA_CH_AUTO; // DMA automático para zero-flicker
            _bus_instance.config(cfg);
            _panel_instance.setBus(&_bus_instance);
        }

        {
            auto cfg = _panel_instance.config();
            cfg.pin_cs           = 5;      // CS
            cfg.pin_rst          = 4;      // RST
            cfg.pin_busy         = -1;
            cfg.panel_width      = 240;    // Resolución nativa GC9A01
            cfg.panel_height     = 240;
            cfg.offset_x         = 0;
            cfg.offset_y         = 0;
            cfg.readable         = false;
            cfg.invert           = true;   // Los paneles GC9A01 suelen requerir invertido
            cfg.rgb_order        = false;
            cfg.dlen_16bit       = false;
            cfg.bus_shared       = false;
            _panel_instance.config(cfg);
        }

        setPanel(&_panel_instance);
    }
};
