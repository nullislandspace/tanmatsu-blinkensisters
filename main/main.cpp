// BlinkenSisters - Tanmatsu port
// Main entry point

extern "C" {
#include "bsp/device.h"
#include "bsp/display.h"
#include "bsp/input.h"
#include "bsp/audio.h"
#include "bsp/led.h"
#include "driver/gpio.h"
#include "driver/i2s_std.h"
#include "esp_log.h"
#include "esp_heap_caps.h"
#include "nvs_flash.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "sdcard.h"

// bsp_audio_initialize is not in public bsp/audio.h — use extern declaration
extern esp_err_t bsp_audio_initialize(uint32_t rate);
}

#include "pal/pal_screen.h"
#include "pal/pal_input.h"
#include "pal/pal_font.h"
#include "pal/pal_audio.h"
#include "pal/pal_net.h"
#include "shared/config.h"
#include "shared/errorhandler.h"
#include "shared/showloading.h"
#include "shared/extractmetabmf.h"
#include "game/menu.h"
#include "game/sound.h"
#include "game/joystick.h"
#include "game/gameengine.h"
#include "game/bsgui.h"
#include "game/fginlay.h"
#include "game/engine.h"
#include "game/addonstore.h"

static const char* TAG = "main";

// Globals defined here (others are defined in engine.cpp, sound.cpp)
bool isFullscreen = true;
bool enableColor3D = false;
Uint32 gLastTick = 0;
bool performanceTestMode = false;

static void game_task(void* arg) {
    ESP_LOGI(TAG, "game_task started, free heap: %lu IRAM: %lu PSRAM: %lu",
             (unsigned long)esp_get_free_heap_size(),
             (unsigned long)heap_caps_get_free_size(MALLOC_CAP_INTERNAL),
             (unsigned long)heap_caps_get_free_size(MALLOC_CAP_SPIRAM));

    QueueHandle_t* q = (QueueHandle_t*)arg;
    QueueHandle_t input_queue = *q;

    // Initialize PAL subsystems
    BS_InitScreen();
    initFontHandler();
    PAL_InputInit(input_queue);
    initSound();
    initJoystick();

    // Clear display
    SDL_FillRect(gScreen, NULL, 0xff000000);
    BS_Flip(gScreen);

    // First-run: extract game data from SD card
    displayGraphicalErrors = false;

    initShowLoading("loading.jpg");
    showLoading();
    initExtractMetaBMF();
    initFGInlay();
    initGui();

    configInit(false);
    configStartupComplete();

    // The app ships without game data; on a first start it is downloaded
    // here, before anything below tries to load it.
    if (!addonStoreEnsureBaseData()) {
        quitToLauncher();
    }
    deInitShowLoading();
    initShowLoading("loading.jpg");

    // The built-in sound effects live in the extracted game data, so they can
    // only be loaded now that configInit() has unpacked it.
    loadSoundFX();

    displayGraphicalErrors = true;
    initMenu();
    addonStoreOfferLostPixels();

    // Open on the title screen, as the original did: attract mode starts with
    // the intro and then runs the demo levels, and any key leaves it for the
    // menu. It needs Lost Pixels and quietly does nothing without it.
    menuAttrackMode();

    // Main menu loop
    while (menuDisplay()) {
        // loop returns false to quit
    }

    // Cleanup
    ESP_LOGI(TAG, "Quit: deInitMenu");
    deInitMenu();
    ESP_LOGI(TAG, "Quit: deInitShowLoading");
    deInitShowLoading();
    ESP_LOGI(TAG, "Quit: deInitFGInlay");
    deInitFGInlay();
    ESP_LOGI(TAG, "Quit: deInitGui");
    deInitGui();
    ESP_LOGI(TAG, "Quit: deInitSound");
    deInitSound();

    ESP_LOGI(TAG, "Game exited, returning to launcher");
    bsp_device_restart_to_launcher();
}

// For debugging startup: launching from the launcher resets the USB serial
// connection, and `make monitor` needs a while to reconnect. Uncomment to hold
// off long enough for the whole startup to reach the log.
//#define STARTUP_LOG_DELAY_MS 10000

extern "C" void app_main(void) {
#ifdef STARTUP_LOG_DELAY_MS
    vTaskDelay(pdMS_TO_TICKS(STARTUP_LOG_DELAY_MS));
#endif
    gpio_install_isr_service(0);

    // NVS
    // Never erase NVS here: it holds the WiFi networks saved in the launcher,
    // which the addon downloader connects with. If it cannot be opened, WiFi
    // simply reports that no network is set up.
    esp_err_t res = nvs_flash_init();
    if (res != ESP_OK) {
        ESP_LOGE(TAG, "NVS init failed: %s", esp_err_to_name(res));
    }

    // BSP
    const bsp_configuration_t bsp_config = {
        .display = {
            // RGB565: matches the surface format, and halves both the
            // rotation's output and the panel transfer.
            .requested_color_format = LCD_COLOR_PIXEL_FORMAT_RGB565,
            .num_fbs = 1,
        },
    };
    res = bsp_device_initialize(&bsp_config);
    if (res != ESP_OK) {
        ESP_LOGE(TAG, "BSP init failed: %d", res);
        return;
    }

    // Radio and WiFi stack before the SD card: the radio talks SDIO on the
    // second SDMMC slot, and mounting the card first disturbs its bring-up
    // (the launcher and tanmatsu-discord do it in this order too).
    PAL_NetBootInit();
    vTaskDelay(pdMS_TO_TICKS(500));

    // SD card (must be before any file access)
    res = sdcard_init();
    if (res != ESP_OK) {
        ESP_LOGE(TAG, "SD card init failed: %s", esp_err_to_name(res));
        // Continue anyway — game will log errors when it tries to open files
    }

    // Audio: initialize codec + I2S at 44100 Hz (see tanmatsu-tadoom for sequence)
    // bsp_audio_set_rate() calls i2s_channel_reconfig_std_clock() which requires
    // the channel to be disabled first.
    {
        i2s_chan_handle_t i2s_handle = NULL;
        bsp_audio_initialize(44100);
        bsp_audio_get_i2s_handle(&i2s_handle);
        if (i2s_handle) {
            i2s_channel_disable(i2s_handle);
            bsp_audio_set_rate(44100);
            i2s_channel_enable(i2s_handle);
        }
        bsp_audio_set_amplifier(true);
        bsp_audio_set_volume(100.0);

    }

    // Input queue
    static QueueHandle_t input_queue = NULL;
    ESP_ERROR_CHECK(bsp_input_get_queue(&input_queue));

    // Launch game task with large stack in PSRAM (internal SRAM too small for 64KB)
    StaticTask_t* task_buf = (StaticTask_t*)heap_caps_malloc(sizeof(StaticTask_t), MALLOC_CAP_INTERNAL);
    StackType_t*  task_stack = (StackType_t*)heap_caps_malloc(65536, MALLOC_CAP_SPIRAM);
    if (!task_buf || !task_stack) {
        ESP_LOGE(TAG, "Failed to allocate game task memory");
        return;
    }
    xTaskCreateStaticPinnedToCore(game_task, "game", 65536, &input_queue, 5,
                                  task_stack, task_buf, 0);

    // Main task can exit
}
