# ATK-DNESP32S3 Development Board

[中文](README_CN.md) | [Component overview](../README.md)

`atk_dnesp32s3` defines the ALIENTEK ATK-DNESP32S3 (ESP32-S3) development board. The base configuration uses an ST7789 SPI display and an ES8388 audio codec. It also provides camera, SD card, XL9555 IO expander, button, LED and sensor support, plus infrared RMT peripherals for on-demand initialization.

## Board Selection

Run these commands in an ESP-IDF application with this component installed:

```bash
idf.py bmgr -b atk_dnesp32s3
idf.py build
```

Board definitions are in [board_devices.yaml](board_devices.yaml), [board_peripherals.yaml](board_peripherals.yaml) and [sdkconfig.defaults.board](sdkconfig.defaults.board). Board defaults enable only the ES8388 codec driver. Explicit settings in an existing `sdkconfig` must be updated separately.

## Select an RGB LCD Module

The base board uses the ST7789 SPI LCD. To use an ALIENTEK 4.3-inch 800x480 RGB module, explicitly select this board-local amend:

| Amend | Panel ID in the vendor example | Resolution | Pixel clock |
| ----- | ------------------------------ | ---------- | ----------- |
| `sub_board_rgb_800_480` | `0x4384` | 800 × 480 | 18 MHz |

```bash
idf.py bmgr -b atk_dnesp32s3 -a sub_board_rgb_800_480
idf.py build

# Select the base SPI LCD again.
idf.py bmgr -b atk_dnesp32s3
```

The amend changes the existing `display_lcd` device from SPI to RGB; only one display device is generated. Application and test-case names stay the same. RGB uses a 16-bit RGB565 bus in DE mode (DE GPIO4, PCLK GPIO5, no HSYNC/VSYNC pins) and `RGB565_LE` frame buffers in octal PSRAM. XL9555 supplies the existing LCD backlight and power outputs. The amend removes `camera`, `audio_dac`, `audio_adc` and their dedicated peripherals because their pins overlap the RGB bus; camera PWDN is asserted. SD and buttons remain available, and `spi_shared` remains available for SD. The inherited SPI configuration is ignored by the RGB driver, which creates no SPI panel IO.

The module enables 80 MHz octal PSRAM. Regenerate the configuration when switching displays, and check the corresponding PSRAM options in an existing `sdkconfig`. Connect only the selected LCD module and disconnect the camera while using RGB. The 800x480 amend also registers `lcd_touch` for the GT1158 controller identified on the tested module. Its device and peripheral configuration is kept together in [rgb_panel.yaml](sub_board_rgb_800_480/rgb_panel.yaml).

Wiring and timing values come from the vendor `23_rgb` / `24_touch` `BSP/RGBLCD/ltdc.h` and `ltdc.c`. For 800x480, horizontal/vertical pulse widths use the declared `hsw=48` / `vsw=3`; the vendor initializer interchanges them. The module uses an eight-line bounce buffer, which divides the complete frame size as required by the ESP-IDF RGB driver.

### GT1158 Touch and Controller Identification

The tested 800x480 module reports panel ID `0x4384` and touch product ID `1158`. Its GT1158 uses the `espressif/esp_lcd_touch_gt1151` driver (`^1.1.1`), I2C1 (SDA GPIO39, SCL GPIO38), 7-bit address `0x14` (`0x28` in BMGR YAML), and INT GPIO40. The board factory controls XL9555 P9 directly, applying 200 ms reset-low and 200 ms startup waits before calling the driver. No virtual GPIO is used. The driver's built-in 10 ms waits failed to read the product ID on this module. Three consecutive BMGR touch initialization/deinitialization cycles passed with the longer timing.

The display remains `display_lcd`; the separate touch device is `lcd_touch`. The physical RGB color-bar test was confirmed complete and stable. Touch coordinates were received continuously, and the on-screen drawing test confirmed that position and direction follow the finger. These checks also passed with direct BMGR initialization, without the raw I2C identification probe. These physical results predate the switch to direct XL9555 reset control; the updated path still requires hardware revalidation. Other panel/touch variants require their own verification.

A 4.3-inch 800x480 panel does not identify its touch controller. The vendor `24_touch` example (`BSP/TOUCH/gt9xxx.c`) reads the product ID, recognizes `911`, `9147`, `1158` and `9271`, and prints `CTP ID:<model>` on the serial console. Run that example and inspect this line to identify the actual chip.

For a direct read, use I2C1 (SDA GPIO39, SCL GPIO38) and the vendor's 7-bit address `0x14` (`0x28/0x29` in its source include the R/W bit). Reset the touch controller via XL9555 P9 with the vendor timing, send register address bytes `0x81, 0x40`, and read the four ASCII product-ID bytes. INT is GPIO40. An address scan alone cannot distinguish these models; use the product ID or the marking on the touch IC itself. The existing `io` console command supports GPIO operations, not this I2C register read.

