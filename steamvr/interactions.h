#pragma once
#include "math.h"
#include <array>
#include <cstdint>
#include <cstring>

// Value-only bridge. UI/game threads publish snapshots; XR never follows native
// pointers. Requests name a session and an exact item, not a guessed belt slot.
namespace framevr::interaction {
using math::Vec; using math::Quat;
struct Pose { Vec p{}; Quat q{0,0,0,1}; };
inline bool Valid(Pose p) {
    const float n=p.q.x*p.q.x+p.q.y*p.q.y+p.q.z*p.q.z+p.q.w*p.q.w;
    return std::isfinite(p.p.x)&&std::isfinite(p.p.y)&&std::isfinite(p.p.z)&&std::isfinite(n)&&n>.9f&&n<1.1f;
}
enum class Owner { Combat, Wheel, Bottle, Holster, Journal, Rearm };
enum class Potion { Unknown, Health, Mana, Rejuvenation, Antidote, Thawing, Stamina };
struct Item {
    uint64_t handle=0;
    uint32_t id=0,code=0;
    bool operator==(const Item&) const = default;
};
constexpr uint32_t Code(char a,char b=' ',char c=' ',char d=' ') {
    return uint32_t(a)|(uint32_t(b)<<8)|(uint32_t(c)<<16)|(uint32_t(d)<<24);
}
inline Potion PotionType(uint32_t code) {
    if((code&0xffff)==Code('h','p',0,0))return Potion::Health;
    if((code&0xffff)==Code('m','p',0,0))return Potion::Mana;
    code=(code&0xffffff)|uint32_t(' ')<<24; // tolerate zero or space padding
    if(code==Code('r','v','s')||code==Code('r','v','l'))return Potion::Rejuvenation;
    if(code==Code('y','p','s'))return Potion::Antidote;
    if(code==Code('w','m','s'))return Potion::Thawing;
    if(code==Code('v','p','s'))return Potion::Stamina;
    return Potion::Unknown;
}
inline const char* PotionName(Potion type) {
    constexpr const char* names[]={"Unknown", "Health", "Mana", "Rejuvenation", "Antidote", "Thawing", "Stamina"};
    return names[unsigned(type)<=unsigned(Potion::Stamina)?unsigned(type):0];
}
struct Belt {
    std::array<Item,4> items{}; // bottom first; native belt action uses bottom
    uint32_t revision=0;
    uint8_t count=0;
    bool known=false;
    Item Next() const {return items[0];}
    bool operator==(const Belt&) const = default;
};
struct Skill {
    int32_t id=-1,level=0,charges=-1;
    bool known=false,available=false;
    char name[64]{};
    bool operator==(const Skill&) const = default;
};
struct Snapshot {
    uint64_t session=0,at=0;
    uint32_t revision=0,activeSet=0;
    bool equipmentKnown=false,beltKnown=false;
    std::array<std::array<Item,2>,2> sets{};
    std::array<Belt,4> belt{};
    std::array<Skill,8> skills{};
    float health=0,maxHealth=0,mana=0,maxMana=0;
    bool vitalsKnown=false;
};
inline bool Fresh(const Snapshot& s,uint64_t session,uint64_t now) {
    return session&&s.session==session&&now>=s.at&&now-s.at<=650;
}
enum class Action { None, Potion, Equip, Quest, Map, Character, Skills };
enum class Result { None, Pending, Applied, Refused, Cancelled };
struct Request {
    Action action=Action::None;
    uint64_t sequence=0,session=0,at=0;
    int index=-1;
    Item item{};
    uint32_t revision=0;
};
struct Reply {uint64_t sequence=0,session=0;Result result=Result::None;};
inline bool SameBelt(const Snapshot& s,const Request& r) {
    if(!Fresh(s,r.session,r.at)||r.index<0||r.index>=4)return false;
    const auto& b=s.belt[r.index];
    return b.known&&b.count&&b.Next().handle&&b.Next()==r.item&&b.revision==r.revision;
}
inline bool PotionGone(const Snapshot& s,const Request& r) {
    if(s.session!=r.session||!s.beltKnown)return false;
    for(const auto& b:s.belt)for(const auto& item:b.items)if(item.handle==r.item.handle)return false;
    return r.item.handle!=0;
}

// Wheel coordinates are shared by painter and hit testing. Eight native slots
// clockwise from twelve o'clock; center is weapon mode; bottom tab is utilities.
constexpr float Pi=3.14159265358979323846f;
inline int WheelCell(float u,float v,int previous=-1) {
    if(!std::isfinite(u)||!std::isfinite(v)||u<0||u>1||v<0||v>1)return -1;
    if(v>.90f&&u>.72f&&u<.98f)return 10;
    if(v>.90f&&u>.06f&&u<.70f)return 9;
    const float x=u-.5f,y=v-.45f,r=std::hypot(x,y);
    if(r<.105f)return 8;
    if(r<.14f||r>.405f)return -1;
    float a=std::atan2(x,-y);if(a<0)a+=2*Pi;
    if(previous>=0&&previous<8) {
        const float difference=std::remainder(a-previous*Pi/4,2*Pi);
        if(std::fabs(difference)<Pi/8+.035f)return previous;
    }
    return int(std::floor(a/(Pi/4)+.5f))%8;
}
struct Config {
    float waistBelowEyes=.65f,beltForward=.26f,hipWidth=.28f;
    bool seated=false,holsters=true;
};
struct Input {
    double time=0;
    uint64_t session=0,epoch=0,at=0; // at shares the native snapshot's monotonic clock
    bool active=false,tracked[2]{},wheel=false,cancel=false,reservedLeft=false,spell=false;
    Pose head{},hand[2]{};
    float trigger=0,grip[2]{};
    int hover=-1;
    Snapshot snapshot{};
    Reply reply{};
};
struct Output {
    Owner owner[2]{Owner::Combat,Owner::Combat};
    bool block=false,wheel=false,utility=false,stowed=false,wrist=false,cancelPending=false,critical=false,settings=false;
    int select=-2,bottle=-1,bottleHand=-1,nearBelt=-1,nearHolster=-1,hover=-1;
    int feedback=0,feedbackHand=0;
    Result result=Result::None;
    Item bottleItem{};
    bool bottleEmpty=false;
    Pose body{},belt[4]{},holster[2]{};
    Request request{};
};
class Engine {
    Config config{};
    uint64_t session=0,epoch=0,sequence=0;
    double lastTime=0,drinkStart=0,pendingAt=0,resultAt=0,criticalUntil=0,criticalAgain=0;
    bool primed=false,triggerWas=false,gripWas[2]{},wheelLatch=false,utility=false,rearm=true,stowed=false;
    bool spent=false,consumed=false;
    Owner owner[2]{Owner::Combat,Owner::Combat};
    int bottle=-1,bottleHand=-1,holster=-1,holsterHand=-1;
    Pose body{},grab{};
    Request held{},pending{};
    Result result=Result::None;
    void Drop() {bottle=bottleHand=holster=holsterHand=-1;drinkStart=0;spent=consumed=false;held={};owner[0]=owner[1]=Owner::Combat;}
public:
    void Configure(Config c) {config=c;Reset();}
    void Reset() {primed=false;rearm=true;wheelLatch=true;Drop();pending={};result=Result::None;criticalUntil=criticalAgain=0;}
    bool Utility() const {return utility;}
    bool WheelVisible(bool heldButton) const {return heldButton&&!wheelLatch;}
    Output Step(const Input& in) {
        Output out;
        const bool trigger=in.trigger>(triggerWas?.2f:.55f);
        const bool grips[2]={in.grip[0]>.6f,in.grip[1]>.6f};
        const uint64_t now=in.at;
        const bool fresh=Fresh(in.snapshot,in.session,now);
        const double dt=in.time-lastTime;
        if(!in.active||!in.tracked[0]||!in.tracked[1]||!std::isfinite(in.time)||
           !Valid(in.head)||!Valid(in.hand[0])||!Valid(in.hand[1])||!primed||session!=in.session||epoch!=in.epoch||dt<=0||dt>.15) {
            Reset();primed=in.active&&in.tracked[0]&&in.tracked[1]&&Valid(in.head)&&Valid(in.hand[0])&&Valid(in.hand[1])&&std::isfinite(in.time);
            if(session!=in.session)stowed=false;
            session=in.session;epoch=in.epoch;lastTime=in.time;
            triggerWas=trigger;gripWas[0]=grips[0];gripWas[1]=grips[1];
            body=Valid(in.head)?Pose{in.head.p,math::Yaw(math::Heading(in.head.q))}:Pose{};out.block=true;out.stowed=stowed;out.cancelPending=true;return out;
        }
        lastTime=in.time;
        const bool triggerEdge=trigger&&!triggerWas;
        const bool edges[2]={grips[0]&&!gripWas[0],grips[1]&&!gripWas[1]};
        triggerWas=trigger;gripWas[0]=grips[0];gripWas[1]=grips[1];
        if(!in.wheel)wheelLatch=false;
        if(!trigger&&!grips[0]&&!grips[1])rearm=false;
        if(in.cancel) {Drop();rearm=true;wheelLatch=true;pending={};out.cancelPending=true;}
        if(pending.action!=Action::None) {
            if(in.reply.sequence==pending.sequence&&in.reply.session==session&&in.reply.result!=Result::Pending) {
                result=in.reply.result;resultAt=in.time;out.feedback=result==Result::Applied?2:3;
                if(result==Result::Applied&&pending.action==Action::Equip)stowed=false;
                if(result==Result::Applied&&pending.action==Action::Potion&&pending.item==held.item)consumed=true;
                pending={};
            } else if(in.time-pendingAt>1.5) {result=Result::Refused;resultAt=in.time;out.feedback=3;pending={};out.cancelPending=true;}
        }
        if(result!=Result::Pending&&in.time-resultAt>2.5)result=Result::None;
        const auto& stats=in.snapshot;
        const bool low=fresh&&stats.vitalsKnown&&stats.health>0&&stats.maxHealth>0&&
            (stats.health/stats.maxHealth<.25f||(in.spell&&stats.maxMana>0&&stats.mana/stats.maxMana<.10f));
        if(low&&in.time>=criticalAgain){criticalUntil=in.time+1.5;criticalAgain=in.time+10;out.feedback=4;out.feedbackHand=1;}
        if(!low)criticalUntil=criticalAgain=0;
        out.critical=low&&in.time<criticalUntil;
        auto request=[&](Action a,int index,Item item={},uint32_t revision=0) {
            if(pending.action!=Action::None)return false;
            pending={a,++sequence,session,now,index,item,revision};pendingAt=in.time;
            out.request=pending;result=Result::Pending;return true;
        };
        // An estimated torso, not head-locked widgets. Small head glances do
        // not turn the belt, and anchors freeze while a hand approaches/grabs.
        bool nearby=bottle>=0||holster>=0;
        for(int h=0;h<2;++h)if(std::hypot(in.hand[h].p.x-body.p.x,in.hand[h].p.z-body.p.z)<.55f&&
            in.hand[h].p.y<body.p.y-.3f)nearby=true;
        if(!nearby) {
            const float yaw=math::Heading(body.q),wanted=math::Heading(in.head.q);
            const float delta=std::remainder(wanted-yaw,2*Pi);
            body.q=math::Yaw(yaw+(std::fabs(delta)>.65f?std::clamp(delta,float(-dt*2),float(dt*2)):0));
        }
        if(bottle<0&&holster<0&&(!nearby||std::hypot(body.p.x-in.head.p.x,body.p.z-in.head.p.z)>.35f))body.p=in.head.p;
        out.body=body;
        const float waist=config.seated?.43f:config.waistBelowEyes;
        for(int i=0;i<4;++i)out.belt[i]={body.p+math::Rotate(body.q,{(i-1.5f)*.095f,-waist+.08f,-config.beltForward}),body.q};
        for(int i=0;i<2;++i)out.holster[i]={body.p+math::Rotate(body.q,{(i==0?1:-1)*config.hipWidth,-waist,-.06f}),body.q};
        auto distance=[](Vec a,Vec b){return std::sqrt(math::Dot(a-b,a-b));};
        const Vec wristNormal=math::Rotate(in.hand[1].q,{0,1,0});
        out.wrist=distance(in.hand[1].p,in.head.p)<.65f&&math::Dot(wristNormal,math::Normalized(in.head.p-in.hand[1].p))>.65f;
        const bool wheel=in.wheel&&!wheelLatch&&bottle<0&&holster<0;
        if(wheel) {
            out.wheel=true;out.utility=utility;out.hover=in.hover;out.block=true;
            owner[0]=owner[1]=Owner::Wheel;
            if(!rearm&&triggerEdge) {
                if(in.hover==10){out.settings=true;out.wheel=false;wheelLatch=true;rearm=true;}
                else if(in.hover==9){utility=!utility;out.utility=utility;rearm=true;}
                else if(in.hover==8) {out.select=-1;stowed=false;wheelLatch=true;out.wheel=false;rearm=true;out.feedback=1;}
                else if(in.hover>=0&&in.hover<8) {
                    if(!utility) {
                        const auto& skill=in.snapshot.skills[in.hover];
                        if(!fresh||!skill.known||skill.available) {
                            out.select=in.hover;stowed=false;wheelLatch=true;out.wheel=false;out.feedback=1;
                        } else out.feedback=3;
                    } else if(in.hover<2) {if(fresh&&in.snapshot.equipmentKnown)request(Action::Equip,in.hover+1);else out.feedback=3;}
                    else if(in.hover==2)request(Action::Quest,0);
                    else if(in.hover==3) {stowed=!stowed;out.feedback=1;}
                    else {
                        const int col=in.hover-4;const auto& b=in.snapshot.belt[col];
                        if(fresh&&b.known&&b.count&&b.Next().handle)request(Action::Potion,col,b.Next(),b.revision);
                        else out.feedback=3;
                    }
                    if(utility){wheelLatch=true;out.wheel=false;}
                    rearm=true;
                }
            }
            if(utility&&!rearm&&edges[0]&&in.hover>=4&&in.hover<8) {
                const auto& b=in.snapshot.belt[in.hover-4];
                if(fresh&&b.known&&b.count&&b.Next().handle) {
                    bottle=in.hover-4;bottleHand=0;held={Action::Potion,0,session,now,bottle,b.Next(),b.revision};
                    owner[0]=Owner::Bottle;wheelLatch=true;out.wheel=false;out.feedback=1;
                }
            }
        } else if(bottle<0&&holster<0)owner[0]=owner[1]=Owner::Combat;
        if(!wheel&&!rearm&&bottle<0&&holster<0&&pending.action==Action::None&&fresh) {
            for(int h=0;h<2;++h) {
                if(h==1&&in.reservedLeft)continue;
                for(int i=0;i<4;++i)if(distance(in.hand[h].p,out.belt[i].p)<.095f) {
                    out.nearBelt=i;
                    const auto& b=in.snapshot.belt[i];
                    if(edges[h]&&b.known&&b.count&&b.Next().handle) {
                        bottle=i;bottleHand=h;held={Action::Potion,0,session,now,i,b.Next(),b.revision};
                        owner[h]=Owner::Bottle;out.feedback=1;out.feedbackHand=h;break;
                    }
                }
                if(bottle>=0)break;
                for(int i=0;i<2&&config.holsters;++i)if(distance(in.hand[h].p,out.holster[i].p)<.115f) {
                    out.nearHolster=i;
                    if(edges[h]&&in.snapshot.equipmentKnown) {
                        holster=i;holsterHand=h;grab=in.hand[h];owner[h]=Owner::Holster;out.feedback=1;out.feedbackHand=h;break;
                    }
                }
                if(holster>=0)break;
            }
        }
        if(holster>=0) {
            out.block=true;out.nearHolster=holster;
            if(!grips[holsterHand]){Drop();rearm=true;}
            else if(distance(grab.p,in.hand[holsterHand].p)>.12f) {
                request(Action::Equip,holster+1);Drop();rearm=true;
            }
        }
        if(bottle>=0) {
            out.block=true;
            held.at=now;
            if(!grips[bottleHand]||(!spent&&(!fresh||!SameBelt(in.snapshot,held)))) {Drop();rearm=true;}
            else {
                out.bottle=bottle;out.bottleHand=bottleHand;out.bottleItem=held.item;out.bottleEmpty=consumed;owner[bottleHand]=Owner::Bottle;
                const Vec mouth=in.head.p+math::Rotate(in.head.q,{0,-.1f,-.08f});
                if(!spent&&distance(in.hand[bottleHand].p,mouth)<.23f&&math::Rotate(in.hand[bottleHand].q,{0,1,0}).y<.25f) {
                    if(!drinkStart)drinkStart=in.time;
                    if(in.time-drinkStart>=.25&&request(Action::Potion,bottle,held.item,held.revision))spent=true;
                } else drinkStart=0;
            }
        }
        out.stowed=stowed;out.result=result;
        out.block=out.block||rearm||stowed||pending.action!=Action::None;
        for(int h=0;h<2;++h)out.owner[h]=rearm?Owner::Rearm:owner[h];
        return out;
    }
};
}
