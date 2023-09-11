#include <modules/tnm067lab1/utils/scalartocolormapping.h>

namespace inviwo {

void ScalarToColorMapping::clearColors() { baseColors_.clear(); }
void ScalarToColorMapping::addBaseColors(vec4 color) { baseColors_.push_back(color); }

vec4 ScalarToColorMapping::sample(float t) const {
    if (baseColors_.size() == 0) return vec4(t);  // Ingen färg vald
    if (baseColors_.size() == 1)
        return vec4(baseColors_[0]);  // Bara en färg vald -> vi behöver inte interpolera
    if (t <= 0) return vec4(baseColors_.front());  // t måste vara mellan 0-1
    if (t >= 1) return vec4(baseColors_.back());   // t måste vara mellan 0-1

    // TODO: use t to select which two base colors to interpolate in-between

    // baseColors definerad som std::vector<vec4>

    // 2*0.3=1 right
    // 2*0.3=0 left
    int right = ceil((baseColors_.size() - 1) * t);  // Högra base color
    int left = floor((baseColors_.size() - 1) * t);  // Vänstra base color

    // Normalisera t -> Ta fram nya intervallet som interpolation sker emellan och anpassa t så den matchar.
    // min=0
    // max = 0.5
    float min = left / float(baseColors_.size() - 1); // Minsta värdet i nya intervallet 
    float max = right / float(baseColors_.size() - 1); // Största värdet i nya intervallet 

    // 0.3/0.5 = 0.6
    // Normalisera t till det nya intervallet så den motsvarar proportionerligt samma vikt. 
    t = (t - min) / (max - min);

    // TODO: Interpolate colors in baseColors_ and set dummy color to result

    vec4 finalColor(
        vec4(baseColors_[left]).r + (vec4(baseColors_[right]).r - vec4(baseColors_[left]).r) * t,
        vec4(baseColors_[left]).g + (vec4(baseColors_[right]).g - vec4(baseColors_[left]).g) * t,
        vec4(baseColors_[left]).b + (vec4(baseColors_[right]).b - vec4(baseColors_[left]).b) * t,
        1);

    return finalColor;
}

}  // namespace inviwo
