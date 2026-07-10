# cod2-iwi-gallery

A batch thumbnail gallery generator for **Call of Duty 2 IWI textures**. Point it at
a CoD2 install or an extracted asset tree; it decodes every `.iwi` image, composites
transparency over a checkerboard, and writes a searchable static HTML gallery you can
drop onto GitHub Pages.

Sibling tool to [cod2-xmodel-gallery](../cod2-xmodel-gallery) (3D models). This one
is textures only, needs **no GPU** — pure CPU decode → downscale → JPEG.

## Gallery

Live, searchable, filtered by map type and theme:
**[GitHub Pages](https://nawaftahir.github.io/cod2-iwi-gallery/)** ·
**[GitLab Pages](https://nawaftahir.gitlab.io/cod2-iwi-gallery/)**

Models live in the sibling [cod2-xmodel-gallery](https://nawaftahir.github.io/cod2-xmodel-gallery/).

## What it does

- **Reads `.iwd` archives directly** (plain zip) or a loose extracted asset tree.
- **Decodes IWI v5** (CoD2): DXT1 / DXT3 / DXT5, plus RGB24 / ARGB32.
- **Classifies by map type** from CoD2 naming conventions — Color, Normal (`-gggr`),
  Specular (`_spec`, `~`), Decal/Detail, HUD/UI, Loadscreen/UI, System (`$`) — and
  tags a map/theme (Egypt, Caen, Stalingrad, ...).
- **Shows transparency** by compositing alpha over a checkerboard.
- **Batch mode** writes a thumbnail per texture + an `index.html` with map-type and
  theme filters and live search.

## Building

No `-dev` packages, no `sudo`, no GPU — just `g++`/`gcc` (C++17).

```bash
./build.sh          # or: cmake -B build && cmake --build build
```

## Usage

```bash
# Full gallery from a CoD2 install (reads main/iw_*.iwd)
./build/cod2-iwi-gallery --basepath="/path/to/Call of Duty 2" --batch --outdir=docs

# From an extracted tree (a dir with images/, or a parent of iw_00/ iw_01/ ...)
./build/cod2-iwi-gallery --loose=/path/to/stockrawfiles --batch --outdir=docs

# One texture -> image
./build/cod2-iwi-gallery --loose=/path/to/stockrawfiles sherman_color_desert
```

### Options

| flag | meaning |
|------|---------|
| `--basepath=<dir>` | CoD2 install; reads `main/iw_*.iwd` (+ `localized_*`) |
| `--loose=<dir>` | extracted asset tree, or a parent of `iw_*` dirs (repeatable) |
| `--batch` | render every texture + write `index.html` (default) |
| `--outdir=<dir>` | output directory (default `tex`) |
| `--maxdim=N` | thumbnail max dimension, 32..1024 (default 200) |
| `--quality=N` | JPEG quality 1..100 (default 82) |
| `--limit=N` | only the first N textures (for quick tests) |

## Numbers

The full stock set is ~4,899 unique textures; the tool renders **4,872** of them in
~30s (~40 MB output). The 27 skips are rare pixel formats (GA16 and two uncommon
codes) used for a handful of special maps — not DXT, not worth decoding for a gallery.

## Layout

```
src/vfs.*            asset VFS over .iwd archives and loose dirs   (shared w/ xmodel tool)
src/image.*          DXT1/3/5 + IWI v5 + DDS/TGA -> RGBA8          (shared w/ xmodel tool)
src/categorize_tex.* map-type + theme classification
src/gallery_tex.*    thumbnail compositing + searchable index.html
src/main.cpp         CLI
third_party/         miniz (zip), stb_image_write (JPEG/PNG)
```

`vfs.*` and `image.*` are kept byte-identical to the xmodel tool; edit them in one
place and copy across.
