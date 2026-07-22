#include "ui_app.h"

#include <stdint.h>

#include "esp_timer.h"
#include "lvgl.h"

#include "ui_model.h"

#define SCREEN_SIZE 466
#define PAGE_COUNT 4

#define COLOR_BG       0x050608
#define COLOR_PANEL    0x101318
#define COLOR_LINE     0x252A33
#define COLOR_TEXT     0xF4F7F9
#define COLOR_MUTED    0x747C88
#define COLOR_CYAN     0x00E5FF
#define COLOR_LIME     0xB8FF3B
#define COLOR_ORANGE   0xFF8B3D
#define COLOR_PINK     0xFF4D8D

typedef enum {
    PAGE_NAV = 0,
    PAGE_MAP,
    PAGE_STATS,
    PAGE_BADGE,
} page_id_t;

static lv_obj_t *s_screens[PAGE_COUNT];
static lv_obj_t *s_debug_labels[PAGE_COUNT];
static lv_obj_t *s_status_labels[PAGE_COUNT];

static lv_obj_t *s_nav_progress;
static lv_obj_t *s_nav_distance;
static lv_obj_t *s_nav_speed;
static lv_obj_t *s_nav_eta;
static lv_obj_t *s_nav_halo;

static lv_obj_t *s_map_rider;
static lv_obj_t *s_map_distance;

static lv_obj_t *s_stats_speed;
static lv_obj_t *s_stats_trip;
static lv_obj_t *s_stats_time;
static lv_obj_t *s_stats_arc;

static lv_obj_t *s_badge_orbit_dot;
static lv_obj_t *s_badge_trip;

static page_id_t s_current_page;
static uint32_t s_last_switch_ms;
static uint32_t s_ui_tick_count;
static int64_t s_ui_tick_window_us;
static uint16_t s_motion_phase;
static ui_ride_state_t s_ride;

static const lv_point_precise_t s_turn_path[] = {
    {20, 145}, {95, 145}, {95, 70}, {184, 70},
};
static const lv_point_precise_t s_turn_head[] = {
    {148, 34}, {184, 70}, {148, 106},
};

static const lv_point_precise_t s_street_a[] = {
    {20, 150}, {120, 122}, {228, 158}, {440, 118},
};
static const lv_point_precise_t s_street_b[] = {
    {34, 286}, {146, 230}, {266, 270}, {432, 238},
};
static const lv_point_precise_t s_street_c[] = {
    {115, 52}, {128, 160}, {96, 284}, {120, 420},
};
static const lv_point_precise_t s_street_d[] = {
    {330, 48}, {300, 158}, {332, 278}, {300, 422},
};
static const lv_point_precise_t s_route[] = {
    {55, 365}, {100, 340}, {155, 330}, {180, 270},
    {250, 245}, {290, 180}, {380, 140},
};

static const int16_t s_orbit_x[32] = {
    150, 147, 139, 125, 106, 83, 57, 29,
    0, -29, -57, -83, -106, -125, -139, -147,
    -150, -147, -139, -125, -106, -83, -57, -29,
    0, 29, 57, 83, 106, 125, 139, 147,
};
static const int16_t s_orbit_y[32] = {
    0, 29, 57, 83, 106, 125, 139, 147,
    150, 147, 139, 125, 106, 83, 57, 29,
    0, -29, -57, -83, -106, -125, -139, -147,
    -150, -147, -139, -125, -106, -83, -57, -29,
};

static lv_obj_t *make_label(lv_obj_t *parent, const char *text,
                            const lv_font_t *font, uint32_t color)
{
    lv_obj_t *label = lv_label_create(parent);
    lv_label_set_text(label, text);
    lv_obj_set_style_text_font(label, font, 0);
    lv_obj_set_style_text_color(label, lv_color_hex(color), 0);
    return label;
}

