// VR Settings redesign: the layout (vr_settings_layout.h) over the real
// content tables: every key on one page, at most 8 rows for every class and
// dependency state, nothing overlapping or leaving the panel, VR-sized hit
// targets, and the model's presses with their guards.
#include "vr_settings_layout.h"
#include <cstdio>
#include <cstdlib>
#include <set>
#include <string>
using namespace framevr::settings;
namespace L=framevr::settings::layout;

static int checks=0;
static void Check(bool ok,const std::string& what){
    ++checks;
    if(!ok){std::fprintf(stderr,"FAIL: %s\n",what.c_str());std::exit(1);}
}
static bool Overlap(const Rect& a,const Rect& b){return a.x<b.x+b.w&&b.x<a.x+a.w&&a.y<b.y+b.h&&b.y<a.y+a.h;}
static bool Inside(const Rect& inner,const Rect& outer){return inner.x>=outer.x&&inner.y>=outer.y&&inner.x+inner.w<=outer.x+outer.w&&inner.y+inner.h<=outer.y+outer.h;}
static int Gap(const Rect& a,const Rect& b){
    const int dx=std::max({0,b.x-(a.x+a.w),a.x-(b.x+b.w)}),dy=std::max({0,b.y-(a.y+a.h),a.y-(b.y+b.h)});
    return std::max(dx,dy);
}
static bool Container(L::Role r){return r==L::Role::Row||r==L::Role::HelpCard||r==L::Role::ReadoutBox||r==L::Role::PresetCard;}
static bool CardPart(L::Role r){
    return r==L::Role::CardTitle||r==L::Role::CardText||r==L::Role::CardState||r==L::Role::CardButton||r==L::Role::PendingEntry||r==L::Role::ChangeLine;
}
static const char* ViewName(View v){return framevr::settings::content::PageOf(v).name;}

// Every geometric rule of one built panel.
static void CheckLayout(const L::Layout& layout,const std::string& where){
    const auto& e=layout.elements;
    Check(!e.empty(),where+": elements");
    for(size_t i=0;i<e.size();++i){
        const auto& a=e[i];
        const std::string name=where+" element "+std::to_string(i)+" ("+a.text+")";
        Check(a.rect.w>0&&a.rect.h>0,name+": has a size");
        Check(Inside(a.rect,L::PanelInner),name+": inside the panel");
        if(CardPart(a.role))Check(Inside(a.rect,layout.card),name+": inside the card");
        if(a.hit.kind!=Kind::None&&a.role!=L::Role::HelpCard){
            Check(a.rect.h>=L::MinTarget&&a.rect.w>=L::MinTarget,name+": hit target at least 64 x 64");
            if(a.Has(L::Enabled))Check(L::HitTest(layout,a.rect.x+a.rect.w/2.f,a.rect.y+a.rect.h/2.f)==a.hit,name+": the hit test finds it at its center");
        }
        if(Container(a.role))continue;
        for(size_t j=i+1;j<e.size();++j){
            const auto& b=e[j];
            if(Container(b.role))continue;
            Check(!Overlap(a.rect,b.rect),name+" overlaps element "+std::to_string(j)+" ("+b.text+")");
            if(a.hit.kind!=Kind::None&&b.hit.kind!=Kind::None)Check(Gap(a.rect,b.rect)>=L::MinGap,name+": at least 12 px from element "+std::to_string(j)+" ("+b.text+")");
        }
    }
    // Containers: rows and preset cards never overlap one another or the card.
    for(size_t i=0;i<e.size();++i)for(size_t j=i+1;j<e.size();++j){
        const auto& a=e[i];const auto& b=e[j];
        const bool ra=a.role==L::Role::Row||a.role==L::Role::PresetCard||a.role==L::Role::ReadoutBox||a.role==L::Role::HelpCard;
        const bool rb=b.role==L::Role::Row||b.role==L::Role::PresetCard||b.role==L::Role::ReadoutBox||b.role==L::Role::HelpCard;
        if(ra&&rb)Check(!Overlap(a.rect,b.rect),where+": containers apart ("+a.text+" / "+b.text+")");
    }
    // The help card's rectangle holds the HelpCard element.
    for(const auto& a:e)if(a.role==L::Role::HelpCard)Check(a.rect.x==layout.card.x&&a.rect.y==layout.card.y&&a.rect.w==layout.card.w&&a.rect.h==layout.card.h,where+": card rect");
}

