#include "random_logic.h"
#include <cmath>
#include <cstdio>
#include <set>

#define CHECK(condition) do { if(!(condition)){std::fprintf(stderr,"check failed at line %d: %s\n",__LINE__,#condition);return __LINE__;} } while(false)
int main(){
    random_logic::Rng rng(12345);
    std::set<int> snapped;
    for(int i=0;i<500;i++){
        float a=random_logic::snapped_rotation(rng,false,90.0f);
        CHECK(a>=0&&a<360); CHECK(std::fmod(a,45.0f)==0); snapped.insert(static_cast<int>(a));
    }
    CHECK(snapped.size()==8);
    for(int i=0;i<500;i++){float a=random_logic::snapped_rotation(rng,true,90);CHECK(a>=0&&a<360);}
    random_logic::Rgba base{.2f,.4f,.8f,.7f};
    auto unchanged=random_logic::randomise_colour(rng,base,false,false);
    CHECK(std::fabs(unchanged.r-base.r)<.0001f&&std::fabs(unchanged.a-.7f)<.0001f);
    for(int i=0;i<500;i++){
        auto c=random_logic::randomise_colour(rng,base,true,true);
        CHECK(c.r>=0&&c.r<=1&&c.g>=0&&c.g<=1&&c.b>=0&&c.b<=1&&c.a==.7f);
        auto h=random_logic::rgb_to_hsv(c);CHECK(h.s>=.149f&&h.v>=.399f);
    }
    random_logic::Rng hueRng(98765);random_logic::HueBag hueBag;bool seen[12]{};
    for(int i=0;i<12;i++){float h=hueBag.next(hueRng);CHECK(h>=0&&h<1);int bin=static_cast<int>(h*12);CHECK(!seen[bin]);seen[bin]=true;}
    for(bool value:seen)CHECK(value);
    random_logic::Rng focusedRng(45678);
    constexpr float tau=6.2831853071795864769f;
    for(float centreHue:{0.0f,0.25f,0.5f,0.98f})for(float centreSaturation:{0.0f,0.4f,0.85f,1.0f})for(int i=0;i<1000;i++){
        auto focused=random_logic::focused_colour(focusedRng,centreHue,centreSaturation);CHECK(focused.h>=0&&focused.h<1);CHECK(focused.s>=0&&focused.s<=1);
        float centreX=centreSaturation*std::cos(centreHue*tau),centreY=centreSaturation*std::sin(centreHue*tau);
        float resultX=focused.s*std::cos(focused.h*tau),resultY=focused.s*std::sin(focused.h*tau);
        float dx=resultX-centreX,dy=resultY-centreY;CHECK(std::sqrt(dx*dx+dy*dy)<=.3001f);
    }
    for(float centre:{0.0f,0.15f,0.5f,0.85f,1.0f})for(int i=0;i<1000;i++){
        float v=random_logic::focused_brightness(focusedRng,centre);CHECK(v>=std::max(0.0f,centre-.15f)-.0001f);CHECK(v<=std::min(1.0f,centre+.15f)+.0001f);
    }
    std::puts("random_logic: all checks passed");
}
