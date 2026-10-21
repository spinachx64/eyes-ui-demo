#include "eyes/eyes.hpp"
#include <algorithm>
#include <cassert>
#include <cstring>
#include <cstdint>

struct Surface : eyes::Canvas {
  int w=466,h=466, clears=0, primitives=0, transactions=0;
  int width() const override { return w; }
  int height() const override { return h; }
  void startWrite() override { ++transactions; }
  void endWrite() override { --transactions; }
  void fillScreen(uint16_t) override { ++clears; }
  void fillRect(int,int,int w,int h,uint16_t) override { assert(w>=0 && h>=0); ++primitives; }
  void fillCircle(int,int,int r,uint16_t) override { assert(r>=0); ++primitives; }
  void fillTriangle(int,int,int,int,int,int,uint16_t) override { ++primitives; }
};
// Records the vertical extent of drawn circles to measure eye motion.
struct Probe : eyes::Canvas {
  int minY=1<<30,maxY=-(1<<30),minX=1<<30,maxX=-(1<<30);
  int width() const override { return 466; }
  int height() const override { return 466; }
  void fillScreen(uint16_t) override {}
  void fillRect(int,int,int w,int h,uint16_t) override { assert(w>=0 && h>=0); }
  void fillCircle(int x,int y,int r,uint16_t c) override {
    assert(r>=0 && x>-1000 && x<2000 && y>-1000 && y<2000); // finite, sane coordinates
    if(!c) return;
    minX=std::min(minX,x);maxX=std::max(maxX,x);minY=std::min(minY,y);maxY=std::max(maxY,y);
  }
  void fillTriangle(int,int,int,int,int,int,uint16_t) override {}
  void reset(){minY=minX=1<<30;maxY=maxX=-(1<<30);}
};

// Runs `ms` of animation and returns the horizontal and vertical motion span.
static void run(eyes::Engine& e,Probe& p,uint32_t& now,uint32_t ms,int& spanX,int& spanY){
  for(uint32_t end=now+ms;now<end;now+=33) e.update(now);
  p.reset();
  for(uint32_t end=now+ms;now<end;now+=33) e.update(now);
  spanX=p.maxX-p.minX;spanY=p.maxY-p.minY;
}

static void activity_tests(){
  Probe p;
  eyes::Engine e(p);
  uint32_t now=0;
  e.begin(now,7);
  assert(e.activity()==eyes::Activity::Idle);
  int idleX,idleY,listenX,listenY,speakX,speakY;
  run(e,p,now,20000,idleX,idleY);
  e.set_activity(eyes::Activity::Listening);
  assert(std::strcmp(e.expression(),"idle")==0); // activity keeps expression
  run(e,p,now,20000,listenX,listenY);
  assert(listenX<idleX); // gaze held near center
  e.set_expression("happy");
  assert(e.activity()==eyes::Activity::Listening); // expression keeps activity
  e.set_expression("idle");
  e.set_activity(eyes::Activity::Speaking);
  run(e,p,now,20000,speakX,speakY);
  assert(speakY>listenY); // livelier bob
  // Rapid toggling must stay finite (asserted inside Probe).
  for(int i=0;i<300;i++){e.set_activity(eyes::Activity(i%3));e.update(now);now+=33;}
}

int main() {
  activity_tests();
  Surface surface;
  eyes::Engine engine(surface);
  engine.begin(0,42);
  assert(!engine.update(15));
  assert(engine.update(16));
  assert(surface.clears==1 && surface.primitives>0 && surface.transactions==0);
  for(std::size_t i=0;i<engine.expression_count();++i) {
    const char* id=engine.expression_id(i);
    assert(id && engine.set_expression(id));
    assert(std::strcmp(id,engine.expression())==0);
    for(uint32_t j=1;j<=100;++j) assert(engine.update(uint32_t(i*100+j)*16+16));
  }
  assert(!engine.set_expression("does-not-exist"));
  assert(!engine.set_expression(static_cast<const char*>(nullptr)));
  assert(!engine.set_expression(engine.expression_count()));
  assert(!engine.expression_id(engine.expression_count()));
  engine.set_expression(engine.expression_count()-1);
  engine.next_expression();
  assert(std::strcmp(engine.expression(),engine.expression_id(0))==0);
  engine.begin(UINT32_MAX-100,42);
  engine.blink(UINT32_MAX-50);
  assert(engine.update(10)); // millisecond rollover
  engine.invalidate();
  const int clears=surface.clears;
  assert(engine.update(26) && surface.clears==clears+1);
  surface.w=0;
  assert(!engine.update(42));
  surface.w=320; surface.h=240;
  engine.invalidate();
  assert(engine.update(58));
  assert(surface.transactions==0);
}
