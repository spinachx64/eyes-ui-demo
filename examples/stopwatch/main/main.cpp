#include <M5Unified.h>
#include "eyes/eyes.hpp"
#include "eyes/m5gfx_canvas.hpp"
#include "esp_log.h"
#include "esp_random.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

extern "C" void app_main() {
  auto cfg = M5.config();
  M5.begin(cfg);
  M5.Display.setBrightness(95);
  // M5GFX reports a 468x468 addressable canvas for the 466x466 panel.
  // Use its runtime dimensions rather than rejecting the detected hardware.
  if (M5.Display.width() <= 0 || M5.Display.height() <= 0) {
    ESP_LOGE("eyes", "Invalid display: %dx%d; check StopWatch detection",
             M5.Display.width(), M5.Display.height());
    return;
  }
  eyes::M5GFXCanvas<decltype(M5.Display)> canvas(M5.Display);
  eyes::Engine engine(canvas);
  auto now_ms = [] { return static_cast<uint32_t>(esp_timer_get_time() / 1000); };
  engine.begin(now_ms(), esp_random());
  ESP_LOGI("eyes", "Ready: %dx%d, %u expressions. A: next, B: blink",
           canvas.width(), canvas.height(), unsigned(engine.expression_count()));
  // This task exclusively owns engine and display. Forward commands from other
  // tasks through a queue rather than calling the engine concurrently.
  bool first_frame = true;
  while (true) {
    M5.update();
    const uint32_t now = now_ms();
    if (M5.BtnA.wasClicked()) {
      engine.next_expression();
      ESP_LOGI("eyes", "expression=%s", engine.expression());
    }
    if (M5.BtnB.wasClicked()) engine.blink(now);
    if (engine.update(now) && first_frame) {
      ESP_LOGI("eyes", "First frame rendered");
      first_frame = false;
    }
    vTaskDelay(pdMS_TO_TICKS(1));
  }
}
