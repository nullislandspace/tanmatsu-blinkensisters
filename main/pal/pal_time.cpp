#include "pal_time.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

Uint32 SDL_GetTicks(void) {
    return (Uint32)(esp_timer_get_time() / 1000ULL);
}

void SDL_Delay(Uint32 ms) {
    vTaskDelay(pdMS_TO_TICKS(ms));
}
