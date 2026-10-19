# 正点原子 ESP Board Manager 板组件

[English](README.md)

本组件提供正点原子（ALIENTEK）开发板的 ESP Board Manager 板级定义，包括设备与外设 YAML 配置、板级默认配置、初始化适配代码和可选 amend 模块。
应用可从 [ESP 组件注册库](https://components.espressif.com/components/openedv/atk_esp32_boards)安装本组件，通过 `idf.py bmgr` 选择开发板并生成配置。

## 支持的开发板

| 开发板 | 芯片 | 板级说明 |
| ------ | ---- | -------- |
| `atk_dnesp32s3` | ESP32-S3 | [ATK-DNESP32S3](atk_dnesp32s3/README_CN.md) |

屏幕选择、硬件资源、引脚冲突、设备接口和调试方法见对应的板级说明。

## 安装与使用

在已激活的 ESP-IDF Python 环境中安装辅助工具：

```bash
python -m pip install --upgrade esp-bmgr-assist
```

在应用工程中添加本组件，并列出可用开发板：

```bash
idf.py add-dependency "openedv/atk_esp32_boards"
idf.py bmgr -l
```

选择列表中的开发板并构建，将 `<board_name>` 替换为实际名称：

```bash
idf.py bmgr -b <board_name>
idf.py build
```

本组件已声明 `espressif/esp_board_manager` 依赖。除非应用需要更严格的版本限制，否则不必再次添加。可选 amend 模块的选择命令见对应的板级说明。

## 许可证

本组件采用 [Apache-2.0](LICENSE)。迁移 BSP 或引入第三方内容时，请保留并遵守其原有版权声明和许可证。
