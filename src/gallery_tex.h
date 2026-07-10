// Texture thumbnail output + the searchable IWI gallery page.
#pragma once
#include <cstdint>
#include <string>
#include <vector>

struct Image; // from image.h

struct TexItem {
    std::string name;     // raw texture name (with .iwi stripped)
    std::string file;     // thumbnail filename relative to gallery dir
    std::string pretty;
    std::string maptype;  // Color / Normal / Specular / ...
    std::string theme;    // may be empty
    int w = 0, h = 0;     // original texture dimensions
};

// Composite alpha over a checkerboard and box-downscale to fit maxDim.
// Produces top-down RGB and the output dimensions.
void make_thumbnail(const Image &img, int maxDim, std::vector<uint8_t> &rgb, int &outW, int &outH);

bool write_image(const std::string &path, const std::vector<uint8_t> &rgb, int w, int h, int jpegQuality);
void write_texture_gallery(const std::string &outdir, const std::vector<TexItem> &items);
