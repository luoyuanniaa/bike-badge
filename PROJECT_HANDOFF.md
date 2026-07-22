# Bike Badge project handoff

This file is the durable context for continuing the project from another
computer or a new Codex chat. Read it together with `README.md` and
`docs/ARCHITECTURE.md` before changing the firmware or creating the iPhone app.

## Product intent

Bike Badge is an open-source round bicycle computer that remains useful after
a ride. On the bike it is a glanceable navigation and ride-data display. Off
the bike, the round display detaches from its mount and becomes an electronic
badge/pendant that can hang from a bag or clothing.

The industrial-design direction combines a premium octagonal/sports-watch feel
with a playful translucent consumer-product character. Audemars Piguet and
Swatch are aesthetic references only; the project must develop its own design
language and must not reproduce protected brand marks or product geometry.

The direct UX benchmark is Beeline Velo 2: simple, glanceable navigation and
low visual complexity. Bike Badge should add independent onboard positioning,
offline route continuation, and an off-bike badge mode.

## Hardware baseline

- Board: Waveshare `ESP32-S3-Touch-AMOLED-1.75`, with the LC76G GNSS accessory
  when using the `-G` package.
- MCU: ESP32-S3, dual core, up to 240 MHz.
- Memory: 8 MB PSRAM and 16 MB flash.
- Display: 1.75-inch round 466 x 466 AMOLED, CO5300 over QSPI.
- Touch: CST9217 capacitive touch.
- Power management: AXP2101.
- Motion and time: QMI8658 IMU and PCF85063 RTC.
- Storage and audio are available on the development board but are not core MVP
  requirements.
- GNSS: LC76G over I2C in the official example; the example uses SCL GPIO 14,
  SDA GPIO 15 and emits NMEA data.

Official board repository:
<https://github.com/waveshareteam/ESP32-S3-Touch-AMOLED-1.75>

Official firmware baseline:

- ESP-IDF 5.5.4 for the first reproducible prototype.
- Waveshare managed BSP
  `waveshare/esp32_s3_touch_amoled_1_75` version 3.0.1.
- LVGL 9 through the BSP dependency graph.

## Battery and enclosure assumptions

- CAD indicates a `562438` single-cell pouch battery, nominally about
  5.6 x 24 x 38 mm.
- Use a 3.7 V cell charged to 4.2 V unless the exact battery and PMIC profile
  explicitly support a different chemistry.
- Capacity depends on supplier and must be verified from the purchased cell;
  typical listings are roughly 400-500 mAh.
- Prefer a protected pack and verify the MX1.25 two-pin connector polarity
  before connecting it. Connector polarity is not universal.
- Leave mechanical clearance for tabs, wires and normal pouch swelling. Do not
  hard-clamp the cell.
- A realistic first-prototype target is about 3-5 hours of active navigation,
  subject to measurement. Beeline-class all-day runtime probably requires a
  custom PCB and aggressive power optimisation.

## Display and UI decisions

Keep AMOLED for the prototype. The badge mode and high-contrast premium visual
identity benefit from true black and saturated colour.

Performance target:

- Stable 30 Hz overall UI cadence.
- Small local animations may approach 60 Hz.
- Avoid full-screen 60 Hz animation.
- RGB565 rendering.
- Partial dirty-region updates.
- Approximately 40-line DMA/LVGL draw strips instead of two full-screen
  buffers. A 466 x 466 RGB565 frame is about 424 KiB.
- UI timing must remain independent from GNSS, BLE and storage update rates.

Implemented prototype screens:

1. Turn navigation.
2. Simplified route overview.
3. Ride statistics.
4. Electronic badge mode.

Swipe left/right or tap the screen edges to switch pages. The current UI uses
deterministic fake data so hardware smoothness can be measured before live
services are introduced.

Asset pipeline:

- Draw navigation geometry, roads, rings, labels and simple icons in LVGL.
- Use LVGL-converted RGB565 images for opaque artwork.
- Use RGB565A8 only where transparent edges are necessary.
- Store user-provided badge art as files rather than compiling every image into
  the firmware.
- Prefer the LVGL converter over the generic Waveshare Image2Lcd workflow.

## Firmware status

The existing ESP-IDF prototype is in this repository root.

Important files:

```text
CMakeLists.txt
sdkconfig.defaults
main/
  app_main.c
  idf_component.yml
  ui/
    ui_app.c
    ui_app.h
    ui_model.c
    ui_model.h
docs/
  ARCHITECTURE.md
```

`app_main.c` starts the official BSP display adapter with
`bsp_display_start()`, sets brightness and starts the LVGL UI. The project has
not yet been compiled or flashed in the original Windows workspace because the
ESP-IDF toolchain was not installed there.

First Mac firmware validation:

