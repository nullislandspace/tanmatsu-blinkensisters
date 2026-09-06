// =====================================================================
//  BlinkenSisters -- pal_ppa.cpp   (see pal_ppa.h)
//  ESP32-P4 PPA offload: client lifecycle, ordered job queue + pump task,
//  cache maintenance helpers.
// =====================================================================

#include "pal_ppa.h"

extern "C" {
#include "driver/ppa.h"
#include "esp_cache.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
}

static const char* TAG = "pal_ppa";

// Depth of the submit/done queues: the most jobs that may be in flight
// un-drained at once. A frame enqueues a handful, so 16 is ample.
#define PPA_QUEUE_DEPTH      16
#define PPA_CLIENT_DEPTH     1     // the pump runs one op at a time
#define PPA_WAIT_TIMEOUT_MS  50    // PPA ops are sub-millisecond
#define PPA_PUMP_STACK       3072
#define PPA_PUMP_PRIO        6     // above the game task, below audio
#define PPA_PUMP_CORE        1     // off the game task's core

// PSRAM L2 cache line; PPA output buffers must be aligned to it.
#define PPA_CACHE_LINE       128

static ppa_client_handle_t s_srm_client  = NULL;
static ppa_client_handle_t s_fill_client = NULL;
static QueueHandle_t       s_submit_q    = NULL;  // game -> pump
static QueueHandle_t       s_done_q      = NULL;  // pump -> game
static SemaphoreHandle_t   s_op_done_sem = NULL;  // ISR  -> pump
static TaskHandle_t        s_pump_task   = NULL;
static int                 s_inflight    = 0;     // producer task only
static bool                s_inited      = false;

typedef enum { PPA_JOB_SRM, PPA_JOB_FILL } ppa_job_type_t;

// A queued job: a caller id plus the fully built driver config. The config
// is built and bounds-checked in caller context (so a bad rect is refused
// immediately) and copied by value into the queue. The buffers it points at
// outlive the frame.
typedef struct {
    uint32_t       id;
    ppa_job_type_t type;
    union {
        ppa_srm_oper_config_t  srm;
        ppa_fill_oper_config_t fill;
    } cfg;
} ppa_job_t;

// Completion ISR, shared by both clients: wake the pump, which is blocked
// on s_op_done_sem after submitting the in-flight op. Give only -- the next
// submit happens back in the pump's task context.
static bool ppa_on_trans_done(ppa_client_handle_t client,
                              ppa_event_data_t* event_data, void* user_data) {
    (void)client;
    (void)event_data;
    (void)user_data;
    BaseType_t hpw = pdFALSE;
    xSemaphoreGiveFromISR(s_op_done_sem, &hpw);
    return hpw == pdTRUE;
}

static bool ppa_enqueue(const ppa_job_t* job) {
    if (xQueueSend(s_submit_q, job, 0) != pdTRUE) {
        ESP_LOGW(TAG, "submit queue full (depth %d); job %u refused",
                 (int)PPA_QUEUE_DEPTH, (unsigned)job->id);
        return false;
    }
    s_inflight++;
    return true;
}

// The pump: submit one op, wait for it, record its id, repeat. Task context,
// so the driver's blocking submit calls are legal here. One op in flight
// means execution order == submission order.
static void ppa_pump_task(void* arg) {
    (void)arg;
    for (;;) {
        ppa_job_t job;
        if (xQueueReceive(s_submit_q, &job, portMAX_DELAY) != pdTRUE) {
            continue;
        }
        esp_err_t err = ESP_FAIL;
        switch (job.type) {
            case PPA_JOB_SRM:  err = ppa_do_scale_rotate_mirror(s_srm_client, &job.cfg.srm); break;
            case PPA_JOB_FILL: err = ppa_do_fill(s_fill_client, &job.cfg.fill);              break;
        }
        if (err == ESP_OK) {
            if (xSemaphoreTake(s_op_done_sem, pdMS_TO_TICKS(PPA_WAIT_TIMEOUT_MS)) != pdTRUE) {
                ESP_LOGW(TAG, "pump: job %u completion timed out", (unsigned)job.id);
            }
        } else {
            ESP_LOGW(TAG, "pump: job %u (type %d) submit failed: %d",
                     (unsigned)job.id, (int)job.type, err);
        }
        // Record completion even on failure, so a waiter never hangs.
        if (xQueueSend(s_done_q, &job.id, pdMS_TO_TICKS(100)) != pdTRUE) {
            ESP_LOGE(TAG, "pump: done queue full; completion of job %u dropped",
                     (unsigned)job.id);
        }
    }
}

