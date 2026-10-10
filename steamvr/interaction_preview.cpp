// Standalone GDI atlas check. Uses synthetic state; never opens game data.
#include "interaction_ui.h"
#include <cstdio>
#include <cstring>
int main(int argc,char** argv) {
    using namespace framevr::interaction;
    Snapshot s;s.session=1;s.equipmentKnown=s.beltKnown=s.vitalsKnown=true;s.activeSet=1;
    s.health=74;s.maxHealth=100;s.mana=39;s.maxMana=80;
    const char* names[]={"Fire Bolt","Raise Skeleton","Corpse Explosion","Teleport","Frozen Armor","Bone Spear","Amplify Damage","Unassigned"};
    for(int i=0;i<8;++i){s.skills[i].known=true;s.skills[i].available=i!=7;std::snprintf(s.skills[i].name,64,"%s",names[i]);}
    for(int i=0;i<4;++i){auto& b=s.belt[i];b.known=true;b.count=i==3?0:4-i;b.items[0]={uint64_t(i+1),uint32_t(i),i==1?Code('m','p','2'):i==2?Code('r','v','s'):Code('h','p','3')};}
    s.sets[0][0]={1,1,Code('w','n','d')};s.sets[1][0]={2,2,Code('h','b','w')};
    Output o;o.hover=1;o.wheel=true;o.utility=argc>2;
    const auto pixels=Paint(s,o,2);if(pixels.size()!=1024*1024*4)return 1;
    FILE* file=std::fopen(argc>1?argv[1]:"interaction-preview.ppm","wb");if(!file)return 2;
    std::fprintf(file,"P6\n1024 1024\n255\n");
    for(size_t i=0;i<pixels.size();i+=4)if(std::fwrite(pixels.data()+i,1,3,file)!=3){std::fclose(file);return 3;}
    return std::fclose(file)==0?0:4;
}