1. Install and activate ESP-IDF 5.5.4.
2. Build and flash Waveshare's official `02_lvgl_demo_v9` example to verify the
   board, touch controller and USB setup.
3. Build this repository with `idf.py set-target esp32s3` and `idf.py build`.
4. Flash the Bike Badge prototype and record page-transition smoothness, touch
   latency and current draw at several brightness levels.

## Navigation responsibility split

The product uses a dual-track architecture:

- iPhone: destination search, route calculation, alternative routes and
  rerouting.
- Device: display, ride state, local route cache and navigation progress using
  onboard GNSS.
- Phone location is a fallback when the device GNSS fix is stale or missing.
- The device must continue along the cached route when the phone disconnects.

Do not run a full map engine or store map tiles on the ESP32. The phone sends a
simplified polyline, manoeuvre list, street names and route metadata. The device
renders only the local route geometry and current instruction.

## Planned BLE surfaces

Use the ESP32 as a BLE peripheral and the iPhone as the central.

- Control: handshake, protocol version, time sync, ride start/pause/end and
  mode changes.
- Navigation: route metadata, polyline chunks, manoeuvres and reroute updates.
- Telemetry: battery, charging, speed, distance, GPS quality and connection
  state.
- Transfer: ride-track download and badge-image upload.

The UUIDs and byte-level packet format are intentionally not final yet. Define
them once in a shared `protocol/` specification, then generate or mirror the
constants in C and Swift. Route/image transfer needs chunking, acknowledgement,
versioning and checksums. Avoid large JSON payloads for the production BLE
protocol.

## iPhone companion app

Build the companion app natively in Swift and SwiftUI. It is a route brain and
sync centre, not a mirror of the round display.

Planned modules:

```text
BikeBadgeApp
DeviceLink       CoreBluetooth central and protocol transport
LocationService  Core Location and background ride tracking
RoutingService   MapKit first, replaceable provider boundary
RideSession      Current ride state and track
RideStore        Local ride persistence
HealthSync       HealthKit workout/distance/route export
BadgeStudio      Circular crop, preview, conversion and upload
Views            Route, Ride, Badge and History screens
```

Current Apple SDKs expose cycling routes through
`MKDirectionsTransportType.cycling`. This was introduced with the WWDC25 SDK
generation, so older iOS deployment targets require an availability guard and
an alternative routing provider. For the fastest prototype, use it directly if
the test iPhone is running iOS 26.

Required Xcode capabilities for the working prototype:

- Background Modes: Location updates.
- Background Modes: Uses Bluetooth LE accessories.
- HealthKit, when Health sync is implemented.

Request location access in context when the rider starts a ride. Start with
When In Use permission; request Always only if a later product requirement
really needs unattended relaunch or tracking.

HealthKit is a post-MVP integration. At ride completion, write an outdoor
cycling workout, total distance and an `HKWorkoutRoute`. Do not treat the
Health app as a real-time sensor bus. Live Apple Watch heart rate would require
a watchOS companion/workout session; an external BLE heart-rate sensor is a
simpler earlier option.

## iPhone development order

1. Create `ios/BikeBadge` as a SwiftUI iOS project in Xcode.
2. Scan for and connect to the Bike Badge BLE service.
3. Send a fake instruction such as "left in 180 m" and update the device UI.
4. Receive device battery and GPS status.
5. Add iPhone location and background ride state.
6. Search a destination and calculate a cycling route.
7. Transfer and cache a complete route on the device.
8. Add rerouting, ride sync, HealthKit and badge-image upload.

The first end-to-end milestone is intentionally small: the iPhone connects,
sends one simulated navigation instruction, and receives battery telemetry.
This validates the most important cross-platform boundary before map and health
features increase complexity.

## Repository direction

This should become a monorepo:

```text
firmware/ or current ESP-IDF root
ios/
protocol/
hardware/
docs/
```

The current firmware can remain at the repository root for the first milestone;
avoid a disruptive move until the iOS project exists. Do not commit signing
certificates, provisioning profiles, API keys, personal Xcode user data or
generated build directories.

An open-source licence still needs an explicit project decision. A reasonable
future split is Apache-2.0 for firmware/app software and CERN-OHL-S-2.0 for
hardware design files, but do not add licences until the owner confirms.

## Immediate next task on macOS

After cloning the repository on the Mac, start a new Codex chat with:

> Read `PROJECT_HANDOFF.md`, `README.md` and `docs/ARCHITECTURE.md` completely.
> Continue the Bike Badge project by creating the minimal SwiftUI iPhone app in
> `ios/BikeBadge`: BLE scan/connect, a simulated navigation command, and battery
> telemetry. Preserve the ESP-IDF firmware architecture and define shared
> protocol constants under `protocol/`.

Before writing the iPhone code, confirm the Xcode version and the test iPhone's
iOS version so MapKit cycling availability is handled correctly.
