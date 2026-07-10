// CoD2 IWI Texture Gallery - decode every stock texture to a browsable thumbnail
// grid with map-type + theme filters. No GPU: pure CPU decode -> downscale -> JPEG.
//
//   cod2-iwi-gallery [sources] [--outdir=./tex] [--maxdim=200] [--quality=82]
//   cod2-iwi-gallery [sources] <texturename>       # single texture -> image
//
// Sources (repeatable, combined; later wins):
//   --basepath=<CoD2 dir>   read main/iw_*.iwd (+ localized_*.iwd)
//   --loose=<dir>           extracted asset tree, or a parent of iw_* dirs
#include "vfs.h"
#include "image.h"
#include "categorize_tex.h"
#include "gallery_tex.h"
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>
#include <algorithm>
#include <filesystem>

namespace fs = std::filesystem;

static std::string baseName(std::string p)
{
    auto s = p.rfind('/'); if(s != std::string::npos) p = p.substr(s + 1);
    if(p.size() > 4 && p.substr(p.size() - 4) == ".iwi") p = p.substr(0, p.size() - 4);
    return p;
}

int main(int argc, char **argv)
{
    std::vector<std::string> looseRoots;
    std::string basepath, single, outdir = "tex";
    int maxdim = 200, quality = 82, limit = 0;
    bool batch = true;

    for(int i = 1; i < argc; i++) {
        std::string a = argv[i];
        if(a.rfind("--basepath=", 0) == 0)     basepath = a.substr(11);
        else if(a.rfind("--loose=", 0) == 0)   looseRoots.push_back(a.substr(8));
        else if(a.rfind("--outdir=", 0) == 0)  outdir = a.substr(9);
        else if(a.rfind("--maxdim=", 0) == 0)  maxdim = atoi(a.substr(9).c_str());
        else if(a.rfind("--quality=", 0) == 0) quality = atoi(a.substr(10).c_str());
        else if(a.rfind("--limit=", 0) == 0)   limit = atoi(a.substr(8).c_str());
        else if(a == "--batch")                batch = true;
        else { single = a; batch = false; }
    }
    maxdim = std::clamp(maxdim, 32, 1024);
    quality = std::clamp(quality, 1, 100);

    // ---- Build VFS (mirrors the xmodel tool's source handling) ----
    VFS vfs;
    auto looksLikeAssetRoot = [](const fs::path &p) {
        std::error_code ec;
        for(const char *sub : { "images", "xmodel", "materials" })
            if(fs::is_directory(p / sub, ec)) return true;
        return false;
    };
    auto addLoose = [&](const std::string &dir) {
        std::error_code ec;
        if(fs::is_directory(fs::path(dir) / "images", ec)) { vfs.addLooseRoot(dir); return; }
        std::vector<std::string> subs;
        for(auto &e : fs::directory_iterator(dir, ec))
            if(e.is_directory(ec) && looksLikeAssetRoot(e.path())) subs.push_back(e.path().string());
        std::sort(subs.begin(), subs.end());
        if(subs.empty()) vfs.addLooseRoot(dir);
        else for(const auto &s : subs) vfs.addLooseRoot(s);
    };
    for(const auto &r : looseRoots) addLoose(r);
    if(!basepath.empty()) {
        int added = 0;
        auto tryAdd = [&](const std::string &fn) {
            std::string p = basepath + "/main/" + fn; std::error_code ec;
            if(fs::exists(p, ec) && vfs.addIwd(p)) added++;
        };
        for(int i = 0; i <= 30; i++) { char fn[32]; snprintf(fn, sizeof fn, "iw_%02d.iwd", i); tryAdd(fn); }
        for(int i = 0; i <= 9; i++)  { char fn[48]; snprintf(fn, sizeof fn, "localized_english_iw%02d.iwd", i); tryAdd(fn); }
        printf("Loaded %d iwd archive(s) from %s/main\n", added, basepath.c_str());
    }
    if(vfs.sourceCount() == 0) { fprintf(stderr, "no asset sources; pass --loose= or --basepath=\n"); return 1; }

    std::error_code ec; fs::create_directories(outdir, ec);

    auto renderOne = [&](const std::string &texName, std::string &outFile, TexClass &tc, int &ow, int &oh) -> bool {
        std::vector<uint8_t> data;
        for(const char *ext : { ".iwi", "" }) { data = vfs.read("images/" + texName + ext); if(!data.empty()) break; }
        if(data.empty()) return false;
        Image img = decode_texture(data);
        if(!img.ok()) return false;
        std::vector<uint8_t> rgb; int w, h;
        make_thumbnail(img, maxdim, rgb, w, h);
        ow = img.w; oh = img.h;
        outFile = texName + ".jpg";
        for(char &c : outFile) if(c == '/' || c == '\\') c = '_';
        tc = classify_texture(texName);
        return write_image(outdir + "/" + outFile, rgb, w, h, quality);
    };

    if(!batch) {
        std::string of; TexClass tc; int ow, oh;
        std::string nm = baseName(single);
        if(renderOne(nm, of, tc, ow, oh)) { printf("Wrote %s/%s (%dx%d, %s)\n", outdir.c_str(), of.c_str(), ow, oh, tc.maptype.c_str()); return 0; }
        fprintf(stderr, "failed: %s\n", nm.c_str()); return 2;
    }

    // ---- Batch: every images/*.iwi ----
    std::vector<std::string> names;
    for(const auto &e : vfs.listPrefix("images/")) {
        if(e.size() < 5 || e.substr(e.size() - 4) != ".iwi") continue;
        names.push_back(baseName(e));
    }
    std::sort(names.begin(), names.end());
    names.erase(std::unique(names.begin(), names.end()), names.end());
    if(limit > 0 && (int)names.size() > limit) names.resize(limit);
    printf("Batch: %zu textures -> %s/\n", names.size(), outdir.c_str());

    std::vector<TexItem> items;
    int ok = 0, failed = 0;
    for(size_t i = 0; i < names.size(); i++) {
        std::string of; TexClass tc; int ow, oh;
        if(renderOne(names[i], of, tc, ow, oh)) {
            items.push_back({ names[i], of, tc.pretty, tc.maptype, tc.theme, ow, oh });
            ok++;
        } else failed++;
        if(i % 500 == 499 || i + 1 == names.size())
            printf("  %zu/%zu (ok=%d fail=%d)\n", i + 1, names.size(), ok, failed);
    }
    write_texture_gallery(outdir, items);
    printf("Done: %d textures, %d failed\n", ok, failed);
    return ok > 0 ? 0 : 2;
}
