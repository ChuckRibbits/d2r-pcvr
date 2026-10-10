// Synthetic UI preview; does not load the game or its configuration.
#include "vr_settings_ui.h"
#include <cstdio>
#include <cstdlib>
int main(int argc,char** argv){
    using namespace framevr::settings;
    Snapshot s;s.ready=true;s.values[Motion]=1;s.values[Snap]=30;s.values[Foveation]=2;
    Model m;m.Open(s);const int page=argc>2?std::atoi(argv[2]):0;
    m.Click({Kind::Tab,page});
    const auto pixels=Paint(m,{},"Foveated shading available. Eye-tracking samples are active.");
    if(pixels.size()!=Width*Height*4)return 1;
    FILE* file=std::fopen(argc>1?argv[1]:"vr-settings-preview.ppm","wb");if(!file)return 2;
    std::fprintf(file,"P6\n%d %d\n255\n",Width,Height);
    for(size_t i=0;i<pixels.size();i+=4)if(std::fwrite(pixels.data()+i,1,3,file)!=3){std::fclose(file);return 3;}
    return std::fclose(file)==0?0:4;
}
