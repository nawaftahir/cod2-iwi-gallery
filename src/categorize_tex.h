// Texture classification for CoD2 IWI images.
//
// Primary axis is the MAP TYPE (Color, Normal, Specular, Decal, HUD, Loadscreen,
// System) inferred from CoD2's naming conventions: `-gggr` normal maps, `_spec`
// / `-rgb` specular, `~` auxiliary maps, `$` engine textures. Secondary axis is
// the same map/theme tag used by the model gallery.
#pragma once
#include <string>

struct TexClass {
    std::string maptype;  // never empty
    std::string theme;    // may be empty
    std::string pretty;
};

TexClass classify_texture(const std::string &rawName); // name may include .iwi
