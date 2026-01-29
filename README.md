# OSRST 项目：Genijoy SR 脚本格式转换工具
# OSRST Project: Genijoy SR Script Format Conversion Tools

---

## 项目简介 / Project Overview

本项目包含两个核心工具，用于处理 Genijoy SR 系列设备所使用的脚本文件。
This project includes two core utilities for handling script files used by the Genijoy SR series devices.

1.  **转换器 (OSR Script Transform | OSRST)** - 将 `.funscript` 文件转换为 `.srsb` 二进制格式。
    **Converter (OSR Script Transform | OSRST)** - Transforms `.funscript` files into the `.srsb` binary format.

2.  **读取器/解析器 (OSR Script Parser | OSRSP)** - 加载并解析 `.srsb` 二进制文件。
    **Reader/Parser (OSR Script Parser | OSRSP)** - Loads and interprets `.srsb` binary files.

---

## 开发背景 / Background

这些程序专为 Genijoy SR 系列设备而设计。该设备支持通过 SD 卡离线运行模式。
These programs are designed for the Genijoy SR series. The device supports an offline operation mode via SD card.

然而，若让设备直接读取 `.funscript` 文件，则需要在设备本地进行大规模的 JSON 解析。这对于 Genijoy SR 这样的**边缘计算设备**而言，会产生不小的内存开销，可能影响性能与响应速度。
However, directly reading `.funscript` files on the device would require extensive JSON parsing locally. This imposes significant memory overhead on an **edge computing device** like the Genijoy SR, potentially impacting performance and responsiveness.

为了解决这一问题，我们开发了此格式转换工具。通过将 `.funscript` 转换为紧凑的二进制脚本格式 (`.srsb`)，可以**大幅减少**设备加载脚本时的内存开销，从而提升运行效率。
To address this, this format conversion tool was developed. By transforming `.funscript` into a compact binary script format (`.srsb`), it **substantially reduces** the memory overhead associated with the device loading scripts, thereby improving operational efficiency.

---

## 使用流程 / Workflow

1.  在电脑上使用 **OSRST (转换器)**，将现有的 `.funscript` 文件转换为 `.srsb` 格式。
    Use **OSRST (Converter)** on your computer to convert existing `.funscript` files to `.srsb` format.

2.  将生成的 `.srsb` 文件拷贝到 Genijoy SR 设备的 SD 卡中。
    Copy the generated `.srsb` files to the SD card of your Genijoy SR device.

3.  Genijoy SR 设备内置的 **OSRSP (解析器)** 将直接、高效地读取并运行 `.srsb` 文件。
    The built-in **OSRSP (Parser)** on the Genijoy SR device will directly and efficiently read and execute the `.srsb` files.