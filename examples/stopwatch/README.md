# StopWatch / native ESP-IDF example

M5Stack StopWatch: ESP32-S3R8, 16MB QIO Flash, 8MB OPI PSRAM, 466×466 CO5300.
Uses native M5Unified 0.2.21 + M5GFX 0.2.28 managed components. No Arduino dependency.
This board example targets ESP-IDF >=6.1,<7.0; 6.1 is the local validation environment. Third-party libraries may have
additional version/platform restrictions; the engine itself needs only C++17.

From repository root on this machine:

```sh
./tools/build_stopwatch_idf.sh                       # generate data + build only
./tools/build_stopwatch_idf.sh -p /dev/cu.usbmodem21201 flash
./tools/build_stopwatch_idf.sh -p /dev/cu.usbmodem21201 monitor
```

The helper uses T7 for toolchain, component download cache, temporary files and
project build output. Override `IDF_PATH`, `IDF_TOOLS_PATH`, `TMPDIR_EYES`, and
`IDF_COMPONENT_CACHE_PATH` to use another installation. `flash` also builds changed sources before uploading (ESP-IDF default behavior).
`monitor` only opens the log console; exit with Ctrl+].

With an already activated ESP-IDF environment, the portable equivalent is:

```sh
python3 tools/generate_presets.py
python3 tools/build_web.py
cd examples/stopwatch
idf.py set-target esp32s3
idf.py build
idf.py -p /dev/cu.usbmodem21201 flash monitor
```

If connection fails, hold Reset for about 2 seconds until the internal green LED
lights, release, and retry with the resulting USB port. Flashing replaces the
running app. This example uses a factory partition rather than OTA slots.

A cycles through all expressions, B blinks. Startup and expression IDs appear on
the USB Serial/JTAG console at 115200. The legacy numeric Arduino serial commands
are not included in this minimal IDF example. Add app-specific input handling
around `engine.set_expression("id")` in `main/main.cpp`.

`main.cpp` owns M5 initialization, brightness, clock, RNG seed, buttons and task
scheduling. The reusable `eyes` component owns animation and drawing only. Device
screen initialization is delegated to M5Unified/M5GFX automatic detection.

## ESP-IDF 6.1 SPI compatibility

M5GFX 0.2.28's legacy SPI probe fills the bus config with `0xff`. IDF 6.1 adds
`dma_burst_size`; leaving it at `UINT32_MAX` causes a GDMA error and a reboot
inside `M5.begin()`. The example links `main/spi_compat.c` to normalize this unset
field to the documented driver default (0). Valid configs pass through unchanged.
The linker explicitly retains the wrapper, including when libraries are archived.
This workaround is local to the board example and survives dependency reinstalls;
no changes to ESP-IDF or managed components are needed. Keep the wrapper and its
CMake link options together if you reuse this example with these versions.

Boot diagnostics distinguish `Ready: 468x468` (M5GFX canvas for the 466×466 panel) from `First frame rendered` so a
successful upload alone isn't mistaken for a working application.
