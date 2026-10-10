#include <windows.h>
#include "interaction_ui.h"
#include <cstdio>
#include <string>
namespace framevr::interaction {
std::vector<uint8_t> Paint(const Snapshot& s,const Output& o,int selection) {
    constexpr int size=1024;
    BITMAPINFO info{};info.bmiHeader.biSize=sizeof(BITMAPINFOHEADER);
    info.bmiHeader.biWidth=size;info.bmiHeader.biHeight=-size;info.bmiHeader.biPlanes=1;info.bmiHeader.biBitCount=32;
    HDC dc=CreateCompatibleDC(nullptr);void* bits=nullptr;
    HBITMAP bitmap=CreateDIBSection(dc,&info,DIB_RGB_COLORS,&bits,nullptr,0);
    if(!dc||!bitmap||!bits){if(bitmap)DeleteObject(bitmap);if(dc)DeleteDC(dc);return {};}
    auto old=SelectObject(dc,bitmap);std::memset(bits,0,size*size*4);
    HFONT font=CreateFontW(23,0,0,0,FW_SEMIBOLD,FALSE,FALSE,FALSE,DEFAULT_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,ANTIALIASED_QUALITY,DEFAULT_PITCH,L"Arial");
    auto oldFont=SelectObject(dc,font);SetBkMode(dc,TRANSPARENT);SetTextColor(dc,RGB(238,235,220));
    auto fill=[&](RECT r,COLORREF c){auto b=CreateSolidBrush(c);FillRect(dc,&r,b);DeleteObject(b);};
    auto text=[&](const char* t,RECT r){wchar_t w[256]{};MultiByteToWideChar(CP_UTF8,0,t,-1,w,256);DrawTextW(dc,w,-1,&r,DT_CENTER|DT_VCENTER|DT_WORDBREAK|DT_NOPREFIX);};
    auto polygon=[&](POINT* p,int n,COLORREF c){auto b=CreateSolidBrush(c);auto was=SelectObject(dc,b);auto pen=SelectObject(dc,GetStockObject(NULL_PEN));Polygon(dc,p,n);SelectObject(dc,pen);SelectObject(dc,was);DeleteObject(b);};
    const char* fallback[]={"B","X","Y","RB","LT + B","LT + X","LT + Y","LT + RB"};
    for(int i=0;i<8;++i) {
        POINT points[34];int n=0;
        const float angle=i*Pi/4;
        for(int k=0;k<=16;++k){float a=angle-Pi/8+.012f+(Pi/4-.024f)*k/16;points[n++]={LONG(384+std::sin(a)*308),LONG(345.6f-std::cos(a)*308)};}
        for(int k=16;k>=0;--k){float a=angle-Pi/8+.012f+(Pi/4-.024f)*k/16;points[n++]={LONG(384+std::sin(a)*110),LONG(345.6f-std::cos(a)*110)};}
        bool available=!s.skills[i].known||s.skills[i].available;
        polygon(points,n,o.hover==i?RGB(91,100,72):!o.utility&&selection==i?RGB(41,87,93):!o.utility&&!available?RGB(28,28,30):RGB(28,40,46));
        char label[120]{};
        if(!o.utility)std::snprintf(label,sizeof label,"%s%s",s.skills[i].known?s.skills[i].name:fallback[i],s.skills[i].known?"":"\nNative slot");
        else if(i<4){const char* utility[]={"Weapon I","Weapon II","Journal",o.stowed?"Draw weapons":"Stow weapons"};std::snprintf(label,sizeof label,"%s",utility[i]);}
        else {const auto& b=s.belt[i-4];std::snprintf(label,sizeof label,"%s\n%s%d",b.known?(b.count?PotionName(PotionType(b.Next().code)):"Empty"):"Belt unknown",b.known?"x":"? ",b.known?b.count:0);}
        const int x=int(384+std::sin(angle)*215),y=int(345.6f-std::cos(angle)*215);
        text(label,{x-91,y-39,x+91,y+48});
    }
    fill({315,300,453,392},o.hover==8?RGB(90,90,65):RGB(39,46,42));text("WEAPON",{315,312,453,380});
    fill({46,692,538,762},o.hover==9?RGB(77,85,68):RGB(29,44,49));text(o.utility?"SPELLS":"EQUIPMENT / BELT",{46,706,538,753});
    fill({553,692,753,762},o.hover==10?RGB(77,85,68):RGB(29,44,49));text("VR SETTINGS",{553,706,753,753});
    const COLORREF colours[]={RGB(90,90,90),RGB(169,37,45),RGB(43,82,193),RGB(131,55,165),RGB(67,151,66),RGB(183,191,205),RGB(218,165,70)};
    for(int i=0;i<4;++i) {
        const auto& b=s.belt[i];const int x=i*128;
        fill({x+42,774,x+86,814},RGB(160,135,83));
        fill({x+18,814,x+110,940},b.known&&b.count?colours[unsigned(PotionType(b.Next().code))]:RGB(45,47,47));
        char label[60];std::snprintf(label,sizeof label,"%s\nx%d",b.known?(b.count?PotionName(PotionType(b.Next().code)):"Empty"):"Unknown",b.count);
        text(label,{x,943,x+128,1024});
    }
    // A grabbed bottle has its own tile. Native refill must not recolor it to
    // the next belt item; only confirmed consumption makes this preview empty.
    if(o.bottleItem.handle) {
        const auto kind=PotionType(o.bottleItem.code);
        fill({810,520,866,554},RGB(160,135,83));
        fill({790,554,886,693},o.bottleEmpty?RGB(45,47,47):colours[unsigned(kind)]);
        text(o.bottleEmpty?"Used":PotionName(kind),{768,703,896,768});
    }
    for(int i=0;i<2;++i){const int y=i*256;fill({774,y+8,1018,y+248},o.nearHolster==i?RGB(74,83,60):RGB(35,30,25));
        char label[120];const auto& a=s.sets[i][0];const auto& b=s.sets[i][1];
        char ca[5]{},cb[5]{};std::memcpy(ca,&a.code,4);std::memcpy(cb,&b.code,4);
        std::snprintf(label,sizeof label,"SET %s%s\n%s / %s\nGrip + pull",i==0?"I":"II",s.activeSet==uint32_t(i+1)?" (active)":"",a.handle?ca:"empty",b.handle?cb:"empty");
        text(label,{774,y+50,1018,y+230});}
    fill({512,776,1024,1024},RGB(20,28,33));
    if(s.vitalsKnown){
        auto gauge=[&](float value,float max,int y,COLORREF c,const char* name){
            fill({536,y,1000,y+32},RGB(50,50,50));
            if(max>0)fill({536,y,536+LONG(464*std::clamp(value/max,0.f,1.f)),y+32},c);
            char label[64];std::snprintf(label,sizeof label,"%s %.0f / %.0f",name,value,max);text(label,{536,y-31,1000,y});};
        gauge(s.health,s.maxHealth,842,RGB(180,35,42),"Health");gauge(s.mana,s.maxMana,908,RGB(48,83,185),"Mana");
    } else text("Status unavailable",{530,800,1006,885});
    const char* status=o.result==Result::Pending?"Waiting for game...":o.result==Result::Applied?"Confirmed":o.result==Result::Refused?"Action not available":o.stowed?"Weapons stowed":"Left Y: spells / equipment";
    text(status,{530,955,1006,1017});
    GdiFlush();std::vector<uint8_t> rgba(size*size*4);const auto* b=static_cast<const uint8_t*>(bits);
    for(int i=0;i<size*size;++i){rgba[4*i]=b[4*i+2];rgba[4*i+1]=b[4*i+1];rgba[4*i+2]=b[4*i];rgba[4*i+3]=(b[4*i]||b[4*i+1]||b[4*i+2])?255:0;}
    SelectObject(dc,oldFont);DeleteObject(font);SelectObject(dc,old);DeleteObject(bitmap);DeleteDC(dc);return rgba;
}
}
