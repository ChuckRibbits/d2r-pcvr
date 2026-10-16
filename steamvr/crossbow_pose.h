#pragma once
#include "../vr/bow_pose.h"
#include "world_coordinates.h"
#include <cstdint>

namespace framevr::crossbow {
using math::Vec;
using skel::bow::Mat;
using skel::bow::Vector;
using skel::bow::Position;
using skel::bow::Multiply;
inline constexpr float BoltTip=.69698172f; // owned x_bow_bolt mesh, local +Z
struct Line {Vec center{},direction{},up{};float scale=0;};
inline bool FromBolt(const Mat& bolt,Line& out) {
    Mat basis=bolt;Position(basis,{});const Vec center=Position(bolt);
    if(!skel::bow::Affine(basis)||!std::isfinite(center.x+center.y+center.z))return false;
    const Vec z=Vector(bolt,{0,0,1}),y=Vector(bolt,{0,1,0}),x=Vector(bolt,{1,0,0});
    const float scale=std::sqrt(math::Dot(z,z));
    if(!std::isfinite(scale)||scale<.01f||scale>20)return false;
    const Vec direction=math::Normalized(z),up=math::Normalized(y-direction*math::Dot(y,direction));
    if(math::Dot(up,up)<.99f||std::fabs(math::Dot(x,math::Cross(y,z)))<1e-8f)return false;
    out={center,direction,up,scale};return true;
}
inline Vec Muzzle(const Line& line){return line.center+line.direction*(BoltTip*line.scale);}
// The support palm is under the actual stock, not on a separately rotated
// controller ray. Project the driving palm onto that line before advancing.
inline Vec Support(const Line& line,Vec rightPalm,float ahead,float unitsPerMetre) {
    const float along=math::Dot(rightPalm-line.center,line.direction)+ahead*unitsPerMetre;
    return line.center+line.direction*along-line.up*(.03f*unitsPerMetre);
}
struct Pose {
    Vec muzzle{},direction{},player{};
    float scale=0;
    uint64_t at=0,session=0;
    uint32_t equipment=0,worldEpoch=0,heroGeneration=0;
};
inline bool Fresh(const Pose& p,uint64_t now,uint64_t session,uint32_t equipment,uint32_t epoch) {
    return p.session&&p.at&&now>=p.at&&now-p.at<=100&&p.session==session&&p.equipment==equipment&&p.worldEpoch==epoch&&
        std::isfinite(p.muzzle.x+p.muzzle.y+p.muzzle.z+p.player.x+p.player.y+p.player.z+p.scale)&&
        p.scale>.01f&&p.scale<20&&math::Dot(p.direction,p.direction)>.99f&&math::Dot(p.direction,p.direction)<1.01f;
}
struct Launch {Pose pose{};int x=0,y=0,tx=0,ty=0;};
inline bool Aim(const Pose& pose,Launch& out) {
    const Vec grid=coordinates::GridPlane({pose.muzzle.x,pose.player.y,pose.muzzle.z});
    const float horizontal=std::hypot(pose.direction.x,pose.direction.z);
    if(!std::isfinite(grid.x+grid.z+horizontal)||horizontal<.05f||grid.x<2||grid.z<2||grid.x>131000||grid.z>131000)return false;
    Launch shot;shot.pose=pose;shot.x=int(grid.x*.5f);shot.y=int(grid.z*.5f);
    shot.tx=shot.x+int(pose.direction.x/horizontal*60);shot.ty=shot.y+int(pose.direction.z/horizontal*60);
    if(shot.tx<1||shot.ty<1||shot.tx>65534||shot.ty>65534)return false;
    out=shot;return true;
}
// Native bolts retain their native ground path, speed, hits, ammo and cadence.
// Place their visible tip at the frozen muzzle plus that path's displacement.
// This also removes native grid quantization and the authored +6/-1 VFX offset.
inline bool Matrix(const Launch& launch,Vec grid,Mat& out) {
    if(!std::isfinite(grid.x+grid.z))return false;
    const Vec forward=math::Normalized(Vec{float(launch.tx-launch.x),0,float(launch.ty-launch.y)});
    if(math::Dot(forward,forward)<.99f)return false;
    const Vec right=math::Normalized(math::Cross({0,1,0},forward)),up=math::Cross(forward,right);
    const Vec tip=launch.pose.muzzle+grid-Vec{2.f*launch.x,0,2.f*launch.y};
    out={};const Vec rows[3]={right,up,forward};
    for(int i=0;i<3;++i){out.m[i*4]=rows[i].x*launch.pose.scale;out.m[i*4+1]=rows[i].y*launch.pose.scale;out.m[i*4+2]=rows[i].z*launch.pose.scale;}
    Position(out,tip-forward*(BoltTip*launch.pose.scale));out.m[15]=1;return true;
}
}
