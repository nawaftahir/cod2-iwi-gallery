#include "gallery_tex.h"
#include "image.h"
#include <cstdio>
#include <fstream>
#include <algorithm>
#include <map>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "../third_party/stb_image_write.h"

static std::string lower(std::string s){ for(char &c:s) if(c>='A'&&c<='Z') c+=32; return s; }

bool write_image(const std::string &path, const std::vector<uint8_t> &rgb, int w, int h, int jpegQuality)
{
    std::string ext = lower(path.size() > 4 ? path.substr(path.size() - 4) : "");
    if(ext == ".png") return stbi_write_png(path.c_str(), w, h, 3, rgb.data(), w * 3) != 0;
    return stbi_write_jpg(path.c_str(), w, h, 3, rgb.data(), jpegQuality) != 0;
}

// Box-downscale the RGBA image to fit maxDim (never upscales), then composite the
// downscaled alpha over a checkerboard so transparent textures read as such.
void make_thumbnail(const Image &img, int maxDim, std::vector<uint8_t> &out, int &outW, int &outH)
{
    int w = img.w, h = img.h;
    int longest = std::max(w, h);
    float scale = (longest > maxDim) ? (float)maxDim / longest : 1.f;
    outW = std::max(1, (int)(w * scale));
    outH = std::max(1, (int)(h * scale));

    // Downscale RGBA (colour + alpha) by area averaging.
    std::vector<uint8_t> small((size_t)outW * outH * 4);
    for(int y = 0; y < outH; y++) {
        int sy0 = y * h / outH, sy1 = std::max(sy0 + 1, (y + 1) * h / outH);
        for(int x = 0; x < outW; x++) {
            int sx0 = x * w / outW, sx1 = std::max(sx0 + 1, (x + 1) * w / outW);
            int r = 0, g = 0, b = 0, a = 0, n = 0;
            for(int sy = sy0; sy < sy1; sy++) for(int sx = sx0; sx < sx1; sx++) {
                size_t si = ((size_t)sy * w + sx) * 4;
                r += img.rgba[si]; g += img.rgba[si+1]; b += img.rgba[si+2]; a += img.rgba[si+3]; n++;
            }
            size_t di = ((size_t)y * outW + x) * 4;
            small[di]=r/n; small[di+1]=g/n; small[di+2]=b/n; small[di+3]=a/n;
        }
    }

    // Composite over a 10px checkerboard.
    out.assign((size_t)outW * outH * 3, 0);
    for(int y = 0; y < outH; y++) for(int x = 0; x < outW; x++) {
        bool c = ((x / 10) ^ (y / 10)) & 1;
        int bg = c ? 60 : 42;
        size_t si = ((size_t)y * outW + x) * 4, di = ((size_t)y * outW + x) * 3;
        float a = small[si+3] / 255.f;
        for(int k = 0; k < 3; k++)
            out[di + k] = (uint8_t)(small[si + k] * a + bg * (1.f - a));
    }
}

static std::string esc(const std::string &s)
{
    std::string o; o.reserve(s.size() + 8);
    for(char c : s) switch(c) {
        case '&': o += "&amp;"; break; case '<': o += "&lt;"; break; case '>': o += "&gt;"; break;
        case '"': o += "&quot;"; break; case '\'': o += "&#39;"; break; default: o += c;
    }
    return o;
}

static const std::vector<std::pair<std::string,std::string>> TYPE_COLOR = {
    {"Color","#c9a24b"}, {"Normal","#7f86d6"}, {"Specular","#77b0bd"}, {"Decal/Detail","#c98a5a"},
    {"HUD/UI","#7fb27f"}, {"Loadscreen/UI","#b98ac4"}, {"System","#6f7782"},
};

