# ALIENTEK ESP Board Manager Board Pack

[中文](README_CN.md)

This component provides ESP Board Manager definitions for ALIENTEK development boards, including device and peripheral YAML configuration, board defaults, initialization adapters and optional amend modules. Applications can install it from the [ESP Component Registry](https://components.espressif.com/components/openedv/atk_esp32_boards) and select a board with `idf.py bmgr` to generate its configuration.

## Supported Boards

| Board | Chip | Board Guide |
| ----- | ---- | ----------- |
| `atk_dnesp32s3` | ESP32-S3 | [ATK-DNESP32S3](atk_dnesp32s3/README.md) |

See each board guide for display selection, hardware resources, pin conflicts, device APIs and debugging instructions.

## Installation and Usage

Install the helper in the active ESP-IDF Python environment:

```bash
python -m pip install --upgrade esp-bmgr-assist
```

Add this component to the application and list the available boards:

```bash
idf.py add-dependency "openedv/atk_esp32_boards"
idf.py bmgr -l
```

Select a board from the list and build. Replace `<board_name>` with its name:

```bash
idf.py bmgr -b <board_name>
idf.py build
```

This component already declares the `espressif/esp_board_manager` dependency. Applications do not need to add it again unless they need a tighter version constraint. See each board guide for optional amend selection commands.

## License

This component uses [Apache-2.0](LICENSE). Preserve and comply with existing copyright notices and licenses when migrating a BSP or adding third-party content.
