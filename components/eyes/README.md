# eyes — ESP-IDF component

C++17 procedural eyes engine. No Arduino, M5Unified, LVGL or ESP-specific runtime
calls. Register `components/eyes` via `EXTRA_COMPONENT_DIRS`, then add `eyes` to
`REQUIRES` in your application's `idf_component_register`.

```cmake
# Project CMakeLists.txt, before include(project.cmake)
set(EXTRA_COMPONENT_DIRS "/path/to/eyes-ui-demo/components/eyes")
```

```cpp
#include <M5Unified.h>
#include "eyes/eyes.hpp"
#include "eyes/m5gfx_canvas.hpp"

// Initialize display first. Keep canvas alive for as long as engine exists.
eyes::M5GFXCanvas<decltype(M5.Display)> canvas(M5.Display);
eyes::Engine engine(canvas);
engine.begin(now_ms, random_seed);
engine.set_expression("happy");
engine.blink(now_ms);
engine.update(now_ms); // call repeatedly with monotonic uint32_t milliseconds
```

`begin()` resets to the first preset. Call it before setting an expression.
`update()` draws at most once per 16ms and returns whether a frame was drawn.
The interval is a target, not a measured FPS guarantee. Millisecond rollover is
supported. The engine does not create a task or own the display hardware.

- `set_expression(const char*)`: select by ID; returns false for an unknown ID.
- `set_expression(std::size_t)`: select by index; returns false outside the table.
- `next_expression()`: cycles through the entire generated table, including additions.
- `expression()`, `expression_count()`, `expression_id(index)`: inspect the table.
- `blink(now_ms)`: manual blink; automatic blinking/gaze are enabled by default.
- `invalidate()`: clear and redraw after an external screen change or resize.

All calls must run in the same task that owns the screen. Pass commands from other
tasks via an application queue. Multiple engine instances keep independent state.
The default renderer owns the full canvas background (black), with white RGB565
eyes and dirty-region updates. For embedding, supply a canvas for an isolated
region or offscreen surface; do not let LVGL and this renderer write to the same
screen concurrently. Implement `eyes::Canvas` to use another graphics backend.

## Add expressions

The repository's `presets.json` remains the only editable expression source.
Duplicate an emotion entry, give it a unique nonempty `id`, a `name`, and left/right
poses with `x,y,w,h,angle,lid,tilt,curve`. Run:

```sh
python3 tools/generate_presets.py
python3 tools/build_web.py
```

The first command updates the component's `src/presets.h` and the legacy Arduino
header. The second refreshes the offline browser preview. Rebuild and flash after
changing presets; runtime JSON loading is not implemented. The IDF helper script
runs both automatically. No expression-count constant or button code needs editing.

The renderer retains the original firmware's simplified eyelid treatment:
`tilt` is stored/interpolated but not rendered; lid masking applies only to nearly
unrotated eyes. Browser preview and firmware are not pixel-identical.

## Verify on a desktop

From repository root (Clang/GCC required):

```sh
python3 -m unittest discover -s tests -v
c++ -std=c++17 -Wall -Wextra -Werror -fsanitize=address,undefined \
  -I components/eyes/include components/eyes/src/eyes.cpp tests/engine_test.cpp \
  -o /tmp/eyes-engine-test
/tmp/eyes-engine-test
```

Tests cover generated-data consistency and additions, invalid IDs and poses,
expression cycling, draw scheduling, resize and clock rollover. The complete
StopWatch target is validated separately with the ESP-IDF build.
