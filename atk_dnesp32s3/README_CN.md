# ATK-DNESP32S3 开发板

[English](README.md) | [组件说明](../README_CN.md)

`atk_dnesp32s3` 提供正点原子 ATK-DNESP32S3（ESP32-S3）开发板定义。基础配置使用 ST7789 SPI 屏和 ES8388 音频芯片，并提供摄像头、SD 卡、XL9555 IO 扩展、按键、LED、传感器及按需初始化的红外 RMT 外设。

## 开发板选择

在已安装本组件的 ESP-IDF 应用工程中执行：

```bash
idf.py bmgr -b atk_dnesp32s3
idf.py build
```

板级定义分别位于 [board_devices.yaml](board_devices.yaml)、[board_peripherals.yaml](board_peripherals.yaml) 和 [sdkconfig.defaults.board](sdkconfig.defaults.board)。板级默认只启用 ES8388 codec 驱动。已有 `sdkconfig` 中的显式配置需要单独同步。

## 选择 RGB 屏幕模组

基础配置使用 ST7789 SPI 屏。改用正点原子 4.3 寸 800×480 RGB 屏时，通过 `-a` 显式选择板目录下的 amend 模块：

| amend 模块 | 厂家例程屏幕 ID | 分辨率 | 像素时钟 |
| ---------- | --------------- | ------ | -------- |
| `sub_board_rgb_800_480` | `0x4384` | 800 × 480 | 18 MHz |

```bash
idf.py bmgr -b atk_dnesp32s3 -a sub_board_rgb_800_480
idf.py build

# 重新选择基础 SPI 屏。
idf.py bmgr -b atk_dnesp32s3
```

amend 直接将原有 `display_lcd` 设备从 SPI 改为 RGB，生成结果只有一个显示设备，应用和测试 case 都继续使用原名称。RGB 使用 16 位 RGB565 总线、DE 模式（DE 为 GPIO4，PCLK 为 GPIO5，未连接 HSYNC/VSYNC），帧缓冲为 PSRAM 中的 `RGB565_LE`。背光和屏幕电源沿用 XL9555 的输出。RGB 引脚与摄像头、I2S 音频重叠，因此 amend 会从生成配置中排除 `camera`、`audio_dac`、`audio_adc` 及其专用外设，并将摄像头 PWDN 拉高；SD 卡和按键继续保留，`spi_shared` 继续供 SD 卡使用。继承的 SPI 配置不参与 RGB 驱动初始化，也不会创建 SPI 屏幕 IO。

模块已提供 80 MHz Octal PSRAM 配置。切换模组后需重新生成配置；已有 `sdkconfig` 请确认相应 PSRAM 选项已启用。使用 RGB 时只连接选定屏幕，并拔下摄像头。800×480 amend 同时注册实测模组的 GT1158 触摸设备 `lcd_touch`。设备和外设配置集中在 [rgb_panel.yaml](sub_board_rgb_800_480/rgb_panel.yaml) 中。

接线和时序来自厂家 `23_rgb` / `24_touch` 例程的 `BSP/RGBLCD/ltdc.h`、`ltdc.c`。800×480 的同步脉宽按厂家声明的 `hsw=48`、`vsw=3` 分别对应水平和垂直方向，修正其初始化结构体中两者互换的写法。该模组使用 8 行 bounce buffer，保证完整帧大小可被 buffer 大小整除，符合 ESP-IDF RGB 驱动要求。

### GT1158 触摸与芯片识别

本次实测 800×480 模组的屏幕识别码为 `0x4384`，触摸产品 ID 为 `1158`。GT1158 使用 `espressif/esp_lcd_touch_gt1151` 驱动（`^1.1.1`），接 I2C1（SDA GPIO39、SCL GPIO38），7 位地址为 `0x14`（BMGR YAML 中为 `0x28`），INT 为 GPIO40。板级工厂函数直接控制 XL9555 P9，先复位拉低 200 ms、释放后等待 200 ms，再调用触摸驱动，不使用虚拟 GPIO。驱动原有的 10 ms 等待在该模组上无法成功读取产品 ID，调整后已通过连续三次 BMGR 初始化/释放验证。

显示设备仍叫 `display_lcd`，独立的触摸设备名为 `lcd_touch`。RGB 四条色带已由实物确认显示完整、稳定；串口连续收到有效触摸坐标，实物画线测试确认轨迹跟手、位置和方向正确。移除裸 I2C 探测步骤、直接通过 BMGR 初始化后，上述测试同样通过。上述实测结果来自本次改为直接控制 XL9555 复位之前的固件，新复位路径仍待实板复测。其他屏幕或触摸芯片变体需要另行验证。

4.3 寸、800×480 只能确定屏幕规格，不能确定触摸芯片。厂家 `24_touch` 例程的 `BSP/TOUCH/gt9xxx.c` 会读取产品 ID，支持识别 `911`、`9147`、`1158`、`9271`，并在串口打印 `CTP ID:<型号>`。运行该例程并查看这行输出即可确认实际芯片。