static const char *CSS = R"CSS(
:root{--bg:#14161a;--card:#22262e;--tile:#0e1013;--edge:#313742;--fg:#e9e5d9;--muted:#8b93a1;
--accent:#c9a24b;--accent-dim:#8a6f30;color-scheme:dark}
:root[data-theme="light"]{--bg:#e7e3d7;--card:#f6f3ea;--tile:#ded9cb;--edge:#cdc6b4;--fg:#23221c;
--muted:#6e6a5c;--accent:#9a7420;--accent-dim:#b79240;color-scheme:light}
@media (prefers-color-scheme:light){:root:not([data-theme="dark"]){--bg:#e7e3d7;--card:#f6f3ea;
--tile:#ded9cb;--edge:#cdc6b4;--fg:#23221c;--muted:#6e6a5c;--accent:#9a7420;--accent-dim:#b79240;color-scheme:light}}
*{box-sizing:border-box}body{margin:0;background:var(--bg);color:var(--fg);font-family:ui-sans-serif,system-ui,"Segoe UI",Roboto,sans-serif}
.wrap{max-width:1400px;margin:0 auto;padding:0 22px 60px}
header{position:sticky;top:0;z-index:5;background:color-mix(in oklab,var(--bg) 90%,transparent);backdrop-filter:blur(10px);border-bottom:1px solid var(--edge)}
.head-in{max-width:1400px;margin:0 auto;padding:18px 22px 14px}
.eyebrow{font:600 11px/1 ui-monospace,Menlo,monospace;letter-spacing:.22em;text-transform:uppercase;color:var(--accent);margin:0 0 7px}
h1{margin:0;font-size:24px;font-weight:700}h1 b{color:var(--accent)}
.sub{margin:6px 0 0;color:var(--muted);font-size:13px}
.controls{display:flex;gap:12px;align-items:center;flex-wrap:wrap;margin-top:14px}
.search{position:relative;flex:1;min-width:220px}
.search input{width:100%;padding:11px 14px 11px 38px;border:1px solid var(--edge);border-radius:9px;background:var(--tile);color:var(--fg);font:14px/1.2 ui-monospace,Menlo,monospace}
.search input:focus{outline:2px solid var(--accent-dim);outline-offset:1px}
.search svg{position:absolute;left:12px;top:50%;transform:translateY(-50%);color:var(--muted)}
.count{font:600 12.5px/1 ui-monospace,monospace;color:var(--muted);font-variant-numeric:tabular-nums;white-space:nowrap}
.count b{color:var(--fg)}
.rowlabel{font:600 10px/1 ui-monospace,monospace;letter-spacing:.14em;text-transform:uppercase;color:var(--muted);margin:14px 2px 8px}
.filters{display:flex;gap:7px;flex-wrap:wrap}
.filter{font:600 12px/1 ui-monospace,monospace;color:var(--muted);background:transparent;border:1px solid var(--edge);border-radius:999px;padding:7px 11px;cursor:pointer;transition:.14s;display:inline-flex;gap:6px;align-items:center}
.filter:hover{color:var(--fg);border-color:var(--muted)}.filter .n{font-size:11px;opacity:.7;font-variant-numeric:tabular-nums}
.filter.on{color:var(--bg);background:var(--accent);border-color:var(--accent)}.filter.on .n{opacity:.85}
.grid{display:grid;grid-template-columns:repeat(auto-fill,minmax(150px,1fr));gap:12px;margin-top:20px}
.card{background:var(--card);border:1px solid var(--edge);border-radius:10px;overflow:hidden;transition:transform .14s,border-color .14s}
.card:hover{transform:translateY(-2px);border-color:var(--accent-dim)}
.thumb{aspect-ratio:1/1;background:var(--tile);display:flex;align-items:center;justify-content:center}
.thumb img{max-width:100%;max-height:100%;image-rendering:auto}
.meta{padding:8px 9px 9px}.slug{font:11px/1.3 ui-monospace,Menlo,monospace;word-break:break-all}
.chips{display:flex;gap:4px;flex-wrap:wrap;margin-top:6px}
.chip{font:600 9px/1 ui-monospace,monospace;letter-spacing:.02em;text-transform:uppercase;padding:3px 6px;border-radius:4px;border:1px solid;opacity:.95}
.chip.theme{color:var(--muted);border-color:var(--edge);text-transform:none}
.chip.dim{color:var(--muted);border-color:var(--edge)}
.hidden{display:none!important}
.empty{color:var(--muted);font:13px/1.5 ui-monospace,monospace;padding:44px 4px}
.foot{margin-top:30px;padding-top:16px;border-top:1px solid var(--edge);color:var(--muted);font:12px/1.6 ui-monospace,monospace}
.foot b{color:var(--fg)}
@media (prefers-reduced-motion:reduce){.card{transition:none}}
)CSS";

void write_texture_gallery(const std::string &outdir, const std::vector<TexItem> &items)
{
    std::ofstream f(outdir + "/index.html", std::ios::binary);
    if(!f) { fprintf(stderr, "cannot write %s/index.html\n", outdir.c_str()); return; }

    std::map<std::string,int> typeCount, themeCount;
    for(const auto &it : items) { typeCount[it.maptype]++; if(!it.theme.empty()) themeCount[it.theme]++; }
    std::vector<std::pair<std::string,int>> types, themes;
    for(auto &c : TYPE_COLOR) if(typeCount.count(c.first)) types.push_back({c.first, typeCount[c.first]});
    for(auto &t : themeCount) themes.push_back({t.first, t.second});
    std::sort(themes.begin(), themes.end(), [](auto&a, auto&b){ return a.second > b.second; });

    f << "<!doctype html><html lang=\"en\"><head><meta charset=\"utf-8\">"
      << "<meta name=\"viewport\" content=\"width=device-width,initial-scale=1\">"
      << "<title>CoD2 IWI Texture Gallery</title><style>" << CSS;
    for(auto &c : TYPE_COLOR) {
        std::string id = c.first; // data attribute uses the raw label
        f << ".chip[data-c=\"" << esc(id) << "\"]{color:" << c.second
          << ";border-color:color-mix(in oklab," << c.second << " 45%,transparent)}"
          << ".filter[data-f=\"" << esc(id) << "\"].on{background:" << c.second << ";border-color:" << c.second << "}";
    }
    f << "</style></head><body><header><div class=\"head-in\">"
      << "<p class=\"eyebrow\">Call of Duty 2 &middot; Stock Texture Library</p>"
      << "<h1>IWI <b>Texture</b> Gallery</h1>"
      << "<p class=\"sub\">" << items.size() << " textures &middot; classified by map type &middot; alpha shown over a checkerboard</p>"
      << "<div class=\"controls\"><label class=\"search\">"
      << "<svg width=\"16\" height=\"16\" viewBox=\"0 0 24 24\" fill=\"none\" stroke=\"currentColor\" stroke-width=\"2\"><circle cx=\"11\" cy=\"11\" r=\"7\"/><path d=\"m21 21-4.3-4.3\"/></svg>"
      << "<input id=\"q\" type=\"search\" placeholder=\"Search " << items.size() << " textures...\" autofocus></label>"
      << "<span class=\"count\"><b id=\"n\">" << items.size() << "</b> / " << items.size() << "</span></div>";

    f << "<div class=\"rowlabel\">Map Type</div><div class=\"filters\" id=\"typef\">"
      << "<button class=\"filter on\" data-f=\"All\">All <span class=\"n\">" << items.size() << "</span></button>";
    for(auto &t : types)
        f << "<button class=\"filter\" data-f=\"" << esc(t.first) << "\">" << esc(t.first)
          << " <span class=\"n\">" << t.second << "</span></button>";
    f << "</div>";
    if(!themes.empty()) {
        f << "<div class=\"rowlabel\">Theme / Map</div><div class=\"filters\" id=\"themef\">"
          << "<button class=\"filter on\" data-t=\"All\">All</button>";
        for(auto &t : themes)
            f << "<button class=\"filter\" data-t=\"" << esc(t.first) << "\">" << esc(t.first)
              << " <span class=\"n\">" << t.second << "</span></button>";
        f << "</div>";
    }
    f << "</div></header><div class=\"wrap\"><main class=\"grid\" id=\"grid\">";

    for(const auto &it : items) {
        std::string search = lower(it.name);
        f << "<article class=\"card\" data-name=\"" << esc(search) << "\" data-type=\"" << esc(it.maptype)
          << "\" data-theme=\"" << esc(it.theme) << "\">"
          << "<div class=\"thumb\"><img loading=\"lazy\" src=\"" << esc(it.file) << "\" alt=\"" << esc(it.name)
          << "\" title=\"" << esc(it.name) << " (" << it.w << "x" << it.h << ")\"></div>"
          << "<div class=\"meta\"><div class=\"slug\">" << esc(it.name) << "</div><div class=\"chips\">"
          << "<span class=\"chip\" data-c=\"" << esc(it.maptype) << "\">" << esc(it.maptype) << "</span>";
        if(!it.theme.empty()) f << "<span class=\"chip theme\">" << esc(it.theme) << "</span>";
        f << "</div></div></article>";
    }
    f << "</main><p class=\"empty hidden\" id=\"empty\">No textures match.</p>"
      << "<p class=\"foot\">Decoded from IWI (DXT1/3/5) &middot; " << types.size()
      << " map types &middot; raw names preserved for material scripts.</p></div>";

    f << "<script>"
      "const q=document.getElementById('q'),cards=[...document.querySelectorAll('.card')],nEl=document.getElementById('n'),empty=document.getElementById('empty');"
      "let type='All',theme='All';"
      "function apply(){const s=q.value.trim().toLowerCase();let n=0;"
      "for(const c of cards){const okS=!s||c.dataset.name.includes(s);const okT=type==='All'||c.dataset.type===type;"
      "const okH=theme==='All'||c.dataset.theme===theme;const ok=okS&&okT&&okH;c.classList.toggle('hidden',!ok);if(ok)n++;}"
      "nEl.textContent=n;empty.classList.toggle('hidden',n>0);}"
      "q.addEventListener('input',apply);"
      "typef.addEventListener('click',e=>{const b=e.target.closest('.filter');if(!b)return;type=b.dataset.f;"
      "typef.querySelectorAll('.filter').forEach(x=>x.classList.toggle('on',x===b));apply();});"
      "const tf=document.getElementById('themef');if(tf)tf.addEventListener('click',e=>{const b=e.target.closest('.filter');if(!b)return;theme=b.dataset.t;"
      "tf.querySelectorAll('.filter').forEach(x=>x.classList.toggle('on',x===b));apply();});"
      "apply();</script></body></html>\n";
    printf("Gallery: %s/index.html (%zu textures, %zu map types)\n", outdir.c_str(), items.size(), types.size());
}
