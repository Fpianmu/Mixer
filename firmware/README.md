# 固件下载与烧录

这里提供已经编译好的**合并固件**，克隆仓库后无需编译即可直接烧录到 ESP32-S3。

## 文件说明

- `merged-flash.bin`：已合并 bootloader + 分区表 + 应用程序，烧录到地址 `0x0` 即可。

## 烧录步骤

### 1. 安装 esptool（如果还没装）

```bash
pip install esptool
```

### 2. 烧录（USB 连接开发板）

```bash
esptool.py --chip esp32s3 write_flash 0x0 merged-flash.bin
```

- 端口一般会自动识别；若没识别，手动指定：
  - Windows：`--port COM3`
  - Linux/macOS：`--port /dev/ttyUSB0`
- 烧录时按住开发板上的 BOOT 键（进入下载模式），完成后松开并按一下复位键（或重新上电）。

## 固件信息

- 芯片：ESP32-S3
- Flash：16MB（DIO，80MHz）
- 设备 ID：`esp32_001`
- 分区布局：bootloader `0x0` + 分区表 `0x8000` + 应用 `0x10000`

## 从源码编译

如需自己修改并编译，参考项目根目录，使用 ESP-IDF v6.0.1 构建：

```bash
idf.py build
```

编译产物在 `build/` 目录下。
