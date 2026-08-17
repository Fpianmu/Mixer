#!/usr/bin/env bash
# 一键更新固件并推送到仓库
# 用法: ./firmware/refresh.sh [提交信息]
# 默认使用 ESP-IDF 路径, 可用环境变量 IDF_PATH 覆盖
set -euo pipefail

IDF_PATH="${IDF_PATH:-/home/pianmu/esp/v601/esp-idf}"
PROJECT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
MSG="${1:-update firmware}"

cd "$PROJECT_DIR"

echo "==> 1/4 编译"
. "$IDF_PATH/export.sh" >/dev/null 2>&1
idf.py build

echo "==> 2/4 合并固件 (bootloader + 分区表 + app)"
mkdir -p firmware
esptool --chip esp32s3 merge-bin \
  --flash-mode dio --flash-freq 80m --flash-size 16MB \
  -o firmware/merged-flash.bin \
  0x0 build/bootloader/bootloader.bin \
  0x8000 build/partition_table/partition-table.bin \
  0x10000 build/project.bin

echo "==> 3/4 提交"
git add firmware/merged-flash.bin
git commit -m "$MSG" || echo "(固件无变化, 跳过提交)"

echo "==> 4/4 推送"
git push

echo "完成: firmware/merged-flash.bin 已更新, 合作者可直接烧录."
