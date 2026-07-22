#include "esp_check.h"
#include "esp_log.h"
#include "bsp/esp-bsp.h"

#include "ui/ui_app.h"

static const char *TAG = "bike_badge";

void app_main(void)
{
    ESP_LOGI(TAG, "Starting Waveshare display and LVGL");

    lv_display_t *display = bsp_display_start();
    ESP_ERROR_CHECK(display == NULL ? ESP_FAIL : ESP_OK);

    ESP_ERROR_CHECK(bsp_display_brightness_set(65));
    ESP_ERROR_CHECK(bsp_display_lock(0));
    ui_app_start();
    bsp_display_unlock();

    ESP_LOGI(TAG, "UI prototype is running");
}
