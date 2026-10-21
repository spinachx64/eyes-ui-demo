#pragma once
#include <cstddef>
#include <cstdint>
#include <memory>

namespace eyes {
// RGB565 drawing surface. The application owns the surface and its display task.
class Canvas {
public:
  virtual ~Canvas() = default;
  virtual int width() const = 0;
  virtual int height() const = 0;
  virtual void startWrite() {}
  virtual void endWrite() {}
  virtual void fillScreen(uint16_t color) = 0;
  virtual void fillRect(int x, int y, int w, int h, uint16_t color) = 0;
  virtual void fillCircle(int x, int y, int radius, uint16_t color) = 0;
  virtual void fillTriangle(int x0, int y0, int x1, int y1, int x2, int y2, uint16_t color) = 0;
};

// Device activity layered on top of the expression. The expression sets the eye
// shape; the activity sets gaze range, bob and blink rhythm. Both are independent.
enum class Activity : uint8_t { Idle, Listening, Speaking };

// Single-task API. Call all methods from the task that owns the display.
// Canvas must outlive Engine. update() uses caller-supplied monotonic milliseconds.
class Engine {
public:
  explicit Engine(Canvas& canvas);
  ~Engine();
  Engine(const Engine&) = delete;
  Engine& operator=(const Engine&) = delete;
  void begin(uint32_t now_ms, uint32_t random_seed = 1);
  bool update(uint32_t now_ms); // true when a frame was drawn; target interval 16ms
  bool set_expression(const char* id); // false leaves current expression unchanged
  bool set_expression(std::size_t index);
  void next_expression();
  void blink(uint32_t now_ms);
  void invalidate(); // redraw background after resize/external screen changes
  void set_activity(Activity activity); // keeps the expression; transition is smoothed
  Activity activity() const;
  void set_scale(float scale); // extra size multiplier on top of the fit-to-canvas scale; >0
  const char* expression() const;
  static std::size_t expression_count();
  static const char* expression_id(std::size_t index); // nullptr if out of range
private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
};
} // namespace eyes
