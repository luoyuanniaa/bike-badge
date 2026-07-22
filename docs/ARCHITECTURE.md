# Prototype architecture

The first prototype keeps the UI independent from live GNSS and BLE. The fake
model will later be replaced by the same event-shaped data from real services.

```text
GNSS service  ----+
                  +--> navigation state --> UI model --> LVGL
iPhone BLE   -----+

UI task       33 ms animation cadence and dirty-region redraws
Nav service  200-1000 ms position/route updates
Storage      asynchronous ride log writes
```

Rules for future modules:

- Do not call LVGL from a GNSS, BLE or storage callback.
- Pass small immutable state messages to the UI task.
- Keep route search and full map data on the phone.
- Cache the route polyline and turn list on the device.
- Prefer board GNSS when its fix is fresh and accurate; fall back to phone
  location when the onboard fix is unavailable.
- Keep display animation timing independent from GPS update frequency.
