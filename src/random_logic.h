#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>

namespace random_logic {

struct Rgba { float r, g, b, a; };
struct Hsv { float h, s, v; };

class Rng {
    std::uint64_t state_;
public:
    explicit Rng(std::uint64_t seed) : state_(seed ? seed : 0x9e3779b97f4a7c15ULL) {}
    std::uint64_t next() {
        std::uint64_t x = state_;
        x ^= x >> 12; x ^= x << 25; x ^= x >> 27;
        state_ = x;
        return x * 0x2545f4914f6cdd1dULL;
    }
    float unit() { return static_cast<float>((next() >> 40) * (1.0 / 16777216.0)); }
};

class HueBag {
    std::uint8_t bins_[12]{};
    unsigned index_ = 12;
public:
    float next(Rng& rng) {
        if (index_ >= 12) {
            for (unsigned i=0;i<12;i++) bins_[i]=static_cast<std::uint8_t>(i);
            for (unsigned i=11;i>0;i--) {
                const unsigned j=static_cast<unsigned>(rng.next()%(i+1));
                std::swap(bins_[i],bins_[j]);
            }
            index_=0;
        }
        return (static_cast<float>(bins_[index_++])+rng.unit())/12.0f;
    }
};

inline Hsv focused_colour(Rng& rng, float centreHue, float centreSaturation, float radius=0.30f) {
    if (!std::isfinite(centreHue)) centreHue = 0.0f;
    if (!std::isfinite(centreSaturation)) centreSaturation = 0.0f;
    if (!(radius > 0.0f) || !std::isfinite(radius)) radius = 0.30f;
    centreHue -= std::floor(centreHue);
    centreSaturation = std::clamp(centreSaturation,0.0f,1.0f);
    radius = std::min(radius,1.0f);
    constexpr float tau = 6.2831853071795864769f;
    const float centreAngle = centreHue*tau;
    const float centreX = centreSaturation*std::cos(centreAngle);
    const float centreY = centreSaturation*std::sin(centreAngle);
    // Rejection sampling keeps the result uniformly distributed inside the
    // requested circle while clipping only the portion outside the wheel.
    for(int attempt=0;attempt<64;attempt++){
        const float sampleRadius=std::sqrt(rng.unit())*radius;
        const float sampleAngle=rng.unit()*tau;
        const float x=centreX+sampleRadius*std::cos(sampleAngle);
        const float y=centreY+sampleRadius*std::sin(sampleAngle);
        const float saturation=std::sqrt(x*x+y*y);
        if(saturation<=1.0f){
            float hue=std::atan2(y,x)/tau;
            if(hue<0.0f)hue+=1.0f;
            return {hue,saturation,1.0f};
        }
    }
    return {centreHue,centreSaturation,1.0f};
}

inline float focused_brightness(Rng& rng, float centreBrightness, float radius=0.15f) {
    if (!std::isfinite(centreBrightness)) centreBrightness = 1.0f;
    if (!(radius >= 0.0f) || !std::isfinite(radius)) radius = 0.15f;
    centreBrightness = std::clamp(centreBrightness,0.0f,1.0f);
    const float low = std::max(0.0f,centreBrightness-radius);
    const float high = std::min(1.0f,centreBrightness+radius);
    return low+rng.unit()*(high-low);
}

inline float snapped_rotation(Rng& rng, bool freePlacement, float snapDegrees) {
    if (freePlacement) return rng.unit() * 360.0f;
    if (!(snapDegrees > 0.01f) || !std::isfinite(snapDegrees)) snapDegrees = 90.0f;
    // Double the stock snapped variety without making snapped placement fully free.
    snapDegrees = std::max(15.0f, snapDegrees * 0.5f);
    const int steps = std::max(1, static_cast<int>(std::floor(360.0f / snapDegrees + 0.0001f)));
    const int step = std::min(steps - 1, static_cast<int>(rng.unit() * steps));
    return step * snapDegrees;
}

inline Hsv rgb_to_hsv(Rgba c) {
    c.r = std::clamp(c.r, 0.0f, 1.0f); c.g = std::clamp(c.g, 0.0f, 1.0f); c.b = std::clamp(c.b, 0.0f, 1.0f);
    const float hi = std::max({c.r,c.g,c.b}), lo = std::min({c.r,c.g,c.b}), d = hi-lo;
    Hsv out{0.0f, hi <= 0.0f ? 0.0f : d/hi, hi};
    if (d <= 0.00001f) return out;
    if (hi == c.r) out.h = std::fmod((c.g-c.b)/d,6.0f);
    else if (hi == c.g) out.h = ((c.b-c.r)/d)+2.0f;
    else out.h = ((c.r-c.g)/d)+4.0f;
    out.h /= 6.0f; if (out.h < 0.0f) out.h += 1.0f;
    return out;
}

inline Rgba hsv_to_rgb(Hsv h, float alpha) {
    h.h -= std::floor(h.h); h.s=std::clamp(h.s,0.0f,1.0f); h.v=std::clamp(h.v,0.0f,1.0f);
    const float x=h.h*6.0f; const int sector=static_cast<int>(std::floor(x)); const float f=x-sector;
    const float p=h.v*(1-h.s), q=h.v*(1-h.s*f), t=h.v*(1-h.s*(1-f));
    switch(sector%6){
        case 0:return {h.v,t,p,alpha}; case 1:return {q,h.v,p,alpha}; case 2:return {p,h.v,t,alpha};
        case 3:return {p,q,h.v,alpha}; case 4:return {t,p,h.v,alpha}; default:return {h.v,p,q,alpha};
    }
}

inline Rgba randomise_colour(Rng& rng, Rgba input, bool colour, bool brightness, float selectedHue=-1.0f, float selectedBrightness=-1.0f, float selectedSaturation=-1.0f) {
    Hsv hsv=rgb_to_hsv(input);
    if(colour){hsv.h=selectedHue>=0.0f?selectedHue:rng.unit();hsv.s=selectedSaturation>=0.0f?std::clamp(selectedSaturation,0.0f,1.0f):0.30f+rng.unit()*0.70f;}
    if(brightness)hsv.v=selectedBrightness>=0.0f?focused_brightness(rng,selectedBrightness):0.40f+rng.unit()*0.60f;
    return hsv_to_rgb(hsv,input.a);
}

}
