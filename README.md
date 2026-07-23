<!-- SPDX-License-Identifier: CC-BY-4.0 -->

# Bike Badge firmware

First UI/performance prototype for the Waveshare
`ESP32-S3-Touch-AMOLED-1.75` / `ESP32-S3-Touch-AMOLED-1.75-G`.

The prototype deliberately uses fake ride data. Its job is to validate the
display pipeline, round-screen layout, touch gestures and perceived animation
smoothness before GNSS and iPhone BLE are added.

## Included screens

1. Turn navigation: large turn geometry, distance, speed and ETA.
2. Route view: simplified local roads and an animated rider position.
3. Ride data: speed arc, trip distance and elapsed time.
4. Badge mode: a low-complexity animated electronic badge.

Swipe left/right to change pages. Tapping the left or right edge also changes
pages. The small `UI xx Hz` label measures the 33 ms animation timer cadence;
it is a responsiveness check, not a claim that every pixel was flushed that
many times.

## Toolchain

- ESP-IDF 5.5 or newer
- Target: ESP32-S3
- Waveshare BSP `waveshare/esp32_s3_touch_amoled_1_75` 3.0.1
- LVGL supplied through the BSP dependency graph

The BSP owns the CO5300 QSPI display initialization, CST9217 touch input and
LVGL display adapter. Dependencies are downloaded by the ESP-IDF Component
Manager during the first configure/build.

## Build and flash

From an ESP-IDF terminal:

```powershell
idf.py set-target esp32s3
idf.py build
idf.py -p COMx flash monitor
```

Replace `COMx` with the board's port. If flashing cannot start, hold `BOOT`,
tap reset/power, begin flashing, then release `BOOT`.

## Performance choices

- RGB565 rendering.
- 40-line LVGL draw strips instead of two 424 KiB full-screen buffers.
- 200 ms fake sensor/model updates.
- 33 ms animation cadence (about 30 Hz).
- Only the moving rider/badge dot and changed labels are invalidated normally.
- A full-screen animated transfer happens only during page changes.

## Next milestones

1. Build and flash this baseline; record page-transition FPS and touch latency.
2. Tune buffer height and display adapter settings on the real board.
3. Add LC76G GNSS behind a navigation service interface.
4. Add BLE route/state messages from the iPhone companion app.
5. Cache route geometry so navigation survives a phone disconnect.

## Project layout

```text
main/
  app_main.c          Board/display startup only
  ui/
    ui_app.c          LVGL screens, gestures and animations
    ui_model.c        Deterministic fake ride data
sdkconfig.defaults    Performance-oriented board defaults
```

## Licensing

Bike Badge uses licenses matched to each kind of work:

- firmware, software, and build configuration: Apache-2.0;
- original hardware design sources: CERN-OHL-W-2.0; and
- original documentation and non-brand creative assets: CC BY 4.0.

Project names and brand assets are not automatically licensed. See
[`LICENSING.md`](LICENSING.md) for the exact scope and
[`TRADEMARKS.md`](TRADEMARKS.md) for brand-use boundaries.