bool PAL_PPA_Init(void) {
    if (s_inited) {
        return true;
    }

    s_submit_q    = xQueueCreate(PPA_QUEUE_DEPTH, sizeof(ppa_job_t));
    s_done_q      = xQueueCreate(PPA_QUEUE_DEPTH, sizeof(uint32_t));
    s_op_done_sem = xSemaphoreCreateBinary();
    if (!s_submit_q || !s_done_q || !s_op_done_sem) {
        ESP_LOGE(TAG, "failed to create pump queues/semaphore");
        return false;
    }

    ppa_client_config_t srm_cfg = {};
    srm_cfg.oper_type             = PPA_OPERATION_SRM;
    srm_cfg.max_pending_trans_num = PPA_CLIENT_DEPTH;
    srm_cfg.data_burst_length     = PPA_DATA_BURST_LENGTH_128;

    ppa_client_config_t fill_cfg = {};
    fill_cfg.oper_type             = PPA_OPERATION_FILL;
    fill_cfg.max_pending_trans_num = PPA_CLIENT_DEPTH;
    fill_cfg.data_burst_length     = PPA_DATA_BURST_LENGTH_128;

    if (ppa_register_client(&srm_cfg,  &s_srm_client)  != ESP_OK ||
        ppa_register_client(&fill_cfg, &s_fill_client) != ESP_OK) {
        ESP_LOGE(TAG, "ppa_register_client failed");
        return false;
    }

    ppa_event_callbacks_t cbs = {};
    cbs.on_trans_done = ppa_on_trans_done;
    if (ppa_client_register_event_callbacks(s_srm_client,  &cbs) != ESP_OK ||
        ppa_client_register_event_callbacks(s_fill_client, &cbs) != ESP_OK) {
        ESP_LOGE(TAG, "ppa_client_register_event_callbacks failed");
        return false;
    }

    s_inflight = 0;
    if (xTaskCreatePinnedToCore(ppa_pump_task, "ppa_pump", PPA_PUMP_STACK, NULL,
                                PPA_PUMP_PRIO, &s_pump_task, PPA_PUMP_CORE) != pdPASS) {
        ESP_LOGE(TAG, "failed to create PPA pump task");
        return false;
    }

    s_inited = true;
    ESP_LOGI(TAG, "PPA compositor up");
    return true;
}

bool PAL_PPA_Available(void) {
    return s_inited;
}

// --- Cache maintenance -------------------------------------------------
// esp_cache_msync needs both address and length aligned to the cache line.
// Surface pixel buffers come from BS_CreateSurface, which aligns them for
// exactly this reason; round the length up defensively so an unaligned
// surface degrades to "no sync" (logged) rather than a silent wrong result.

static void surface_msync(const BS_Surface* s, int flags, const char* what) {
    if (!s || !s->pixels) {
        return;
    }
    size_t len = (size_t)s->w * (size_t)s->h * sizeof(Uint32);
    len = (len + PPA_CACHE_LINE - 1) & ~(size_t)(PPA_CACHE_LINE - 1);
    if (((uintptr_t)s->pixels & (PPA_CACHE_LINE - 1)) != 0) {
        ESP_LOGW(TAG, "%s: surface %dx%d not cache-line aligned, skipping",
                 what, (int)s->w, (int)s->h);
        return;
    }
    esp_err_t err = esp_cache_msync(s->pixels, len, flags);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "%s: esp_cache_msync failed: %d", what, err);
    }
}

void PAL_PPA_FlushSurface(const BS_Surface* s) {
    surface_msync(s, ESP_CACHE_MSYNC_FLAG_DIR_C2M | ESP_CACHE_MSYNC_FLAG_TYPE_DATA, "flush");
}

void PAL_PPA_InvalidateSurface(const BS_Surface* s) {
    surface_msync(s, ESP_CACHE_MSYNC_FLAG_DIR_M2C | ESP_CACHE_MSYNC_FLAG_TYPE_DATA, "invalidate");
}

// --- Submits -----------------------------------------------------------

// The driver wants the output buffer_size cache-line aligned. BS_CreateSurface
// rounds every surface allocation up to that boundary, so reporting the
// rounded size is both legal and accurate.
static uint32_t surface_buffer_size(const BS_Surface* s) {
    size_t len = (size_t)s->w * (size_t)s->h * sizeof(Uint32);
    return (uint32_t)((len + PPA_CACHE_LINE - 1) & ~(size_t)(PPA_CACHE_LINE - 1));
}

