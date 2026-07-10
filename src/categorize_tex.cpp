#include "categorize_tex.h"
#include <regex>
#include <vector>
#include <utility>
#include <cctype>

namespace {

struct Rule { const char *cat; std::regex rx; };

std::string toLower(std::string s) { for(char &c : s) c = (char)std::tolower((unsigned char)c); return s; }
std::string stripExt(std::string s) { auto d = s.rfind('.'); if(d != std::string::npos && s.substr(d) == ".iwi") s = s.substr(0, d); return s; }

// Ordered; first match wins. Order matters: aux-map suffixes before the Color fallback.
const std::vector<std::pair<const char*, const char*>> TYPE_SRC = {
    {"System",       R"(^\$)"},
    {"Normal",       R"((-gggr|_bump|_nml|_ddn|normalmap|_normal\b))"},
    {"Specular",     R"((_spec|_cosine|_gloss))"},
    {"HUD/UI",       R"((^hud|_hud|headicon|hudicon|objpoint|objective|compass|^rank_|_rank\b|crosshair|reticle|killicons?|_icon\b|ammoicon))"},
    {"Loadscreen/UI",R"((loadscreen|slideshow|^background|_background|^lsm_|^plan_|mainmenu|mappic|^menu_|_menu\b|briefing|missionpic))"},
    {"Decal/Detail", R"((decal|_dtl\b|detail_|_detail\b|blood|bullethole|scorch|crack))"},
    {"Specular",     R"((^~.*-rgb|^~))"},  // remaining ~ aux maps are spec/gloss
    {"Color",        R"(.*)"},              // fallback
};

// Same map/theme axis as the model gallery.
const std::vector<std::pair<const char*, const char*>> THEME_SRC = {
    {"Egypt",    R"((^egypt|_egypt|elalamein|el_alamein|matmata|libya|tunisia|northafrica|_africa|africa_|desert|_dak\b|dak_))"},
    {"Caen",     R"((^caen|_caen))"},
    {"Duhoc",    R"((^duhoc|_duhoc))"},
    {"Hill400",  R"((^hill400|_hill400))"},
    {"Toujane",  R"((^toujane|_toujane))"},
    {"Stalingrad",R"((^stalingrad|_stalingrad|^moscow|_moscow|_winter|winter_|_snow\b|snow_|russian))"},
    {"Normandy", R"((_normandy|normandy_|newviller|coreviller|_rhine|rhine_|eldaba|gully|silotown|_french|french_))"},
};

std::vector<Rule> compile(const std::vector<std::pair<const char*, const char*>> &src)
{
    std::vector<Rule> out; out.reserve(src.size());
    for(auto &s : src) out.push_back({ s.first, std::regex(s.second, std::regex::ECMAScript | std::regex::icase) });
    return out;
}
const std::vector<Rule>& typeRules()  { static auto r = compile(TYPE_SRC);  return r; }
const std::vector<Rule>& themeRules() { static auto r = compile(THEME_SRC); return r; }

std::string prettify(const std::string &raw)
{
    std::string n = stripExt(toLower(raw));
    while(!n.empty() && (n[0] == '~' || n[0] == '$')) n.erase(n.begin());
    for(char &c : n) if(c == '_') c = ' ';
    // Title-case first letter of each word; leave the rest (names carry casing meaning).
    std::string out; bool bow = true;
    for(char c : n) {
        if(c == ' ' || c == '-') { bow = true; out += c; }
        else { out += bow ? (char)std::toupper((unsigned char)c) : c; bow = false; }
    }
    return out.empty() ? raw : out;
}

} // namespace

TexClass classify_texture(const std::string &rawName)
{
    std::string n = stripExt(toLower(rawName));
    TexClass r;
    r.maptype = "Color";
    for(const auto &rule : typeRules())
        if(std::regex_search(n, rule.rx)) { r.maptype = rule.cat; break; }
    for(const auto &rule : themeRules())
        if(std::regex_search(n, rule.rx)) { r.theme = rule.cat; break; }
    r.pretty = prettify(rawName);
    return r;
}