自行读取时，使用 I2C1（SDA GPIO39、SCL GPIO38），厂家使用的 7 位地址为 `0x14`（其代码中的 `0x28/0x29` 是含读写位的地址字节）。按厂家时序通过 XL9555 P9 复位触摸芯片后，向设备发送寄存器地址字节 `0x81, 0x40`，读取 4 字节 ASCII 产品 ID。INT 接 GPIO40。仅扫描到地址不能区分这些型号；应以产品 ID 或触摸芯片本体丝印为准。现有 `io` 控制台命令只支持 GPIO，不支持这项 I2C 寄存器读取。

## 板载传感器与红外

基础配置和 RGB amend 都保留两个轮询方式的 `custom` 设备，共用 `i2c_main`（SDA41/SCL42）。每个适配入口各自获取 BMGR 总线引用，释放时先删除 I2C 设备，再归还引用，不额外创建总线，也不在 XL9555 上安装 GPIO 中断。

| 设备名 | 驱动 | 默认配置 | 读取接口 |
| ------ | ---- | -------- | -------- |
| `ap3216c` | 板内 `atk_ap3216c` | 7 位地址 `0x1e`、400 kHz、连续 ALS/PS/IR | `atk_ap3216c_read()` |
| `qma6100p` | `espressif/qma6100p ^2.0.1` | 7 位地址 `0x12`、±8g | `qma6100p_get_acce()` |

这两个 custom 的 `i2c_addr` 直接传给驱动，使用 **7 位地址**。AP3216C 返回环境光、红外强度和接近值的原始数据，并用 IR/PS 有效标志表示溢出；ALS 原始值不是 lux，接近值也不是距离。建议读取间隔至少 120 ms。QMA6100P 返回以 g 为单位的三轴加速度，`range_g` 支持 2、4、8、16、32。AP3216C 的红外强度检测与遥控器接收头是不同的硬件功能。

BMGR 初始化后，通过板级句柄获取底层驱动句柄：

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

应用需将读取与初始化、释放操作串行化；底层句柄由 BMGR 管理，不要直接删除。这两个设备默认随板初始化。可在应用 amend 中设置 `init_skip: true` 改为按需初始化，或用 `gen_skip: true` 从生成结果和依赖中排除。AP3216C INT 接 XL9555 P1，QMA6100P INT 接 P0，本次只实现轮询，不启用中断。传感器实际测量值仍待实板验证。

红外外设 `rmt_ir_rx`（GPIO2）和 `rmt_ir_tx`（GPIO8）均默认 `init_skip: true`，分辨率为 1 MHz。GPIO2 与 SD 卡 CS 重叠，使用 RX 前应释放/排除 SD 设备并拔卡；GPIO8 与 RGB 屏数据线重叠，TX 不能与 RGB 屏同时使用。发射前需按厂家说明选择红外跳线。生成时仍会报告这些潜在引脚冲突，默认启动不会创建 RMT 通道。

应用通过 `esp_board_periph_ref_handle()` 显式获取外设，然后使用 `periph_rmt_handle_t.channel` 调用 ESP-IDF RMT 接口。BMGR 只创建通道；接收回调、缓冲区、协议编码器、TX 载波（厂家例程为 38 kHz、1/3 占空比）和 `rmt_enable()` 由应用设置。释放前先停止传输、调用 `rmt_disable()`，再用 `esp_board_periph_unref_handle()` 归还引用。本次不新增 NEC 协议设备。

蜂鸣器沿用 `atk_dnesp32s3_xl9555_set_beep()`：XL9555 P3拉低响、拉高停，不另加蜂鸣器设备或 RMT 通道。

## 硬件调试

摄像头默认配置同时启用 OV2640 和 OV5640 的 DVP 自动检测，实际使用其中一个模块。

XL9555 使用标准 `gpio_expander` 设备，通过扩展器 API 直接控制，不使用 GPIO mapping，板级 defaults 已显式关闭 GPIO API wrapper。PA 使能脚为 XL9555 P2，低电平有效。基础配置在 XL9555 初始化后开启 PA；RGB amend 保持 PA 关闭。codec 不再绑定 `pa_name`，因此 open/close 不会自动切换 PA。应用可用 `atk_dnesp32s3_xl9555_set_speaker(handle, enabled)` 直接开关功放。

配合支持 `io` 命令的 ESP Board Manager 测试应用，可以直接操作扩展器引脚。以下音频测试适用于基础 SPI 屏配置；RGB 模式的 I2S 引脚已被屏幕占用，不能通过开启 PA 恢复音频播放。

```text
bmgr init xl9555
io get xl9555 2
io set xl9555 2 0
io set xl9555 2 1
io set xl9555 2 0
case run speaker.tone.1khz
```

这些操作与板级 helper 共用 XL9555 输出缓存。修改板定义后需重新生成并构建；本板的普通 GPIO API 只用于真实 SoC 引脚。