// Clip a rect against a surface, in place. Returns false if nothing is left.
static bool clip_rect(const BS_Surface* s, int* x, int* y, int* w, int* h) {
    if (*x < 0) { *w += *x; *x = 0; }
    if (*y < 0) { *h += *y; *y = 0; }
    if (*x + *w > s->w) { *w = s->w - *x; }
    if (*y + *h > s->h) { *h = s->h - *y; }
    return (*w > 0 && *h > 0);
}

bool PAL_PPA_Fill(BS_Surface* dst, uint32_t job_id,
                  int x, int y, int w, int h, Uint32 rgba) {
    if (!s_inited || !dst || !dst->pixels) {
        return false;
    }
    if (!clip_rect(dst, &x, &y, &w, &h)) {
        return false;
    }

    ppa_job_t job = {};
    job.id   = job_id;
    job.type = PPA_JOB_FILL;
    job.cfg.fill.out.buffer         = dst->pixels;
    job.cfg.fill.out.buffer_size    = surface_buffer_size(dst);
    job.cfg.fill.out.pic_w          = (uint32_t)dst->w;
    job.cfg.fill.out.pic_h          = (uint32_t)dst->h;
    job.cfg.fill.out.block_offset_x = (uint32_t)x;
    job.cfg.fill.out.block_offset_y = (uint32_t)y;
    job.cfg.fill.out.fill_cm        = PPA_FILL_COLOR_MODE_ARGB8888;
    job.cfg.fill.fill_block_w       = (uint32_t)w;
    job.cfg.fill.fill_block_h       = (uint32_t)h;
    // A BS pixel is 0xAABBGGRR, i.e. byte order R,G,B,A. The PPA lays an
    // ARGB8888 fill down as byte order B,G,R,A, taking those bytes from .b,
    // .g, .r, .a. So to land our channels in the right bytes, our red goes
    // in .b and our blue goes in .r -- the fields are named for the PPA's
    // layout, not ours.
    job.cfg.fill.fill_argb_color.a  = (rgba >> 24) & 0xFF;  // A -> byte 3
    job.cfg.fill.fill_argb_color.r  = (rgba >> 16) & 0xFF;  // our B -> byte 2
    job.cfg.fill.fill_argb_color.g  = (rgba >>  8) & 0xFF;  // G -> byte 1
    job.cfg.fill.fill_argb_color.b  = (rgba      ) & 0xFF;  // our R -> byte 0
    job.cfg.fill.mode               = PPA_TRANS_MODE_NON_BLOCKING;
    return ppa_enqueue(&job);
}

bool PAL_PPA_Blit(const BS_Surface* src, uint32_t job_id,
                  int sx, int sy, int w, int h,
                  BS_Surface* dst, int dx, int dy) {
    if (!s_inited || !src || !dst || !src->pixels || !dst->pixels) {
        return false;
    }
    // Clip against both surfaces, keeping the two rects the same size.
    if (sx < 0) { w += sx; dx -= sx; sx = 0; }
    if (sy < 0) { h += sy; dy -= sy; sy = 0; }
    if (dx < 0) { w += dx; sx -= dx; dx = 0; }
    if (dy < 0) { h += dy; sy -= dy; dy = 0; }
    if (sx + w > src->w) { w = src->w - sx; }
    if (sy + h > src->h) { h = src->h - sy; }
    if (dx + w > dst->w) { w = dst->w - dx; }
    if (dy + h > dst->h) { h = dst->h - dy; }
    if (w <= 0 || h <= 0) {
        return false;
    }

    ppa_job_t job = {};
    job.id   = job_id;
    job.type = PPA_JOB_SRM;
    job.cfg.srm.in.buffer          = src->pixels;
    job.cfg.srm.in.pic_w           = (uint32_t)src->w;
    job.cfg.srm.in.pic_h           = (uint32_t)src->h;
    job.cfg.srm.in.block_w         = (uint32_t)w;
    job.cfg.srm.in.block_h         = (uint32_t)h;
    job.cfg.srm.in.block_offset_x  = (uint32_t)sx;
    job.cfg.srm.in.block_offset_y  = (uint32_t)sy;
    job.cfg.srm.in.srm_cm          = PPA_SRM_COLOR_MODE_ARGB8888;
    job.cfg.srm.out.buffer         = dst->pixels;
    job.cfg.srm.out.buffer_size    = surface_buffer_size(dst);
    job.cfg.srm.out.pic_w          = (uint32_t)dst->w;
    job.cfg.srm.out.pic_h          = (uint32_t)dst->h;
    job.cfg.srm.out.block_offset_x = (uint32_t)dx;
    job.cfg.srm.out.block_offset_y = (uint32_t)dy;
    job.cfg.srm.out.srm_cm         = PPA_SRM_COLOR_MODE_ARGB8888;
    job.cfg.srm.rotation_angle     = PPA_SRM_ROTATION_ANGLE_0;
    job.cfg.srm.scale_x            = 1.0f;
    job.cfg.srm.scale_y            = 1.0f;
    // Source and destination share the same byte order, so no swap: the
    // channels are only mislabelled relative to the PPA, not moved.
    job.cfg.srm.rgb_swap           = false;
    job.cfg.srm.byte_swap          = false;
    job.cfg.srm.alpha_update_mode  = PPA_ALPHA_NO_CHANGE;
    job.cfg.srm.mode               = PPA_TRANS_MODE_NON_BLOCKING;
    return ppa_enqueue(&job);
}