static lv_obj_t *make_dot(lv_obj_t *parent, int32_t size, uint32_t color)
{
    lv_obj_t *dot = lv_obj_create(parent);
    lv_obj_set_size(dot, size, size);
    lv_obj_set_style_radius(dot, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(dot, lv_color_hex(color), 0);
    lv_obj_set_style_bg_opa(dot, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(dot, 0, 0);
    lv_obj_set_style_pad_all(dot, 0, 0);
    lv_obj_remove_flag(dot, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_remove_flag(dot, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_flag(dot, LV_OBJ_FLAG_GESTURE_BUBBLE);
    return dot;
}

static lv_obj_t *make_line(lv_obj_t *parent, const lv_point_precise_t *points,
                           uint32_t count, uint32_t color, int32_t width)
{
    lv_obj_t *line = lv_line_create(parent);
    lv_line_set_points(line, points, count);
    lv_obj_set_style_line_color(line, lv_color_hex(color), 0);
    lv_obj_set_style_line_width(line, width, 0);
    lv_obj_set_style_line_rounded(line, true, 0);
    lv_obj_remove_flag(line, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_flag(line, LV_OBJ_FLAG_GESTURE_BUBBLE);
    return line;
}

static void switch_page(int delta)
{
    uint32_t now = lv_tick_get();
    if ((uint32_t)(now - s_last_switch_ms) < 320) {
        return;
    }

    int next = (int)s_current_page + delta;
    if (next < 0) {
        next = PAGE_COUNT - 1;
    } else if (next >= PAGE_COUNT) {
        next = 0;
    }

    lv_screen_load_anim(
        s_screens[next],
        delta > 0 ? LV_SCREEN_LOAD_ANIM_MOVE_LEFT : LV_SCREEN_LOAD_ANIM_MOVE_RIGHT,
        260,
        0,
        false);
    s_current_page = (page_id_t)next;
    s_last_switch_ms = now;
}

static void screen_event_cb(lv_event_t *event)
{
    lv_event_code_t code = lv_event_get_code(event);
    lv_indev_t *indev = lv_indev_active();
    if (indev == NULL) {
        return;
    }

    if (code == LV_EVENT_GESTURE) {
        lv_dir_t direction = lv_indev_get_gesture_dir(indev);
        if (direction == LV_DIR_LEFT) {
            switch_page(1);
        } else if (direction == LV_DIR_RIGHT) {
            switch_page(-1);
        }
        lv_indev_wait_release(indev);
        return;
    }

    if (code == LV_EVENT_CLICKED) {
        lv_point_t point;
        lv_indev_get_point(indev, &point);
        if (point.x < 115) {
            switch_page(-1);
        } else if (point.x > SCREEN_SIZE - 115) {
            switch_page(1);
        }
    }
}

static lv_obj_t *make_screen(page_id_t page, const char *title, uint32_t accent)
{
    lv_obj_t *screen = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(screen, lv_color_hex(COLOR_BG), 0);
    lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(screen, 0, 0);
    lv_obj_set_style_pad_all(screen, 0, 0);
    lv_obj_remove_flag(screen, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(screen, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(screen, screen_event_cb, LV_EVENT_GESTURE, NULL);
    lv_obj_add_event_cb(screen, screen_event_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *title_label = make_label(screen, title, &lv_font_montserrat_14, COLOR_MUTED);
    lv_obj_set_style_text_letter_space(title_label, 2, 0);
    lv_obj_align(title_label, LV_ALIGN_TOP_MID, 0, 28);

    lv_obj_t *gps_dot = make_dot(screen, 7, COLOR_LIME);
    lv_obj_set_pos(gps_dot, 76, 34);

    lv_obj_t *phone_dot = make_dot(screen, 7, COLOR_CYAN);
    lv_obj_set_pos(phone_dot, 383, 34);

    s_status_labels[page] = make_label(screen, "82%", &lv_font_montserrat_14, COLOR_MUTED);
    lv_obj_align(s_status_labels[page], LV_ALIGN_TOP_RIGHT, -54, 27);

    int first_x = (SCREEN_SIZE - ((PAGE_COUNT - 1) * 16 + 7)) / 2;
    for (int i = 0; i < PAGE_COUNT; ++i) {
        lv_obj_t *dot = make_dot(screen, 7, i == page ? accent : COLOR_LINE);
        lv_obj_set_pos(dot, first_x + i * 16, 434);
    }

    s_debug_labels[page] = make_label(screen, "UI -- Hz", &lv_font_montserrat_14, COLOR_MUTED);
    lv_obj_set_style_text_opa(s_debug_labels[page], LV_OPA_70, 0);
    lv_obj_align(s_debug_labels[page], LV_ALIGN_BOTTOM_MID, 0, -42);

    s_screens[page] = screen;
    return screen;
}

static void set_object_opacity(void *object, int32_t value)
{
    lv_obj_set_style_opa((lv_obj_t *)object, (lv_opa_t)value, 0);
}

static void start_pulse(lv_obj_t *object)
{
    lv_anim_t animation;
    lv_anim_init(&animation);
    lv_anim_set_var(&animation, object);
    lv_anim_set_exec_cb(&animation, set_object_opacity);
    lv_anim_set_values(&animation, LV_OPA_30, LV_OPA_90);
    lv_anim_set_duration(&animation, 700);
    lv_anim_set_reverse_duration(&animation, 700);
    lv_anim_set_repeat_count(&animation, LV_ANIM_REPEAT_INFINITE);
    lv_anim_start(&animation);
}

static void build_navigation_screen(void)
{
    lv_obj_t *screen = make_screen(PAGE_NAV, "NAVIGATION", COLOR_CYAN);

    s_nav_progress = lv_arc_create(screen);
    lv_obj_set_size(s_nav_progress, 410, 410);
    lv_obj_center(s_nav_progress);
    lv_arc_set_range(s_nav_progress, 0, 100);
    lv_arc_set_value(s_nav_progress, s_ride.route_progress);
    lv_arc_set_rotation(s_nav_progress, 230);
    lv_arc_set_bg_angles(s_nav_progress, 0, 260);
    lv_obj_set_style_arc_width(s_nav_progress, 4, LV_PART_MAIN);
    lv_obj_set_style_arc_width(s_nav_progress, 4, LV_PART_INDICATOR);
    lv_obj_set_style_arc_color(s_nav_progress, lv_color_hex(COLOR_LINE), LV_PART_MAIN);
    lv_obj_set_style_arc_color(s_nav_progress, lv_color_hex(COLOR_CYAN), LV_PART_INDICATOR);
    lv_obj_set_style_opa(s_nav_progress, LV_OPA_TRANSP, LV_PART_KNOB);
    lv_obj_remove_flag(s_nav_progress, LV_OBJ_FLAG_CLICKABLE);

    lv_obj_t *turn_label = make_label(screen, "TURN RIGHT", &lv_font_montserrat_18, COLOR_CYAN);
    lv_obj_set_style_text_letter_space(turn_label, 3, 0);
    lv_obj_align(turn_label, LV_ALIGN_TOP_MID, 0, 66);

    s_nav_halo = lv_obj_create(screen);
    lv_obj_set_size(s_nav_halo, 188, 188);
    lv_obj_align(s_nav_halo, LV_ALIGN_TOP_MID, 0, 84);
    lv_obj_set_style_radius(s_nav_halo, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_opa(s_nav_halo, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(s_nav_halo, 2, 0);
    lv_obj_set_style_border_color(s_nav_halo, lv_color_hex(COLOR_CYAN), 0);
    lv_obj_remove_flag(s_nav_halo, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_remove_flag(s_nav_halo, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_flag(s_nav_halo, LV_OBJ_FLAG_GESTURE_BUBBLE);

    lv_obj_t *arrow_box = lv_obj_create(screen);
    lv_obj_set_size(arrow_box, 220, 180);
    lv_obj_align(arrow_box, LV_ALIGN_TOP_MID, 0, 91);
    lv_obj_set_style_bg_opa(arrow_box, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(arrow_box, 0, 0);
    lv_obj_set_style_pad_all(arrow_box, 0, 0);
    lv_obj_remove_flag(arrow_box, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_remove_flag(arrow_box, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_flag(arrow_box, LV_OBJ_FLAG_GESTURE_BUBBLE);
    make_line(arrow_box, s_turn_path, 4, COLOR_TEXT, 22);
    make_line(arrow_box, s_turn_head, 3, COLOR_TEXT, 22);

    s_nav_distance = make_label(screen, "180 m", &lv_font_montserrat_48, COLOR_TEXT);
    lv_obj_align(s_nav_distance, LV_ALIGN_TOP_MID, 0, 278);

    lv_obj_t *road = make_label(screen, "RIVER STREET", &lv_font_montserrat_14, COLOR_MUTED);
    lv_obj_set_style_text_letter_space(road, 2, 0);
    lv_obj_align(road, LV_ALIGN_TOP_MID, 0, 337);

    s_nav_speed = make_label(screen, "24.8", &lv_font_montserrat_24, COLOR_TEXT);
    lv_obj_align(s_nav_speed, LV_ALIGN_BOTTOM_LEFT, 100, -75);
    lv_obj_t *speed_unit = make_label(screen, "KM/H", &lv_font_montserrat_14, COLOR_MUTED);
    lv_obj_align(speed_unit, LV_ALIGN_BOTTOM_LEFT, 105, -57);

    s_nav_eta = make_label(screen, "12:48", &lv_font_montserrat_24, COLOR_TEXT);
    lv_obj_align(s_nav_eta, LV_ALIGN_BOTTOM_RIGHT, -92, -75);
    lv_obj_t *eta_unit = make_label(screen, "ETA", &lv_font_montserrat_14, COLOR_MUTED);
    lv_obj_align(eta_unit, LV_ALIGN_BOTTOM_RIGHT, -109, -57);

    start_pulse(s_nav_halo);
}

static void build_map_screen(void)
{
    lv_obj_t *screen = make_screen(PAGE_MAP, "ROUTE", COLOR_LIME);

    make_line(screen, s_street_a, 4, COLOR_LINE, 3);
    make_line(screen, s_street_b, 4, COLOR_LINE, 3);
    make_line(screen, s_street_c, 4, COLOR_LINE, 3);
    make_line(screen, s_street_d, 4, COLOR_LINE, 3);
    make_line(screen, s_route, 7, COLOR_CYAN, 13);

    lv_obj_t *route_start = make_dot(screen, 13, COLOR_MUTED);
    lv_obj_set_pos(route_start, s_route[0].x - 6, s_route[0].y - 6);
    lv_obj_t *route_end = make_dot(screen, 16, COLOR_LIME);
    lv_obj_set_pos(route_end, s_route[6].x - 8, s_route[6].y - 8);

    s_map_rider = make_dot(screen, 22, COLOR_TEXT);
    lv_obj_set_style_border_width(s_map_rider, 5, 0);
    lv_obj_set_style_border_color(s_map_rider, lv_color_hex(COLOR_CYAN), 0);
    lv_obj_set_pos(s_map_rider, s_route[2].x - 11, s_route[2].y - 11);

    lv_obj_t *card = lv_obj_create(screen);
    lv_obj_set_size(card, 292, 78);
    lv_obj_align(card, LV_ALIGN_BOTTOM_MID, 0, -57);
    lv_obj_set_style_radius(card, 24, 0);
    lv_obj_set_style_bg_color(card, lv_color_hex(COLOR_PANEL), 0);
    lv_obj_set_style_bg_opa(card, LV_OPA_90, 0);
    lv_obj_set_style_border_width(card, 1, 0);
    lv_obj_set_style_border_color(card, lv_color_hex(COLOR_LINE), 0);
    lv_obj_remove_flag(card, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_remove_flag(card, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_flag(card, LV_OBJ_FLAG_GESTURE_BUBBLE);

    lv_obj_t *next = make_label(card, "NEXT  >", &lv_font_montserrat_18, COLOR_CYAN);
    lv_obj_align(next, LV_ALIGN_LEFT_MID, 20, -13);
    s_map_distance = make_label(card, "180 m", &lv_font_montserrat_24, COLOR_TEXT);
    lv_obj_align(s_map_distance, LV_ALIGN_LEFT_MID, 20, 16);
    lv_obj_t *road = make_label(card, "RIVER ST", &lv_font_montserrat_14, COLOR_MUTED);
    lv_obj_align(road, LV_ALIGN_RIGHT_MID, -20, 15);
}

static lv_obj_t *make_metric_card(lv_obj_t *parent, int32_t x, int32_t y,
                                  const char *title, lv_obj_t **value_label)
{
    lv_obj_t *card = lv_obj_create(parent);
    lv_obj_set_size(card, 142, 76);
    lv_obj_set_pos(card, x, y);
    lv_obj_set_style_radius(card, 22, 0);
    lv_obj_set_style_bg_color(card, lv_color_hex(COLOR_PANEL), 0);
    lv_obj_set_style_bg_opa(card, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(card, 1, 0);
    lv_obj_set_style_border_color(card, lv_color_hex(COLOR_LINE), 0);
    lv_obj_remove_flag(card, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_remove_flag(card, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_flag(card, LV_OBJ_FLAG_GESTURE_BUBBLE);

    lv_obj_t *title_label = make_label(card, title, &lv_font_montserrat_14, COLOR_MUTED);
    lv_obj_align(title_label, LV_ALIGN_TOP_LEFT, 15, 11);
    *value_label = make_label(card, "--", &lv_font_montserrat_24, COLOR_TEXT);
    lv_obj_align(*value_label, LV_ALIGN_BOTTOM_LEFT, 15, -10);
    return card;
}

static void build_stats_screen(void)
{
    lv_obj_t *screen = make_screen(PAGE_STATS, "RIDE", COLOR_ORANGE);

    s_stats_arc = lv_arc_create(screen);
    lv_obj_set_size(s_stats_arc, 322, 322);
    lv_obj_align(s_stats_arc, LV_ALIGN_TOP_MID, 0, 54);
    lv_arc_set_range(s_stats_arc, 0, 450);
    lv_arc_set_value(s_stats_arc, s_ride.speed_x10);
    lv_arc_set_rotation(s_stats_arc, 135);
    lv_arc_set_bg_angles(s_stats_arc, 0, 270);
    lv_obj_set_style_arc_width(s_stats_arc, 10, LV_PART_MAIN);
    lv_obj_set_style_arc_width(s_stats_arc, 10, LV_PART_INDICATOR);
    lv_obj_set_style_arc_color(s_stats_arc, lv_color_hex(COLOR_LINE), LV_PART_MAIN);
    lv_obj_set_style_arc_color(s_stats_arc, lv_color_hex(COLOR_ORANGE), LV_PART_INDICATOR);
    lv_obj_set_style_opa(s_stats_arc, LV_OPA_TRANSP, LV_PART_KNOB);
    lv_obj_remove_flag(s_stats_arc, LV_OBJ_FLAG_CLICKABLE);

    s_stats_speed = make_label(screen, "24.8", &lv_font_montserrat_48, COLOR_TEXT);
    lv_obj_align(s_stats_speed, LV_ALIGN_TOP_MID, 0, 145);
    lv_obj_t *unit = make_label(screen, "KM/H", &lv_font_montserrat_14, COLOR_MUTED);
    lv_obj_set_style_text_letter_space(unit, 2, 0);
    lv_obj_align(unit, LV_ALIGN_TOP_MID, 0, 203);

    make_metric_card(screen, 78, 300, "DISTANCE", &s_stats_trip);
    make_metric_card(screen, 246, 300, "ELAPSED", &s_stats_time);
}

static void build_badge_screen(void)
{
    lv_obj_t *screen = make_screen(PAGE_BADGE, "BADGE", COLOR_PINK);

    lv_obj_t *outer = lv_arc_create(screen);
    lv_obj_set_size(outer, 360, 360);
    lv_obj_center(outer);
    lv_arc_set_range(outer, 0, 100);
    lv_arc_set_value(outer, 74);
    lv_arc_set_rotation(outer, 220);
    lv_arc_set_bg_angles(outer, 0, 280);
    lv_obj_set_style_arc_width(outer, 7, LV_PART_MAIN);
    lv_obj_set_style_arc_width(outer, 7, LV_PART_INDICATOR);
    lv_obj_set_style_arc_color(outer, lv_color_hex(COLOR_LINE), LV_PART_MAIN);
    lv_obj_set_style_arc_color(outer, lv_color_hex(COLOR_PINK), LV_PART_INDICATOR);
    lv_obj_set_style_opa(outer, LV_OPA_TRANSP, LV_PART_KNOB);
    lv_obj_remove_flag(outer, LV_OBJ_FLAG_CLICKABLE);

    lv_obj_t *inner = lv_obj_create(screen);
    lv_obj_set_size(inner, 246, 246);
    lv_obj_center(inner);
    lv_obj_set_style_radius(inner, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(inner, lv_color_hex(COLOR_PANEL), 0);
    lv_obj_set_style_bg_opa(inner, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(inner, 1, 0);
    lv_obj_set_style_border_color(inner, lv_color_hex(COLOR_PINK), 0);
    lv_obj_remove_flag(inner, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_remove_flag(inner, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_flag(inner, LV_OBJ_FLAG_GESTURE_BUBBLE);

    lv_obj_t *ride = make_label(inner, "RIDE", &lv_font_montserrat_18, COLOR_PINK);
    lv_obj_set_style_text_letter_space(ride, 5, 0);
    lv_obj_align(ride, LV_ALIGN_TOP_MID, 0, 47);
    lv_obj_t *number = make_label(inner, "042", &lv_font_montserrat_48, COLOR_TEXT);
    lv_obj_align(number, LV_ALIGN_CENTER, 0, -3);
    s_badge_trip = make_label(inner, "12.40 KM", &lv_font_montserrat_18, COLOR_MUTED);
    lv_obj_align(s_badge_trip, LV_ALIGN_BOTTOM_MID, 0, -49);

    s_badge_orbit_dot = make_dot(screen, 13, COLOR_LIME);
    lv_obj_set_pos(s_badge_orbit_dot, 377, 226);
}

static void update_model_widgets(void)
{
    uint32_t speed_whole = s_ride.speed_x10 / 10;
    uint32_t speed_decimal = s_ride.speed_x10 % 10;
    uint32_t trip_km = s_ride.trip_distance_m / 1000;
    uint32_t trip_hundredths = (s_ride.trip_distance_m % 1000) / 10;
    uint32_t elapsed_minutes = s_ride.elapsed_s / 60;
    uint32_t elapsed_seconds = s_ride.elapsed_s % 60;

    lv_label_set_text_fmt(s_nav_distance, "%u m", s_ride.turn_distance_m);
    lv_label_set_text_fmt(s_map_distance, "%u m", s_ride.turn_distance_m);
    lv_label_set_text_fmt(s_nav_speed, "%lu.%lu",
                          (unsigned long)speed_whole,
                          (unsigned long)speed_decimal);
    lv_label_set_text_fmt(s_stats_speed, "%lu.%lu",
                          (unsigned long)speed_whole,
                          (unsigned long)speed_decimal);
    lv_label_set_text_fmt(s_stats_trip, "%lu.%02lu km",
                          (unsigned long)trip_km,
                          (unsigned long)trip_hundredths);
    lv_label_set_text_fmt(s_badge_trip, "%lu.%02lu KM",
                          (unsigned long)trip_km,
                          (unsigned long)trip_hundredths);
    lv_label_set_text_fmt(s_stats_time, "%lu:%02lu",
                          (unsigned long)elapsed_minutes,
                          (unsigned long)elapsed_seconds);
    lv_label_set_text_fmt(s_nav_eta, "12:%02u", 48 - (s_ride.route_progress % 12));

    lv_arc_set_value(s_nav_progress, s_ride.route_progress);
    lv_arc_set_value(s_stats_arc, s_ride.speed_x10);

    for (int i = 0; i < PAGE_COUNT; ++i) {
        lv_label_set_text_fmt(s_status_labels[i], "%u%%", s_ride.battery_percent);
    }
}

static void model_timer_cb(lv_timer_t *timer)
{
    (void)timer;
    ui_model_step(&s_ride);
    update_model_widgets();
}

static void update_map_rider(void)
{
    uint16_t route_phase = s_motion_phase % 120;
    uint16_t segment = route_phase / 20;
    uint16_t within = route_phase % 20;
    if (segment >= 6) {
        segment = 5;
        within = 20;
    }

    int32_t x = s_route[segment].x +
                ((s_route[segment + 1].x - s_route[segment].x) * within) / 20;
    int32_t y = s_route[segment].y +
                ((s_route[segment + 1].y - s_route[segment].y) * within) / 20;
    lv_obj_set_pos(s_map_rider, x - 11, y - 11);
}

static void animation_timer_cb(lv_timer_t *timer)
{
    (void)timer;
    ++s_motion_phase;
    ++s_ui_tick_count;

    if (s_current_page == PAGE_MAP) {
        update_map_rider();
    } else if (s_current_page == PAGE_BADGE) {
        uint16_t orbit = (s_motion_phase / 2) % 32;
        lv_obj_set_pos(s_badge_orbit_dot,
                       SCREEN_SIZE / 2 + s_orbit_x[orbit] - 6,
                       SCREEN_SIZE / 2 + s_orbit_y[orbit] - 6);
    }

    int64_t now = esp_timer_get_time();
    if (now - s_ui_tick_window_us >= 1000000) {
        for (int i = 0; i < PAGE_COUNT; ++i) {
            lv_label_set_text_fmt(s_debug_labels[i], "UI %lu Hz",
                                  (unsigned long)s_ui_tick_count);
        }
        s_ui_tick_count = 0;
        s_ui_tick_window_us = now;
    }
}

void ui_app_start(void)
{
    ui_model_reset(&s_ride);
    s_current_page = PAGE_NAV;
    s_last_switch_ms = 0;
    s_ui_tick_count = 0;
    s_ui_tick_window_us = esp_timer_get_time();
    s_motion_phase = 40;

    build_navigation_screen();
    build_map_screen();
    build_stats_screen();
    build_badge_screen();
    update_model_widgets();

    lv_screen_load(s_screens[PAGE_NAV]);
    lv_timer_create(model_timer_cb, 200, NULL);
    lv_timer_create(animation_timer_cb, 33, NULL);
}