static Snapshot Ready(const Values& v=Defaults()){Snapshot s;s.values=v;s.ready=true;s.offlineDifficultyAvailable=true;return s;}
static const L::Element* FindRole(const L::Layout& l,L::Role role,Key key=Count){
    for(const auto& e:l.elements)if(e.role==role&&(key==Count||e.key==key))return &e;
    return nullptr;
}
static int CountRole(const L::Layout& l,L::Role role){int n=0;for(const auto& e:l.elements)if(e.role==role)++n;return n;}

// vr-settings-layout-test <view> [card]: print that view's elements (title screen, defaults) and exit.
static int Dump(int view,int card){
    Model m;m.Open(Ready());L::Press(m,{Kind::Open,view,-1},0);
    L::LayoutInput in;in.card=card;in.runtimeStatus="Foveated shading available.";
    const auto l=L::Build(m,{},in);
    for(const auto& e:l.elements)std::printf("%2d %4d %4d %4d %4d hit%d/%d/%d f%03x %s | %s\n",int(e.role),e.rect.x,e.rect.y,e.rect.w,e.rect.h,int(e.hit.kind),e.hit.index,e.hit.delta,e.flags,e.text.c_str(),e.sub.c_str());
    std::printf("card %d %d %d %d\n",l.card.x,l.card.y,l.card.w,l.card.h);
    return 0;
}
int main(int argc,char** argv){
    if(argc>1)return Dump(std::atoi(argv[1]),argc>2?std::atoi(argv[2]):NoHelp);
    const auto& tables=L::ContentTables();
    // ---- every key on exactly one page ----
    {
        std::set<int> notInMenu;for(Key k:content::NotInMenu)notInMenu.insert(k);
        for(int k=0;k<Count;++k){
            int n=0;for(size_t i=0;i<tables.rowCount;++i)if(tables.rows[i].key==k)++n;
            Check(n==(notInMenu.count(k)?0:1),std::string("key on exactly one page: ")+Definitions[k].label);
        }
        for(size_t i=0;i<tables.rowCount;++i)Check(content::PageOf(tables.rows[i].page).settings,"rows live on settings pages");
    }
    // ---- at most 8 rows, every class, weapon and dependency state ----
    std::vector<Context> contexts;
    for(int c=-1;c<=6;++c)contexts.push_back({c});
    contexts.push_back({heroclass::Paladin,true});contexts.push_back({heroclass::Assassin,false,true});contexts.push_back({heroclass::Sorceress,false,false,true});
    contexts.push_back({heroclass::Barbarian,true,true,true});
    auto state=[](int bits){
        Values v=Defaults();
        v[SmoothTurn]=float(bits&1);v[Seated]=float((bits>>1)&1);v[TargetHighlight]=(bits>>2)&1?60.f:0.f;
        v[SpellLock]=(bits>>3)&1?3.f:0.f;v[Foveation]=float((bits>>4)&1);v[BloodVisor]=float((bits>>5)&1);v[DeferGpuWait]=float((bits>>6)&1);
        v[GroundAim]=float((bits>>7)%3);
        return v;
    };
    constexpr int States=128*3;
    for(const auto& c:contexts)for(int s=0;s<States;++s){
        const Values v=state(s);
        for(int view=0;view<int(View::Count);++view){
            const int n=L::CountRows(View(view),v,c);
            Check(n<=L::MaxViewRows,std::string("at most 8 rows: ")+ViewName(View(view)));
            if(content::PageOf(View(view)).settings)Check(n>=1,std::string("a settings page shows rows: ")+ViewName(View(view)));
        }
    }
    // Title screen, everything on: the fullest pages.
    {
        Values v=state(0b0100'0101);v[GroundAim]=2;
        Check(L::CountRows(View::Vibration,v,{})==8&&L::CountRows(View::Shield,v,{})==8,"Vibration and Shield hold 8 rows");
        Check(L::RowsOf(View::Targets,v,{}).count==5,"Targets: the outline, both hand colors as their own rows, and the arrow path line");
        bool right=false;const auto rows=L::RowsOf(View::Targets,v,{});
        for(int i=0;i<rows.count;++i)if(rows.keys[size_t(i)]==HighlightRight)right=true;
        Check(right&&RowShown(HighlightRight,v,{}),"the right hand's color has its own row");
        v[TargetHighlight]=0;Check(!RowShown(HighlightRight,v,{})&&!RowShown(HighlightLeft,v,{}),"and both hand colors go with the outline");
    }

    // ---- geometry: every view, class and state, with cards, drafts and footers ----
    int built=0;
    for(size_t ci=0;ci<contexts.size();++ci){
        const Context& c=contexts[ci];
        for(int s=0;s<States;s+=(ci<2?1:23)){
            Model m;Snapshot snap=Ready(state(s));m.Open(snap);m.SetContext(c);
            for(int view=0;view<int(View::Count);++view){
                L::Press(m,{Kind::Open,view,-1},0);
                const std::string where=std::string(ViewName(View(view)))+" class "+std::to_string(c.heroClass)+" state "+std::to_string(s);
                L::LayoutInput in;in.runtimeStatus="Foveated shading available. Eye tracking unavailable on this headset/runtime.";
                in.performance={true,true,90,11.1f,3.4f,2.6f,4.1f,.9f};in.game.vsync=s&1;in.monoAudio=(s%5)==0;
                const auto layout=L::Build(m,c,in);CheckLayout(layout,where);++built;
                // Point at each row: its card (buttons, hints) still fits.
                if(s%7==0)for(const auto& e:layout.elements)if(e.role==L::Role::Row){
                    L::LayoutInput pointed=in;pointed.card=e.key;CheckLayout(L::Build(m,c,pointed),where+" card "+Definitions[e.key].label);++built;
                }
            }
        }
    }
    // Drafts everywhere: pending cards, row cards with Undo / Default / Feel it, confirm, presets.
    {
        Model m;m.Open(Ready());
        for(int k=0;k<Count;++k){
            const auto* r=tables.Row(Key(k));
            if(!r||r->kind==content::RowKind::Button)continue;
            const auto& d=Definitions[k];
            L::Press(m,{Kind::Set,k,0,Normalize(Key(k),d.defaultValue==d.min?d.min+d.step*2:d.min)},0);
        }
        Check(m.Dirty(),"a draft on every page");
        for(int view=0;view<int(View::Count);++view){
            L::Press(m,{Kind::Open,view,-1},0);
            const std::string where=std::string("draft ")+ViewName(View(view));
            for(int saving=0;saving<2;++saving){
                L::LayoutInput in;
                const auto layout=L::Build(m,m.context,in);CheckLayout(layout,where);
                if(content::PageOf(View(view)).settings&&view!=int(View::Performance))Check(FindRole(layout,L::Role::PendingEntry)||FindRole(layout,L::Role::ChangeLine),where+": the idle card lists what is not applied");
                for(const auto& e:layout.elements){
                    if(e.role==L::Role::Row||e.role==L::Role::Tab||e.role==L::Role::PresetCard){
                        L::LayoutInput p;p.card=e.help;p.hover=e.hit;CheckLayout(L::Build(m,m.context,p),where+" card "+std::to_string(e.help));
                    }
                }
                for(int id:{HelpApply,HelpClose,HelpRecenter,HelpCalibrate,HelpMonoAudio,HelpPerformance,HelpResetPage,HelpUndoReset,HelpGuide,HelpWhatsNew,HelpPresets,HelpDiscard,HelpKeepEditing,HelpApplyClose,HelpPending}){
                    L::LayoutInput p;p.card=id;const auto pl=L::Build(m,m.context,p);CheckLayout(pl,where+" help "+std::to_string(id));
                    Check(FindRole(pl,L::Role::CardTitle)&&!FindRole(pl,L::Role::CardTitle)->text.empty(),where+": every help subject has a title");
                }
            }
        }
        // Status strip and confirm row on a full page.
        L::Press(m,{Kind::Open,int(View::Vibration),-1},0);
        L::Press(m,{Kind::Close},1000);
        Check(m.confirmClose,"Close with a draft asks first");
        const auto confirm=L::Build(m,m.context,{});CheckLayout(confirm,"confirm row");
        Check(FindRole(confirm,L::Role::ConfirmDiscard)&&FindRole(confirm,L::Role::ConfirmKeep)&&FindRole(confirm,L::Role::ConfirmApply)&&!FindRole(confirm,L::Role::FooterApply),"the footer turns into Discard / Keep editing / Apply and close");
        Check(FindRole(confirm,L::Role::CardTitle)->text=="Close without applying?","and the card asks");
        Request r=L::Press(m,{Kind::ApplyClose},1100);
        Check(r.sequence==0&&m.confirmClose,"the confirm row ignores presses for 300 ms");
        r=L::Press(m,{Kind::ApplyClose},1300);
        Check(r.sequence!=0&&!m.confirmClose&&m.closeWhenSaved&&m.open,"Apply and close saves first");
        const auto saving=L::Build(m,m.context,{});CheckLayout(saving,"saving");
        Check(FindRole(saving,L::Role::StatusStrip)&&FindRole(saving,L::Role::StatusStrip)->text=="Saving...","the save state shows above the footer");
        for(const auto& e:saving.elements)if(e.role==L::Role::StepPlus||e.role==L::Role::Chip||e.role==L::Role::Toggle)Check(!e.Has(L::Enabled),"no change while saving");
        Check(FindRole(saving,L::Role::FooterClose)->Has(L::Enabled)&&FindRole(saving,L::Role::Tab)->Has(L::Enabled),"tabs and Close still work while saving");
        Snapshot done;done.values=r.values;done.ready=true;done.sequence=r.sequence;done.result=Result::Saved;
        m.Observe(done);
        Check(!m.open&&!m.Dirty(),"and closes once saved");
    }

    // ---- presses ----
    {
        Model m;Snapshot snap=Ready();m.Open(snap);
        Check(m.view==View::Guide&&m.lastPage==View::Body,"the panel opens on the Controller guide");
        auto guide=L::Build(m,{},{});CheckLayout(guide,"guide");
        Check(CountRole(guide,L::Role::GuideLine)>=16,"the guide lists both hands");
        Check(!FindRole(guide,L::Role::FooterResetPage)&&!FindRole(guide,L::Role::FooterApply)&&FindRole(guide,L::Role::FooterClose),"the guide's footer: Close only");
        L::Press(m,{Kind::Open,int(View::Shield),-1},0);Check(m.view==View::Shield&&m.lastPage==View::Shield,"a tab opens its page");
        L::Press(m,{Kind::Open,int(View::Guide),-1},0);L::Press(m,{Kind::Open,int(View::Guide),-1},0);
        Check(m.view==View::Shield,"the Guide link again returns to the page");
        L::Press(m,{Kind::Open,int(View::Targets),TargetHighlight},0);Check(m.view==View::Targets&&m.focusKey==TargetHighlight,"Open points the card at its row");
        // Set and its rules.
        L::Press(m,{Kind::Set,ProjectileAimAssist,0,1},0);
        Check(m.Get()[ProjectileAimAssist]==1&&m.Applied()[ProjectileAimAssist]==2&&m.Dirty(),"Set changes the draft only");
        L::Press(m,{Kind::Undo,ProjectileAimAssist},0);Check(!m.Dirty(),"Undo: back to the applied value");
        L::Press(m,{Kind::Set,ShieldRoll,0,90},0);L::Press(m,{Kind::Default,ShieldRoll},0);Check(m.Get()[ShieldRoll]==0,"Default");
        L::Press(m,{Kind::Set,DeferGpuWait,0,1},0);Check(m.Get()[DeferGpuWait]==1,"Defer GPU wait: the offer turns it on");
        L::Press(m,{Kind::Set,DeferGpuWait,0,0},0);L::Press(m,{Kind::Default,DeferGpuWait},0);Check(m.Get()[DeferGpuWait]==1,"and nothing in the menu turns it off");
        L::Press(m,{Kind::Undo,DeferGpuWait},0);Check(m.Get()[DeferGpuWait]==0,"except Undo before Apply");
        L::Press(m,{Kind::Open,int(View::Performance),-1},0);
        {L::LayoutInput in;in.performance.valid=false;auto perf=L::Build(m,{},in);CheckLayout(perf,"perf");
         const auto* offer=FindRole(perf,L::Role::Button,DeferGpuWait);Check(offer&&offer->Has(L::Enabled),"the readout offers Defer GPU wait while it is Off");
         m.Set(DeferGpuWait,1);perf=L::Build(m,{},in);Check(!FindRole(perf,L::Role::Button,DeferGpuWait),"and not once it is On");m.Set(DeferGpuWait,0);}
        // Lists wrap both ways: a cycler or color list goes round; numbers stop at their ends.
        m.Set(ControllerLabels,4);L::Press(m,{Kind::Change,ControllerLabels,1},0);Check(m.Get()[ControllerLabels]==0,"a cycler wraps from its last option to the first");
        L::Press(m,{Kind::Change,ControllerLabels,-1},0);Check(m.Get()[ControllerLabels]==4,"and from the first to the last");
        m.Set(TargetHighlight,60);m.Set(HighlightRight,360);L::Press(m,{Kind::Change,HighlightRight,1},0);Check(m.Get()[HighlightRight]==0,"a hand color wraps from its last hue to By range");
        L::Press(m,{Kind::Change,HighlightRight,-1},0);Check(m.Get()[HighlightRight]==360,"and back");
        m.Set(BowLaser,345);L::Press(m,{Kind::Change,BowLaser,1},0);Check(m.Get()[BowLaser]==LaserOff,"the arrow path line wraps to Off");
        L::Press(m,{Kind::Change,BowLaser,-1},0);Check(m.Get()[BowLaser]==345,"and back from Off");
        m.Set(ShieldRoll,180);L::Press(m,{Kind::Change,ShieldRoll,1},0);Check(m.Get()[ShieldRoll]==180,"a number stops at its end");
        m.Set(ShieldRoll,-180);L::Press(m,{Kind::Change,ShieldRoll,-1},0);Check(m.Get()[ShieldRoll]==-180,"at both ends");
        for(size_t i=0;i<tables.rowCount;++i){   // every list row wraps, every number stops
            const auto& r=tables.rows[i];using content::RowKind;
            const bool list=r.kind==RowKind::Cycler||r.kind==RowKind::Choice||r.kind==RowKind::Swatch;
            const bool number=r.kind==RowKind::Stepper||r.kind==RowKind::Slider;
            if(list)Check(L::WrapsRound(r),std::string("a list wraps: ")+r.label);
            if(number)Check(!L::WrapsRound(r),std::string("a number stops: ")+r.label);
        }
        L::Press(m,{Kind::Change,MeleeThrustSpeed,0},0);Check(m.Get()[MeleeThrustSpeed]==2,"a drag of zero steps changes nothing");
        L::Press(m,{Kind::Change,MeleeThrustSpeed,4},0);Check(m.Get()[MeleeThrustSpeed]==3,"a drag of n steps");
        m.Discard();Check(!m.Dirty(),"Discard drops the draft");
    }
    {   // Spells go to the aimed monster: a Paladin keeps the saved bow bit.
        Values v=Defaults();v[SpellLock]=2;Model m;m.Open(Ready(v));m.SetContext({heroclass::Paladin});
        L::Press(m,{Kind::Open,int(View::Aim),-1},0);
        const auto aim=L::Build(m,m.context,{});CheckLayout(aim,"aim paladin");
        int chips=0;const L::Element* on=nullptr;
        for(const auto& e:aim.elements)if(e.role==L::Role::Chip&&e.key==SpellLock){++chips;if(e.text=="On")on=&e;}
        Check(chips==2&&on,"Off / On for a class without bow skills");
        L::Press(m,on->hit,0);Check(m.Get()[SpellLock]==3,"On sets spells and keeps bow skills");
        L::Press(m,{Kind::Default,SpellLock},0);Check(m.Get()[SpellLock]==2,"Default turns spells off, bow skills kept");
        // Presets never touch the owner's keys, calibration or rows hidden for the class.
        m.Set(StereoRendering,1);m.Set(CompanionAi,1);m.Set(ElementalHands,0);
        for(size_t p=0;p<tables.presetCount;++p){
            L::Press(m,{Kind::Preset,int(p)},0);
            Check(m.Get()[StereoRendering]==1&&m.Get()[CompanionAi]==1&&m.Get()[ElementalHands]==0&&m.Get()[DeferGpuWait]==0&&m.Get()[EyeHeight]==m.Applied()[EyeHeight]&&m.Get()[ArmSpan]==m.Applied()[ArmSpan],
                  std::string("preset leaves the owner's keys and calibration: ")+tables.presets[p].name);
            Check(m.Get()[GroundAim]==0&&m.Get()[LandingCircles]==Defaults()[LandingCircles]&&m.Get()[BowLaser]==Defaults()[BowLaser]&&m.Get()[StaffPoint]==Defaults()[StaffPoint],
                  std::string("and the Paladin's hidden rows: ")+tables.presets[p].name);
            Check((int(m.Get()[SpellLock])&2)==2,"and the saved bow-skills bit");
            Check(m.pickedPreset==int(p),"the picked preset is remembered");
        }
        L::Press(m,{Kind::Open,int(View::Presets),-1},0);
        for(int p=0;p<int(tables.presetCount);++p){L::LayoutInput in;in.card=HelpPresetFirst+p;CheckLayout(L::Build(m,m.context,in),"preset card");}
    }
    {   // Reset page and Undo reset, with their guards.
        Model m;m.Open(Ready());L::Press(m,{Kind::Open,int(View::Shield),-1},0);
        auto shield=L::Build(m,{},{});Check(!FindRole(shield,L::Role::FooterResetPage)->Has(L::Enabled),"Reset page is off while every row is at its default");
        m.Set(ShieldRoll,90);m.Set(ShieldGrip,10);m.Set(HapticMelee,50);
        shield=L::Build(m,{},{});Check(FindRole(shield,L::Role::FooterResetPage)->Has(L::Enabled),"and on once one differs");
        L::Press(m,{Kind::ResetPage},10000);
        Check(m.Get()[ShieldRoll]==0&&m.Get()[ShieldGrip]==0&&m.Get()[HapticMelee]==50,"Reset page resets this page's rows only");
        L::LayoutInput in;in.nowMs=10100;auto undo=L::Build(m,{},in);CheckLayout(undo,"undo reset");
        const auto* slot=FindRole(undo,L::Role::FooterResetPage);
        Check(slot&&slot->hit.kind==Kind::UndoReset&&!slot->Has(L::Enabled)&&FindRole(undo,L::Role::DrainBar),"the slot reads Undo reset with its bar, not yet armed");
        L::Press(m,{Kind::UndoReset},10300);Check(m.Get()[ShieldRoll]==0,"a double press never lands on Undo reset (400 ms)");
        L::Press(m,{Kind::UndoReset},10500);Check(m.Get()[ShieldRoll]==90&&m.Get()[ShieldGrip]==10,"Undo reset puts the rows back");
        L::Press(m,{Kind::ResetPage},20000);L::Press(m,{Kind::UndoReset},26000);Check(m.Get()[ShieldRoll]==0,"Undo reset is gone after 6 s");
        in.nowMs=26000;Check(FindRole(L::Build(m,{},in),L::Role::FooterResetPage)->hit.kind==Kind::ResetPage,"and the slot reads Reset page again");
        // Confirm close: discard, keep editing, Right B.
        L::Press(m,{Kind::Close},30000);Check(m.confirmClose&&m.open,"Close asks");
        L::Press(m,{Kind::KeepEditing},30100);Check(m.confirmClose,"Keep editing waits 300 ms");
        L::Press(m,{Kind::Close},30400);Check(!m.confirmClose&&m.open&&m.Dirty(),"Close again (Right B) keeps editing");
        L::Press(m,{Kind::Close},31000);L::Press(m,{Kind::ConfirmClose},31100);Check(m.open,"Discard waits 300 ms");
        L::Press(m,{Kind::ConfirmClose},31400);Check(!m.open&&!m.Dirty(),"Discard drops the draft and closes");
        // Open resets the redesign's state.
        m.Open(Ready());m.view=View::World;m.confirmClose=true;m.undoResetUntil=9;m.focusKey=3;m.pickedPreset=1;m.closeWhenSaved=true;m.resetKeys=ChangeMask{1};
        m.Open(Ready());
        Check(m.view==View::Guide&&!m.confirmClose&&m.undoResetUntil==0&&m.focusKey==-1&&m.pickedPreset==-1&&!m.closeWhenSaved&&!m.resetKeys,"Open resets every new field");
        L::Press(m,{Kind::Close},0);Check(!m.open,"Close without a draft closes at once");
    }
    {   // Calibrate is off while a draft waits; offline difficulty locks without its adapter.
        Model m;Snapshot s=Ready();s.offlineDifficultyAvailable=false;m.Open(s);
        L::Press(m,{Kind::Open,int(View::Body),-1},0);
        auto body=L::Build(m,{},{});CheckLayout(body,"body");
        const L::Element* calibrate=nullptr;for(const auto& e:body.elements)if(e.hit.kind==Kind::Calibrate)calibrate=&e;
        Check(calibrate&&calibrate->Has(L::Enabled),"Calibrate body with no draft");
        m.Set(Dominant,1);body=L::Build(m,{},{});
        for(const auto& e:body.elements)if(e.hit.kind==Kind::Calibrate)calibrate=&e;
        Check(!calibrate->Has(L::Enabled)&&FindRole(body,L::Role::Label,BodyCalibration)->sub=="Apply or discard first","Calibrate is disabled while Dirty, with the reason");
        L::Press(m,{Kind::Open,int(View::Survival),-1},0);
        // As main: without the adapter the row shows the notice and stays editable (never locked).
        m.Discard();
        const auto survival=L::Build(m,{},{});CheckLayout(survival,"survival without the adapter");
        const auto* offline=FindRole(survival,L::Role::Label,OfflinePlayers);
        Check(!offline->Has(L::Locked)&&offline->sub==content::OfflineUnavailableShort,"offline difficulty shows main's notice, not a lock");
        L::LayoutInput card;card.card=OfflinePlayers;const auto pointed=L::Build(m,{},card);
        const auto* text=FindRole(pointed,L::Role::CardText);
        Check(text&&text->text.rfind(content::OfflineUnavailable,0)==0&&text->text.find(content::OfflineNote)==std::string::npos,"its card leads with the notice instead of the difficulty note");
        Check(!(FindRole(pointed,L::Role::CardTitle)->flags&L::Locked),"and is not locked");
        L::Press(m,{Kind::Change,OfflinePlayers,1},0);Check(m.Get()[OfflinePlayers]==1,"it steps without the adapter");
        m.offlineDifficultyAvailable=true;L::Press(m,{Kind::Change,OfflinePlayers,1},0);Check(m.Get()[OfflinePlayers]==2,"and with it");
    }
    {   // Calibration view, What's new, mono audio and not-ready.
        Model m;m.Open(Ready());
        L::LayoutInput in;in.calibration.active=true;in.calibration.progress=.5f;
        const auto calib=L::Build(m,{},in);CheckLayout(calib,"calibration");
        Check(!FindRole(calib,L::Role::Tab)&&L::HitTest(calib,640,512).kind==Kind::None,"calibration replaces the panel and blocks presses");
        static const char* const lines[]={"More natural shield wrists","Guard strain edge: Pale glow, Dark edge or Off","Grab a menu with right grip to move it","Performance settings in one place",
            "Menus and wrist maps fixed at high resolutions","Cleaner One by turns view","Smoother frames","Scenery distance now starts at 210"};
        L::LayoutInput notes;notes.notes={"D2R VR Unleashed / v0.3.24","A mod by monkeylashes","https://github.com/bearstonem/d2r-pcvr","beadlebytes; ChuckRibbits; Sergious.",lines,8};
        L::Press(m,{Kind::Open,int(View::WhatsNew),-1},0);
        const auto news=L::Build(m,{},notes);CheckLayout(news,"what's new");Check(CountRole(news,L::Role::NotesText)>=10,"What's new lists its lines");
        Snapshot off=Ready();off.ready=false;m.Open(off);L::Press(m,{Kind::Open,int(View::World),-1},0);
        L::LayoutInput mono;mono.monoAudio=true;
        const auto world=L::Build(m,{},mono);CheckLayout(world,"not ready");
        Check(FindRole(world,L::Role::Warning)&&FindRole(world,L::Role::StatusStrip),"mono warning and the unavailable strip");
        L::Press(m,{Kind::Set,Sky,0,1},0);Check(m.Get()[Sky]==0,"no edits while not ready");
        Check(L::HelpAt(world,FindRole(world,L::Role::Warning)->rect.x+5.f,FindRole(world,L::Role::Warning)->rect.y+5.f)==HelpMonoAudio,"pointing at the warning explains it");
    }
    std::printf("VR settings layout tests passed: %d checks, %d panels built\n",checks,built);
}