bool PAL_PPA_FlipToPanel(const BS_Surface* screen, uint32_t job_id,
                         void* phys, size_t phys_size,
                         int phys_w, int phys_h, bool rgb_swap) {
    if (!s_inited || !screen || !screen->pixels || !phys) {
        return false;
    }
    // The panel is portrait and the game is landscape: logical (lx,ly) lands
    // at physical (col = phys_w-1-ly, row = lx), i.e. a 90-degree CLOCKWISE
    // turn. The PPA names rotations counter-clockwise, so that is ANGLE_270.
    if (screen->w != phys_h || screen->h != phys_w) {
        ESP_LOGW(TAG, "flip: %dx%d does not rotate into %dx%d",
                 (int)screen->w, (int)screen->h, phys_w, phys_h);
        return false;
    }

    ppa_job_t job = {};
    job.id   = job_id;
    job.type = PPA_JOB_SRM;
    job.cfg.srm.in.buffer          = screen->pixels;
    job.cfg.srm.in.pic_w           = (uint32_t)screen->w;
    job.cfg.srm.in.pic_h           = (uint32_t)screen->h;
    job.cfg.srm.in.block_w         = (uint32_t)screen->w;
    job.cfg.srm.in.block_h         = (uint32_t)screen->h;
    job.cfg.srm.in.block_offset_x  = 0;
    job.cfg.srm.in.block_offset_y  = 0;
    job.cfg.srm.in.srm_cm          = PPA_SRM_COLOR_MODE_ARGB8888;
    job.cfg.srm.out.buffer         = phys;
    job.cfg.srm.out.buffer_size    = (uint32_t)phys_size;
    job.cfg.srm.out.pic_w          = (uint32_t)phys_w;
    job.cfg.srm.out.pic_h          = (uint32_t)phys_h;
    job.cfg.srm.out.block_offset_x = 0;
    job.cfg.srm.out.block_offset_y = 0;
    job.cfg.srm.out.srm_cm         = PPA_SRM_COLOR_MODE_RGB888;
    job.cfg.srm.rotation_angle     = PPA_SRM_ROTATION_ANGLE_270;
    job.cfg.srm.scale_x            = 1.0f;
    job.cfg.srm.scale_y            = 1.0f;
    // The RGB888 the PPA writes is byte order B,G,R, which is what the panel
    // wants; `rgb_swap` decides how the input's channels reach it.
    job.cfg.srm.rgb_swap           = rgb_swap;
    job.cfg.srm.byte_swap          = false;
    job.cfg.srm.alpha_update_mode  = PPA_ALPHA_NO_CHANGE;
    job.cfg.srm.mode               = PPA_TRANS_MODE_NON_BLOCKING;
    return ppa_enqueue(&job);
}

// --- Completion --------------------------------------------------------

void PAL_PPA_WaitJob(uint32_t job_id) {
    // Drain finished ids until job_id pops. Execution is in submission order,
    // so by then everything submitted before it is done as well.
    while (s_inflight > 0) {
        uint32_t done;
        if (xQueueReceive(s_done_q, &done, pdMS_TO_TICKS(PPA_WAIT_TIMEOUT_MS)) != pdTRUE) {
            ESP_LOGW(TAG, "wait_job(%u) timed out (%d in flight)",
                     (unsigned)job_id, s_inflight);
            return;
        }
        s_inflight--;
        if (done == job_id) {
            return;
        }
    }
}

void PAL_PPA_WaitAll(void) {
    while (s_inflight > 0) {
        uint32_t done;
        if (xQueueReceive(s_done_q, &done, pdMS_TO_TICKS(PPA_WAIT_TIMEOUT_MS)) != pdTRUE) {
            ESP_LOGW(TAG, "wait_all timed out (%d in flight)", s_inflight);
            return;
        }
        s_inflight--;
    }
}

int PAL_PPA_Pending(void) {
    return s_inflight;
}
