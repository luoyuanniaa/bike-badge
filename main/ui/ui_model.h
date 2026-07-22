#pragma once

#include <stdbool.h>
#include <stdint.h>

typedef struct {
    uint16_t speed_x10;
    uint16_t turn_distance_m;
    uint32_t trip_distance_m;
    uint32_t elapsed_s;
    uint8_t battery_percent;
    uint8_t route_progress;
    bool gps_fix;
    bool phone_connected;
} ui_ride_state_t;

void ui_model_reset(ui_ride_state_t *state);
void ui_model_step(ui_ride_state_t *state);
