#include "ui_model.h"

#include <string.h>

static uint32_t s_demo_tick;

void ui_model_reset(ui_ride_state_t *state)
{
    memset(state, 0, sizeof(*state));
    state->speed_x10 = 248;
    state->turn_distance_m = 180;
    state->trip_distance_m = 12400;
    state->elapsed_s = 38 * 60 + 12;
    state->battery_percent = 82;
    state->route_progress = 43;
    state->gps_fix = true;
    state->phone_connected = true;
    s_demo_tick = 0;
}

void ui_model_step(ui_ride_state_t *state)
{
    ++s_demo_tick;

    /* A cheap triangle wave keeps the demo deterministic and avoids float work. */
    uint32_t wave = s_demo_tick % 80;
    if (wave > 40) {
        wave = 80 - wave;
    }
    state->speed_x10 = (uint16_t)(224 + wave);

    if (state->turn_distance_m > 2) {
        state->turn_distance_m -= 1;
    } else {
        state->turn_distance_m = 180;
    }

    state->trip_distance_m += 1;
    if ((s_demo_tick % 5) == 0) {
        state->elapsed_s += 1;
    }
    if ((s_demo_tick % 15) == 0 && state->route_progress < 94) {
        state->route_progress += 1;
    }
}
