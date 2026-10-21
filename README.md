# Eyes Lab — Two-Eye Procedural UI Demo

一个只用两只眼睛表达表情的可移植 UI 实验项目。

## 立即体验：Mac / Windows / Linux / 手机浏览器

直接双击 `web/index.html` 即可。HTML / CSS / JS 完全离线，无任何依赖。

- 10 个情绪预设：平静、开心、兴奋、思考、生气、难过、惊讶、疑惑、困倦、眨单眼
- Keyframe pose + 二阶阻尼 Spring 插值
- 随机注视点、自动眨眼、呼吸、微动
- 鼠标 / 触摸跟踪、自动巡演、过渡速度和活跃度调整
- 圆屏 466×466 与方屏 320×240 预览
- 快捷键 1–9、0 切换表情；空格眨眼；左右方向键切换

## 原生 ESP-IDF 组件（推荐）

`components/eyes` 是独立 C++17 组件，包含表情切换、弹簧插值、眨眼、注视和绘图。
不依赖 Arduino；通过 `eyes::Canvas` 接入屏幕。StopWatch 示例用原生
M5Unified/M5GFX 初始化硬件，按 A 循环全部表情，按 B 眨眼。

```bash
./tools/build_stopwatch_idf.sh   # 本机 T7 环境：生成表情数据、更新网页并编译
# 烧录需单独执行：
./tools/build_stopwatch_idf.sh -p /dev/cu.usbmodem21201 flash
```

- [组件接入与 API](components/eyes/README.md)
- [StopWatch 配置、构建与烧录](examples/stopwatch/README.md)

增加表情仍只需修改 `presets.json`，运行下面的生成命令并重新编译烧录。
组件按 ID 切换（`engine.set_expression("happy")`），按键循环自动涵盖新增表情。

## 旧版 Arduino 固件（保留基线）

使用 Arduino CLI / Arduino IDE 构建 `firmware/eyes_demo/eyes_demo.ino`，依赖 `M5Unified`（内含 M5GFX）。适配设备屏幕大小，不依赖 LVGL。以 `M5.Display` 做直接绘制；渲染两只参数化胶囊眼睛，局部擦除旧区域。

Arduino CLI 示例：

```bash
arduino-cli lib install M5Unified
arduino-cli compile --fqbn 'esp32:esp32:esp32s3:PSRAM=opi,FlashSize=16M' firmware/eyes_demo
```

设备特定 FQBN、分区、Flash/PSRAM 参数请根据实际板卡调整。旧版已通过 StopWatch 编译与烧录，画面效果仍以实机为准。后续组件开发以原生 IDF 工程为主。

操作：按钮 A 切换表情，按钮 B 触发眨眼。串口输入 `1..9`、`0` 切换表情，`b` 眨眼。

## 结构

```
web/index.html                 浏览器 Demo（可直接打开）
web/index.template.html        HTML 源模板
presets.json                   统一表情参数源
firmware/eyes_demo/eyes_demo.ino  M5Stack 固件示例
firmware/eyes_demo/presets.h   自动生成的旧版 C++ 数据
components/eyes/              可复用 ESP-IDF 组件
examples/stopwatch/           原生 ESP-IDF StopWatch 工程
 tools/generate_presets.py     生成 C++ 表情参数
```

表情参数的唯一来源是 `presets.json`。修改后执行：

```bash
python3 tools/generate_presets.py
```

随后如需同步浏览器内嵌参数，运行仓库根目录 `python3 tools/build_web.py`。

## 当前边界与下一步

- 浏览器与 ESP32 使用相同的 8 维 EyePose 数据和运动思路，但渲染器不是同一份二进制代码，部分眼皮与曲线细节不完全一致。
- 设备固件采用直接 M5GFX 绘图；如果之后要把它集成到 LVGL 的图片/菜单上，必须增加 **离屏缓冲 / LVGL 自定义组件**，不要两个渲染任务争抢屏幕。
- 当前 60FPS 为目标刷新间隔，不是实测设备性能。真实效果应以屏幕型号、SPI 速度和 PSRAM 配置为准。
- 原生 IDF 路径已将动画和绘图提取到 `components/eyes`，M5GFX 通过 Canvas 适配；LVGL Renderer 和桌面可视预览尚未实现。

许可证：这份 Demo 代码可以按 MIT 协议使用；第三方库仍按其各自许可证。
