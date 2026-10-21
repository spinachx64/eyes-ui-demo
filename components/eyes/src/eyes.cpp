#include "../include/eyes/eyes.hpp"
#include <algorithm>
#include <cmath>
#include <cstring>

namespace eyes {
#include "presets.h"
struct Engine::Impl {
static constexpr uint16_t kBlack = 0x0000;
static constexpr uint16_t kEye   = 0xffff;
static constexpr float kPI = 3.14159265f;
struct EyeState { EyePose pose; EyePose vel; };
struct Rect { int x=0, y=0, w=0, h=0; bool valid=false; };
struct Point { float x, y; };
struct EyeFrame { float cx, cy, width, height, angle, lid, tilt, curve; };
EyeState eyes[2]{};
Rect previousBounds[2]{};
int presetIndex = 0;
uint32_t lastFrameMs=0, lastBlinkMs=0, nextBlinkMs=0, nextGazeMs=0;
float gazeX=0.0f, gazeY=0.0f, gazeTargetX=0.0f, gazeTargetY=0.0f;
bool forceFull=true;
// Per-activity motion targets. gaze scales the random gaze offset, open scales eye height.
struct Motion { float gaze, bobAmp, bobFreq, open; int blinkMin, blinkSpan; };
static constexpr Motion kMotion[] = {
  {1.00f,2.8f,1.28f,1.00f,1800,3700}, // Idle: original demo values
  {0.15f,1.2f,0.90f,1.08f,2200,3800}, // Listening: centered, still, slightly wider
  {0.60f,5.0f,4.20f,1.00f,1200,2000}, // Speaking: livelier bob, more frequent blinks
};
Activity activity=Activity::Idle;
float gazeScale=1.0f,bobAmp=2.8f,bobFreq=1.28f,open=1.0f,bobPhase=0.0f;
Canvas& canvas;
uint32_t rng=1;
bool started=false;
float userScale=1.0f;
explicit Impl(Canvas& c):canvas(c){}
int random(int lo,int hi){rng ^= rng << 13; rng ^= rng >> 17; rng ^= rng << 5; return lo + int(rng % uint32_t(hi-lo));}
static bool due(uint32_t now,uint32_t deadline){return int32_t(now-deadline)>=0;}

float clampf(float n,float lo,float hi){return std::max(lo,std::min(n,hi));}
float smoothstep(float t){t=clampf(t,0.0f,1.0f);return t*t*(3.0f-2.0f*t);}
void triggerBlink(uint32_t now){
  const Motion& m=kMotion[(int)activity];
  lastBlinkMs=now;nextBlinkMs=now+m.blinkMin+random(0,m.blinkSpan);
}
void smoothMotion(float dt){
  // Ease toward the activity targets; bob uses a phase accumulator so frequency
  // changes never jump the eyes.
  const Motion& m=kMotion[(int)activity];
  const float k=1.0f-expf(-dt*1000.0f/180.0f);
  gazeScale+=(m.gaze-gazeScale)*k;bobAmp+=(m.bobAmp-bobAmp)*k;
  bobFreq+=(m.bobFreq-bobFreq)*k;open+=(m.open-open)*k;
  bobPhase=fmodf(bobPhase+dt*bobFreq,2*kPI);
}
float blinkFactor(uint32_t now){
  const float p=(float)(now-lastBlinkMs)/280.0f;
  if(p<0||p>1)return 0;
  return p<0.4f?smoothstep(p/0.4f):smoothstep((1-p)/0.6f);
}

void springEye(EyeState& eye,const EyePose& target,float dt){
  // Same 8-dimensional pose model as the browser prototype.
  const float omega=4900.0f/320.0f,zeta=0.82f;
  // Explicit field updates avoid reinterpreting a struct as an array.
  auto step=[&](float& value,float& vel,float goal){
    vel+=((goal-value)*omega*omega-2*zeta*omega*vel)*dt;
    value+=vel*dt;
  };
  step(eye.pose.x,eye.vel.x,target.x);
  step(eye.pose.y,eye.vel.y,target.y);
  step(eye.pose.w,eye.vel.w,target.w);
  step(eye.pose.h,eye.vel.h,target.h);
  step(eye.pose.angle,eye.vel.angle,target.angle);
  step(eye.pose.lid,eye.vel.lid,target.lid);
  step(eye.pose.tilt,eye.vel.tilt,target.tilt);
  step(eye.pose.curve,eye.vel.curve,target.curve);
  eye.pose.w=std::max(3.0f,eye.pose.w);
  eye.pose.h=std::max(3.0f,eye.pose.h);
}

void capsule(const Point& a,const Point& b,float radius,uint16_t color){
  const float dx=b.x-a.x,dy=b.y-a.y;
  const float len=sqrtf(dx*dx+dy*dy);
  if(len<1.0f){canvas.fillCircle(lroundf(a.x),lroundf(a.y),std::max(1,(int)lroundf(radius)),color);return;}
  const float px=-dy/len*radius,py=dx/len*radius;
  const int x0=lroundf(a.x+px),y0=lroundf(a.y+py);
  const int x1=lroundf(a.x-px),y1=lroundf(a.y-py);
  const int x2=lroundf(b.x+px),y2=lroundf(b.y+py);
  const int x3=lroundf(b.x-px),y3=lroundf(b.y-py);
  canvas.fillTriangle(x0,y0,x1,y1,x2,y2,color);
  canvas.fillTriangle(x1,y1,x3,y3,x2,y2,color);
  canvas.fillCircle(lroundf(a.x),lroundf(a.y),lroundf(radius),color);
  canvas.fillCircle(lroundf(b.x),lroundf(b.y),lroundf(radius),color);
}

EyeFrame makeFrame(const EyePose& pose,int side,float blink,uint32_t now) {
  const int W=canvas.width(),H=canvas.height();
  const bool roundLike=(float)W/H<1.2f;
  float sc=roundLike?(float)std::min(W,H)/466.0f:
       std::min((float)W/466.0f*1.15f,(float)H/466.0f*1.5f);
  sc*=userScale;
  const float time=now*0.001f;
  const float bob=sinf(bobPhase)*bobAmp+(strcmp(kPresets[presetIndex].id,"excited")==0?sinf(time*12)*4.5f:0.0f);
  const float subtle=sinf(time*3.1f+side)*1.2f;
  return {W*.5f+(pose.x+gazeX*9*gazeScale+subtle)*sc,
          H*.5f+(pose.y+gazeY*8*gazeScale+bob)*sc,
          std::max(3.0f,pose.w*sc),
          std::max(2.4f,pose.h*(1-blink*.94f)*open*sc),
          pose.angle,pose.lid,pose.tilt,pose.curve*sc};
}

Rect eyeBounds(const EyeFrame& e) {
  // Conservative transformed box; extra margin covers glow, curved eyes and bob.
  const float a=e.angle*kPI/180.0f,c=fabsf(cosf(a)),s=fabsf(sinf(a));
  const float extra=fabsf(e.curve)+6.0f;
  const float halfW=.5f*(e.width*c+e.height*s)+extra;
  const float halfH=.5f*(e.width*s+e.height*c)+extra;
  int W=canvas.width(),H=canvas.height();
  int x=std::max(0,(int)floorf(e.cx-halfW)),y=std::max(0,(int)floorf(e.cy-halfH));
  int r=std::min(W,(int)ceilf(e.cx+halfW)),b=std::min(H,(int)ceilf(e.cy+halfH));
  return {x,y,std::max(0,r-x),std::max(0,b-y),true};
}
Rect mergeRect(Rect a,Rect b){
  if(!a.valid)return b;
  if(!b.valid)return a;
  int x=std::min(a.x,b.x),y=std::min(a.y,b.y),r=std::max(a.x+a.w,b.x+b.w),t=std::max(a.y+a.h,b.y+b.h);
  return {x,y,r-x,t-y,true};
}

void drawEye(const EyeFrame& eye){
  const float rad=eye.angle*kPI/180.0f;
  // A bent happy eye is several joined miniature capsules; otherwise one capsule.
  if(fabsf(eye.curve)>1.5f&&eye.width>eye.height*1.3f){
    const int steps=10;const float half=eye.width*.5f-eye.height*.5f;
    const float c=cosf(rad),s=sinf(rad);
    auto at=[&](int i){
      float t=(float)i/steps, x=(t*2-1)*half;
      float y=eye.curve*(1-(x*x/(half*half+0.001f)));
      return Point{eye.cx+x*c-y*s,eye.cy+x*s+y*c};
    };
    Point prev=at(0);
    for(int i=1;i<=steps;i++){Point next=at(i);capsule(prev,next,eye.height*.5f,kEye);prev=next;}
    return;
  }
  const bool vertical=eye.height>=eye.width;
  const float major=vertical?eye.height:eye.width,minor=vertical?eye.width:eye.height;
  const float length=std::max(0.0f,major-minor);
  const float axis=rad+(vertical?kPI*.5f:0);
  Point start={eye.cx-cosf(axis)*length*.5f,eye.cy-sinf(axis)*length*.5f};
  Point end={eye.cx+cosf(axis)*length*.5f,eye.cy+sinf(axis)*length*.5f};
  capsule(start,end,minor*.5f,kEye);
  // Simple top-lid coverage; angled mask is richer in the browser implementation.
  if(eye.lid>.01f&&fabsf(eye.angle)<10){
    const int cover=(int)lroundf(eye.height*eye.lid);
    canvas.fillRect(lroundf(eye.cx-eye.width*.5f)-2,
                        lroundf(eye.cy-eye.height*.5f)-2,
                        lroundf(eye.width)+4,cover+2,kBlack);
  }
}

void drawFrame(uint32_t now){
  const float blink=blinkFactor(now);
  EyeFrame f[2]={makeFrame(eyes[0].pose,0,blink,now),makeFrame(eyes[1].pose,1,blink,now)};
  Rect bounds[2]={eyeBounds(f[0]),eyeBounds(f[1])};
  canvas.startWrite();
  if(forceFull){canvas.fillScreen(kBlack);forceFull=false;}
  else for(int i=0;i<2;i++){
    const Rect r=mergeRect(previousBounds[i],bounds[i]);
    if(r.valid&&r.w>0&&r.h>0)canvas.fillRect(r.x,r.y,r.w,r.h,kBlack);
  }
  drawEye(f[0]);drawEye(f[1]);
  canvas.endWrite();
  previousBounds[0]=bounds[0];previousBounds[1]=bounds[1];
}
void begin(uint32_t now,uint32_t seed){
  rng=seed?seed:1;
  presetIndex=0;
  eyes[0]={kPresets[0].left,{}};eyes[1]={kPresets[0].right,{}};
  previousBounds[0]={};previousBounds[1]={};
  gazeX=gazeY=gazeTargetX=gazeTargetY=0;
  activity=Activity::Idle;
  gazeScale=kMotion[0].gaze;bobAmp=kMotion[0].bobAmp;bobFreq=kMotion[0].bobFreq;open=kMotion[0].open;
  bobPhase=0;
  forceFull=true;started=true;lastFrameMs=now;lastBlinkMs=now-1000;
  nextBlinkMs=now+1500;nextGazeMs=now+1600;
}
bool update(uint32_t now){
  if(!started)begin(now,1);
  if(canvas.width()<=0 || canvas.height()<=0)return false;
  if(now-lastFrameMs<16)return false;
  const float dt=clampf((now-lastFrameMs)*.001f,.001f,.033f);
  lastFrameMs=now;
  if(due(now,nextBlinkMs))triggerBlink(now);
  if(due(now,nextGazeMs)){
    gazeTargetX=random(-85,86)/100.0f;
    gazeTargetY=random(-55,56)/100.0f;
    nextGazeMs=now+random(1350,3750);
  }
  const float gk=1.0f-expf(-(float)(dt*1000)/120.0f);
  gazeX+=(gazeTargetX-gazeX)*gk;gazeY+=(gazeTargetY-gazeY)*gk;
  smoothMotion(dt);
  springEye(eyes[0],kPresets[presetIndex].left,dt);
  springEye(eyes[1],kPresets[presetIndex].right,dt);
  drawFrame(now);
  return true;
}
};
Engine::Engine(Canvas& c):impl_(new Impl(c)){}
Engine::~Engine() = default;
void Engine::begin(uint32_t now,uint32_t seed){impl_->begin(now,seed);}
bool Engine::update(uint32_t now){return impl_->update(now);}
bool Engine::set_expression(const char* id){
  if(!id)return false;
  for(std::size_t i=0;i<expression_count();++i)
    if(strcmp(id,kPresets[i].id)==0)return set_expression(i);
  return false;
}
bool Engine::set_expression(std::size_t index){
  if(index>=expression_count())return false;
  impl_->presetIndex=static_cast<int>(index);return true;
}
void Engine::next_expression(){set_expression((impl_->presetIndex+1)%expression_count());}
void Engine::blink(uint32_t now){impl_->triggerBlink(now);}
void Engine::invalidate(){impl_->forceFull=true;}
void Engine::set_activity(Activity activity){impl_->activity=activity;}
Activity Engine::activity() const{return impl_->activity;}
void Engine::set_scale(float scale){if(scale>0.0f&&std::isfinite(scale)){impl_->userScale=scale;impl_->forceFull=true;}}
const char* Engine::expression() const{return kPresets[impl_->presetIndex].id;}
std::size_t Engine::expression_count(){return kPresetCount;}
const char* Engine::expression_id(std::size_t index){return index<expression_count()?kPresets[index].id:nullptr;}
} // namespace eyes
