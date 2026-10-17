#include <windows.h>
#include "interaction_ui.h"
#include "interaction_icons.h"
#include <algorithm>
#include <cstdio>

namespace framevr::interaction {
std::vector<uint8_t> PaintCompanions(const companions::Roster& roster) {
    using namespace companions;
    const int count=Count(roster);
    std::vector<uint8_t> rgba(Width*Height*4);
    if(!count)return rgba;
    BITMAPINFO info{};info.bmiHeader.biSize=sizeof(BITMAPINFOHEADER);
    info.bmiHeader.biWidth=Width;info.bmiHeader.biHeight=-Height;
    info.bmiHeader.biPlanes=1;info.bmiHeader.biBitCount=32;info.bmiHeader.biCompression=BI_RGB;
    void* bits=nullptr;HDC dc=CreateCompatibleDC(nullptr);
    HBITMAP bitmap=CreateDIBSection(dc,&info,DIB_RGB_COLORS,&bits,nullptr,0);
    if(!dc||!bitmap||!bits){if(bitmap)DeleteObject(bitmap);if(dc)DeleteDC(dc);return {};}
    auto old=SelectObject(dc,bitmap);std::memset(bits,0,rgba.size());
    auto fill=[&](RECT r,COLORREF colour){auto brush=CreateSolidBrush(colour);FillRect(dc,&r,brush);DeleteObject(brush);};
    fill({0,0,Width,UsedHeight(roster)},RGB(17,16,14));
    fill({0,0,Width,2},RGB(122,94,51));fill({0,UsedHeight(roster)-2,Width,UsedHeight(roster)},RGB(122,94,51));
    HFONT number=CreateFontW(46,0,0,0,FW_BOLD,FALSE,FALSE,FALSE,DEFAULT_CHARSET,
        OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,ANTIALIASED_QUALITY,DEFAULT_PITCH,L"Georgia");
    HFONT label=CreateFontW(count>=Columns?18:22,0,0,0,FW_SEMIBOLD,FALSE,FALSE,FALSE,DEFAULT_CHARSET,
        OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,ANTIALIASED_QUALITY,DEFAULT_PITCH,L"Georgia");
    const auto oldFont=SelectObject(dc,number);SetBkMode(dc,TRANSPARENT);
    int index=0;
    for(const auto& d:Definitions) {
        if(!(roster.relevant&Bit(d.type)))continue;
        const int row=index/Columns,column=index%Columns,cols=std::min(Columns,count-row*Columns);
        const int cell=Width/cols,x=column*cell,y=row*RowHeight;
        const int groupWidth=std::min(cell-12,128),left=x+(cell-groupWidth)/2,iconSize=std::min(64,groupWidth/2);
        const bool alive=roster.alive[d.type]>0;
        if(column)fill({x,y+12,x+1,y+RowHeight-12},RGB(64,50,31));
        auto image=d.skill>=0?icons::Get("s"+std::to_string(d.skill)):nullptr;
        if(image&&image->width&&image->height) {
            GdiFlush();auto* dst=static_cast<uint8_t*>(bits);
            const float scale=std::min(float(iconSize)/image->width,float(iconSize)/image->height);
            const int w=std::max(1,int(image->width*scale)),h=std::max(1,int(image->height*scale));
            const int ix=left+(iconSize-w)/2,iy=y+18+(64-h)/2;
            for(int j=0;j<h;++j)for(int i=0;i<w;++i) {
                const auto* p=&image->rgba[(std::min(image->height-1,unsigned(j/scale))*image->width+std::min(image->width-1,unsigned(i/scale)))*4];
                auto* out=&dst[((iy+j)*Width+ix+i)*4];const float alpha=p[3]/255.f;
                for(int c=0;c<3;++c)out[c]=uint8_t(p[2-c]*(alive?1.f:.45f)*alpha+out[c]*(1-alpha));
            }
        } else {
            // Text fallback also gives mercenaries a readable identity without
            // shipping game art or requiring a new extracted-icon cache.
            SelectObject(dc,label);SetTextColor(dc,RGB(190,163,110));
            const char* tag=d.type==7?"MERC":d.type==4?"SKEL":d.type==5?"MAGE":d.type==18?"TRAP":"ALLY";
            RECT box{left,y+20,left+iconSize,y+84};DrawTextA(dc,tag,-1,&box,DT_CENTER|DT_VCENTER|DT_SINGLELINE|DT_NOPREFIX);
        }
        SelectObject(dc,number);SetTextColor(dc,alive?RGB(245,232,198):RGB(228,124,92));
        char text[16];std::snprintf(text,sizeof text,"%u",unsigned(roster.alive[d.type]));
        RECT value{left+iconSize+3,y+13,x+cell-6,y+88};
        DrawTextA(dc,text,-1,&value,DT_CENTER|DT_VCENTER|DT_SINGLELINE|DT_NOPREFIX);
        SelectObject(dc,label);SetTextColor(dc,alive?RGB(222,206,174):RGB(175,142,124));
        RECT name{x+4,y+98,x+cell-4,y+132};DrawTextA(dc,d.name,-1,&name,DT_CENTER|DT_VCENTER|DT_SINGLELINE|DT_END_ELLIPSIS|DT_NOPREFIX);
        ++index;
    }
    GdiFlush();const auto* src=static_cast<const uint8_t*>(bits);
    for(int i=0;i<Width*UsedHeight(roster);++i) {
        rgba[i*4]=src[i*4+2];rgba[i*4+1]=src[i*4+1];rgba[i*4+2]=src[i*4];rgba[i*4+3]=255;
    }
    SelectObject(dc,oldFont);DeleteObject(number);DeleteObject(label);
    SelectObject(dc,old);DeleteObject(bitmap);DeleteDC(dc);return rgba;
}
}
