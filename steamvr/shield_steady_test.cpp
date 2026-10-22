#include "../vr/shield_steady.h"
#include <cstdio>
#include <cstdlib>
using namespace skel::steady;
namespace bow=skel::bow;
void Check(bool ok,const char* why){if(!ok){std::fprintf(stderr,"FAIL: %s\n",why);std::exit(1);}}
bool Near(float a,float b,float e=1e-3f){return std::fabs(a-b)<=e;}
// A shield matrix turned deg about up (y), at x, with a row scale.
bow::Mat Held(float deg,float x,float scale=1.2f) {
    const float r=deg/57.2958f,c=std::cos(r),s=std::sin(r);bow::Mat m;
    const float rows[3][3]={{c,0,-s},{0,1,0},{s,0,c}};
    for(int i=0;i<3;++i)for(int j=0;j<3;++j)m.m[i*4+j]=rows[i][j]*scale;
    m.m[12]=x;m.m[13]=1;m.m[14]=2;m.m[15]=1;return m;
}
float Yaw(const bow::Mat& m){return std::atan2(-m.m[2],m.m[0])*57.2958f;}
// Settings as the headset tuning described them: dead zones (deg, mm), strength (cutoff 10/strength Hz), fast (beta 0.02 a step).
Settings S(float angle,float mm,float strength,float fast){Settings s;s.angleDeadDeg=angle;s.moveDeadCm=mm*.1f;s.cutoffHz=strength>0?10/strength:0;s.beta=fast*.02f;return s;}
int main() {
    const float dt=1/72.f,cm=.02f;   // 72 fps; 0.02 model units per cm
    // Quaternion round trip keeps the rotation.
    {float r[3][3]={{0,0,-1},{0,1,0},{1,0,0}},b[3][3];ToRows(FromRows(r),b);
     for(int i=0;i<3;++i)for(int j=0;j<3;++j)Check(Near(r[i][j],b[i][j]),"quaternion round trip");}
    // Off (all zero): the shield passes through exactly.
    {Filter f;const Settings off;bow::Mat m=Held(0,0);f.Apply(m,dt,cm,off);
     for(int i=1;i<60;++i){m=Held(i%2?1.f:0.f,i%2?.01f:0.f);f.Apply(m,dt,cm,off);
         Check(Near(Yaw(m),i%2?1.f:0.f,.01f)&&Near(m.m[12],i%2?.01f:0.f,1e-5f),"all zero = no change");}}
    // A 0.3-degree shiver inside a 0.5-degree dead zone is gone; scale kept.
    {Filter f;const auto s=S(.5f,0,0,0);bow::Mat m=Held(0,0);f.Apply(m,dt,cm,s);
     for(int i=1;i<60;++i){m=Held(i%2?.3f:0.f,0);f.Apply(m,dt,cm,s);Check(Near(Yaw(m),0,.01f),"shiver inside the dead zone dropped");}
     Check(Near(std::sqrt(m.m[0]*m.m[0]+m.m[1]*m.m[1]+m.m[2]*m.m[2]),1.2f),"row scale kept");}
    // Past the dead zone only the excess passes (no jump at its edge).
    {Filter f;const auto s=S(.5f,0,0,0);bow::Mat m=Held(0,0);f.Apply(m,dt,cm,s);
     m=Held(2,0);f.Apply(m,dt,cm,s);Check(Near(Yaw(m),1.5f,.02f),"2 degrees past a 0.5 dead zone moves 1.5");}
    // Position dead zone in mm: 0.25 mm jitter inside 1 mm is dropped.
    {Filter f;const auto s=S(0,1,0,0);bow::Mat m=Held(0,0);f.Apply(m,dt,cm,s);
     for(int i=1;i<60;++i){m=Held(0,i%2?.025f*cm:0.f);f.Apply(m,dt,cm,s);Check(Near(m.m[12],0,1e-6f),"position jitter dropped");}}
    // Smoothing: a slow shiver is reduced a lot ...
    float slowSwing=0;
    {Filter f;const auto s=S(0,0,6,5);bow::Mat m=Held(0,0);f.Apply(m,dt,cm,s);float lo=1e9f,hi=-1e9f;
     for(int i=1;i<144;++i){m=Held(i%2?1.f:0.f,0);f.Apply(m,dt,cm,s);if(i>72){lo=std::min(lo,Yaw(m));hi=std::max(hi,Yaw(m));}}
     slowSwing=hi-lo;Check(slowSwing<.3f,"a 1-degree shiver shrinks under a third");}
    // ... while a fast block follows almost at once: 90 degrees in 0.1 s.
    {Filter f;const auto s=S(0,0,6,5);bow::Mat m=Held(0,0);f.Apply(m,dt,cm,s);
     for(int i=1;i<=7;++i){m=Held(90.f*i/7,0);f.Apply(m,dt,cm,s);}
     const float at=Yaw(m);for(int i=0;i<4;++i){m=Held(90,0);f.Apply(m,dt,cm,s);}
     Check(at>60&&Yaw(m)>84,"a fast 90-degree block lags under a third, then lands within 6 degrees in 4 frames");}
    // Without fast response the same block lags clearly (what the slider is for).
    {Filter f;const auto s=S(0,0,6,0);bow::Mat m=Held(0,0);f.Apply(m,dt,cm,s);
     for(int i=1;i<=7;++i){m=Held(90.f*i/7,0);f.Apply(m,dt,cm,s);}Check(Yaw(m)<40,"no fast response: heavy lag");}
    // A gap restarts: after 0.5 s the shield is taken as it is.
    {Filter f;const auto s=S(1,5,10,0);bow::Mat m=Held(0,0);f.Apply(m,dt,cm,s);
     m=Held(45,.5f);f.Apply(m,.5f,cm,s);Check(Near(Yaw(m),45,.01f)&&Near(m.m[12],.5f),"a gap restarts the filter");}
    // A degenerate matrix resets and passes through.
    {Filter f;bow::Mat z;f.Apply(z,dt,cm,S(1,1,5,5));Check(!f.ok,"degenerate matrix rejected");}
    // The shipped values are the tuned ones: 0.8 deg, 1.5 mm, strength 6, fast 5.
    {const auto t=S(.8f,1.5f,6,5);Check(Near(Shield.angleDeadDeg,t.angleDeadDeg)&&Near(Shield.moveDeadCm,t.moveDeadCm)&&Near(Shield.cutoffHz,t.cutoffHz)&&Near(Shield.beta,t.beta),"shipped values = the headset tuning");}
    // With them: a held-still shiver (1 degree, 0.25 mm) is gone, a fast block keeps up.
    {Filter f;bow::Mat m=Held(0,0);f.Apply(m,dt,cm,Shield);float lo=1e9f,hi=-1e9f;
     for(int i=1;i<144;++i){m=Held(i%2?1.f:0.f,i%2?.025f*cm:0.f);f.Apply(m,dt,cm,Shield);if(i>72){lo=std::min(lo,Yaw(m));hi=std::max(hi,Yaw(m));Check(Near(m.m[12],0,1e-6f),"position jitter gone");}}
     Check(hi-lo<.2f,"a 1-degree shiver nearly gone");}
    {Filter f;bow::Mat m=Held(0,0);f.Apply(m,dt,cm,Shield);for(int i=1;i<=7;++i){m=Held(90.f*i/7,0);f.Apply(m,dt,cm,Shield);}
     const float at=Yaw(m);for(int i=0;i<4;++i){m=Held(90,0);f.Apply(m,dt,cm,Shield);}Check(at>55&&Yaw(m)>83,"a fast block keeps up with the shipped values");}
    std::printf("Shield steady: dead zones, slow shiver %.2f deg left of 1, fast blocks, gaps and Off passed\n",slowSwing);
}
