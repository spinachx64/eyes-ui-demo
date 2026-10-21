#pragma once
#include "eyes.hpp"

namespace eyes {
// Header-only adapter; Display can be an M5GFX display or an M5Canvas sprite.
// Include M5GFX/M5Unified before this header. No hardware dependency in the engine.
template<class Display>
class M5GFXCanvas final : public Canvas {
public:
  explicit M5GFXCanvas(Display& display): display_(display) {}
  int width() const override { return display_.width(); }
  int height() const override { return display_.height(); }
  void startWrite() override { display_.startWrite(); }
  void endWrite() override { display_.endWrite(); }
  void fillScreen(uint16_t c) override { display_.fillScreen(c); }
  void fillRect(int x,int y,int w,int h,uint16_t c) override { display_.fillRect(x,y,w,h,c); }
  void fillCircle(int x,int y,int r,uint16_t c) override { display_.fillCircle(x,y,r,c); }
  void fillTriangle(int x0,int y0,int x1,int y1,int x2,int y2,uint16_t c) override {
    display_.fillTriangle(x0,y0,x1,y1,x2,y2,c);
  }
private:
  Display& display_;
};
} // namespace eyes