## On-board Sensors and Infrared

The base board and the RGB amend include two polling `custom` devices on `i2c_main` (SDA41/SCL42). Each adapter acquires its own BMGR bus reference and removes its I2C device before releasing that reference. Neither creates another bus nor installs an interrupt handler on the XL9555.

| Device name | Driver | Default configuration | Read API |
| ----------- | ------ | --------------------- | -------- |
| `ap3216c` | Board-local `atk_ap3216c` | 7-bit `0x1e`, 400 kHz, continuous ALS/PS/IR | `atk_ap3216c_read()` |
| `qma6100p` | `espressif/qma6100p ^2.0.1` | 7-bit `0x12`, ±8g | `qma6100p_get_acce()` |

These custom `i2c_addr` values are seven-bit addresses, passed directly to the driver. AP3216C returns raw ambient light, infrared intensity and proximity values, with IR/PS validity flags for overflow. Raw ALS is not lux and proximity is not a distance. Poll at intervals of at least 120 ms. QMA6100P reports three acceleration axes in g. Its configured `range_g` accepts 2, 4, 8, 16 or 32. The AP3216C IR measurement is separate from the remote-control receiver.

After BMGR initialization, read the borrowed driver handles as follows:

```c
#include "esp_board_device.h"
#include "atk_dnesp32s3_sensors.h"

atk_dnesp32s3_ap3216c_handles_t *light = NULL;
atk_dnesp32s3_qma6100p_handles_t *motion = NULL;
atk_ap3216c_sample_t sample;
qma6100p_acce_value_t acceleration;

ESP_ERROR_CHECK(esp_board_device_get_handle("ap3216c", (void **)&light));
ESP_ERROR_CHECK(atk_ap3216c_read(light->sensor, &sample));
ESP_ERROR_CHECK(esp_board_device_get_handle("qma6100p", (void **)&motion));
ESP_ERROR_CHECK(qma6100p_get_acce(motion->sensor, &acceleration));
```

Serialize sensor access with initialization/deinitialization. Driver handles remain owned by BMGR; do not delete them directly. Devices initialize with the board by default. Set `init_skip: true` in an application amend for on-demand initialization, or `gen_skip: true` to exclude a sensor and its dependency. AP3216C INT is wired to XL9555 P1 and QMA6100P INT to P0; these initial adapters use polling only. Sensor measurements still require physical-board validation.

The `rmt_ir_rx` (GPIO2) and `rmt_ir_tx` (GPIO8) peripherals both default to `init_skip: true`, with 1 MHz resolution. GPIO2 overlaps the SD card CS; deinitialize/exclude SD and disconnect the card before using IR RX. GPIO8 overlaps the RGB display bus; IR TX cannot be used alongside the RGB display. Select the vendor IR jumper before transmitting. Generation reports these potential pin conflicts even though the RMT peripherals are skipped at startup.

Explicitly acquire a peripheral with `esp_board_periph_ref_handle()`, then use `periph_rmt_handle_t.channel` with ESP-IDF RMT APIs. BMGR creates the channel; the application configures callbacks, the receive buffer or protocol encoder, the TX carrier (vendor example: 38 kHz, 1/3 duty), and calls `rmt_enable()`. Stop transfers, call `rmt_disable()`, and release the reference with `esp_board_periph_unref_handle()` before teardown. No NEC protocol device is registered by this board definition.

The buzzer already supports simple level control through `atk_dnesp32s3_xl9555_set_beep()`: XL9555 P3 low turns it on, high turns it off. No additional buzzer device or RMT channel is added.

## Hardware Debugging

The camera defaults enable DVP auto-detection for both OV2640 and OV5640; connect one camera module at a time.

XL9555 is a standard `gpio_expander` device. The board uses the expander API without GPIO mapping; board defaults explicitly disable the GPIO API wrapper. PA enable is XL9555 P2 and is active low. The base configuration enables PA after XL9555 initialization; the RGB amend keeps it off. The codec has no `pa_name` binding, so codec open/close does not switch PA. Applications can use `atk_dnesp32s3_xl9555_set_speaker(handle, enabled)` to manage it directly.

An ESP Board Manager test application with the `io` command can control the expander pins directly. The following audio test applies to the base SPI display configuration. RGB uses the I2S pins, so enabling PA does not restore audio playback in RGB mode.

```text
bmgr init xl9555
io get xl9555 2
io set xl9555 2 0
io set xl9555 2 1
io set xl9555 2 0
case run speaker.tone.1khz
```

PA changes use the same XL9555 output cache as the other board helpers. Regenerate and rebuild after updating the board definition. Native GPIO APIs must only be used with real SoC pins on this board.
