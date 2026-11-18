// The Xenos's textures on the host GPU (texture.h). The fetch constant's
// fields, the guest layout of levels and slices (pitches, packed mip
// tails, tiling), the formats' meanings and their block decompression are
// Xenia's (BSD-3: its xenos.h, texture_util.cc, texture_address.h, texture
// caches and texture load shaders); docs/xenos.md, "Textures", says what
// is taken from where and what is a choice.
//
// PackedMipOffset and Tiled3D are ported from Xenia's (texture_util.cc's
// GetPackedMipOffset, texture_address.h):
// Copyright (c) 2015, Ben Vanik. All rights reserved. BSD 3-Clause; the
// licence is in NOTICE.

#include "xbox360/texture.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>
#include <map>
#include <memory>
#include <thread>
#include <vector>

#include "ppc/fatal.h"
#include "xbox360/display.h"
#include "xbox360/edram.h"
#include "xbox360/gpu.h"
#include "xbox360/memory.h"

namespace recomp::xbox360 {
namespace {

using ppc::Fatal;

// Textures not sampled for this many frames (swaps) are dropped: about
// five seconds at 60 frames a second, which bounds host texture memory to
// what a few seconds of frames use (a choice).
constexpr uint64_t kEvictFrames = 300;

int32_t Signed(uint32_t v, uint32_t bits) { return int32_t(v << (32 - bits)) >> (32 - bits); }
uint32_t Log2Ceil(uint32_t v) { return v <= 1 ? 0 : 32 - uint32_t(__builtin_clz(v - 1)); }
uint32_t Log2Floor(uint32_t v) { return 31 - uint32_t(__builtin_clz(v)); }
uint32_t NextPow2(uint32_t v) { return 1u << Log2Ceil(v); }
uint32_t Align(uint32_t v, uint32_t a) { return (v + a - 1) / a * a; }

// ---- formats -----------------------------------------------------------------
// The host image a format decodes to: 8-bit unorm, 16-bit unorm or snorm
// (signed components), half floats, floats.
enum HostClass { kRgba8, kRgba16, kRgba16Snorm, kRgba16Float, kRgba32Float };

// A texture format: its blocks (texels and bytes), its components (the
// missing ones repeat the last, as Xenia's host swizzles do: RRRR, RGGG,
// RGBB) and its host class; bits 0 when it is not implemented.
struct Format {
  uint8_t bw, bh, bytes, components;
  HostClass host;
  const char* name;
};
const Format kFormats[64] = {
    {0, 0, 0, 0, kRgba8, "k_1_REVERSE"},
    {0, 0, 0, 0, kRgba8, "k_1"},
    {1, 1, 1, 1, kRgba8, "k_8"},
    {1, 1, 2, 4, kRgba8, "k_1_5_5_5"},
    {1, 1, 2, 3, kRgba8, "k_5_6_5"},
    {1, 1, 2, 3, kRgba8, "k_6_5_5"},
    {1, 1, 4, 4, kRgba8, "k_8_8_8_8"},
    {1, 1, 4, 4, kRgba16, "k_2_10_10_10"},
    {1, 1, 1, 1, kRgba8, "k_8_A"},
    {0, 0, 0, 0, kRgba8, "k_8_B"},
    {1, 1, 2, 2, kRgba8, "k_8_8"},
    {2, 1, 4, 3, kRgba8, "k_Cr_Y1_Cb_Y0_REP"},
    {2, 1, 4, 3, kRgba8, "k_Y1_Cr_Y0_Cb_REP"},
    {0, 0, 0, 0, kRgba8, "k_16_16_EDRAM"},
    {1, 1, 4, 4, kRgba8, "k_8_8_8_8_A"},
    {1, 1, 2, 4, kRgba8, "k_4_4_4_4"},
    {1, 1, 4, 3, kRgba16, "k_10_11_11"},
    {1, 1, 4, 3, kRgba16, "k_11_11_10"},
    {4, 4, 8, 4, kRgba8, "k_DXT1"},
    {4, 4, 16, 4, kRgba8, "k_DXT2_3"},
    {4, 4, 16, 4, kRgba8, "k_DXT4_5"},
    {0, 0, 0, 0, kRgba8, "k_16_16_16_16_EDRAM"},
    {1, 1, 4, 1, kRgba32Float, "k_24_8"},
    {1, 1, 4, 1, kRgba32Float, "k_24_8_FLOAT"},
    {1, 1, 2, 1, kRgba16, "k_16"},
    {1, 1, 4, 2, kRgba16, "k_16_16"},
    {1, 1, 8, 4, kRgba16, "k_16_16_16_16"},
    {1, 1, 2, 1, kRgba16Float, "k_16_EXPAND"},
    {1, 1, 4, 2, kRgba16Float, "k_16_16_EXPAND"},
    {1, 1, 8, 4, kRgba16Float, "k_16_16_16_16_EXPAND"},
    {1, 1, 2, 1, kRgba16Float, "k_16_FLOAT"},
    {1, 1, 4, 2, kRgba16Float, "k_16_16_FLOAT"},
    {1, 1, 8, 4, kRgba16Float, "k_16_16_16_16_FLOAT"},
    {0, 0, 0, 0, kRgba8, "k_32"},
    {0, 0, 0, 0, kRgba8, "k_32_32"},
    {0, 0, 0, 0, kRgba8, "k_32_32_32_32"},
    {1, 1, 4, 1, kRgba32Float, "k_32_FLOAT"},
    {1, 1, 8, 2, kRgba32Float, "k_32_32_FLOAT"},
    {1, 1, 16, 4, kRgba32Float, "k_32_32_32_32_FLOAT"},
    {0, 0, 0, 0, kRgba8, "k_32_AS_8"},
    {0, 0, 0, 0, kRgba8, "k_32_AS_8_8"},
    {0, 0, 0, 0, kRgba8, "k_16_MPEG"},
    {0, 0, 0, 0, kRgba8, "k_16_16_MPEG"},
    {0, 0, 0, 0, kRgba8, "k_8_INTERLACED"},
    {0, 0, 0, 0, kRgba8, "k_32_AS_8_INTERLACED"},
    {0, 0, 0, 0, kRgba8, "k_32_AS_8_8_INTERLACED"},
    {0, 0, 0, 0, kRgba8, "k_16_INTERLACED"},
    {0, 0, 0, 0, kRgba8, "k_16_MPEG_INTERLACED"},
    {0, 0, 0, 0, kRgba8, "k_16_16_MPEG_INTERLACED"},
    {4, 4, 16, 2, kRgba8, "k_DXN"},
    {1, 1, 4, 4, kRgba8, "k_8_8_8_8_AS_16_16_16_16"},
    {4, 4, 8, 4, kRgba8, "k_DXT1_AS_16_16_16_16"},
    {4, 4, 16, 4, kRgba8, "k_DXT2_3_AS_16_16_16_16"},
    {4, 4, 16, 4, kRgba8, "k_DXT4_5_AS_16_16_16_16"},
    {1, 1, 4, 4, kRgba16, "k_2_10_10_10_AS_16_16_16_16"},
    {1, 1, 4, 3, kRgba16, "k_10_11_11_AS_16_16_16_16"},
    {1, 1, 4, 3, kRgba16, "k_11_11_10_AS_16_16_16_16"},
    {1, 1, 12, 3, kRgba32Float, "k_32_32_32_FLOAT"},
    {4, 4, 8, 1, kRgba8, "k_DXT3A"},
    {4, 4, 8, 1, kRgba8, "k_DXT5A"},
    {4, 4, 8, 2, kRgba8, "k_CTX1"},
    {4, 4, 8, 4, kRgba8, "k_DXT3A_AS_1_1_1_1"},
    {0, 0, 0, 0, kRgba8, "k_8_8_8_8_GAMMA_EDRAM"},
    {0, 0, 0, 0, kRgba8, "k_2_10_10_10_FLOAT_EDRAM"},
};
bool Compressed(uint32_t f) { return kFormats[f].bw == 4; }
bool FloatFormat(uint32_t f) { return kFormats[f].host == kRgba16Float || kFormats[f].host == kRgba32Float; }

// Integer components as the fetch constant's sign says (per data
// component X, Y, Z, W: 1 signed, as max(v / (2^(n-1) - 1), -1), the
// Xenos's only signed mode; 0, 2 biased and 3 gamma unsigned, xe_sample
// applying the latter two after filtering).
float Component(uint32_t v, uint32_t bits, uint32_t sign) {
  if (sign == 1) return std::max(float(Signed(v, bits)) / float((1u << (bits - 1)) - 1), -1.0f);
  return float(v) / float(bits == 32 ? 4294967295.0 : double((1ull << bits) - 1));
}

// Exact for values a half holds (the half formats' own), rounded to the
// nearest even otherwise.
uint16_t ToHalf(float f) {
  uint32_t u;
  std::memcpy(&u, &f, 4);
  uint32_t s = u >> 16 & 0x8000, e = u >> 23 & 0xFF, m = u & 0x7FFFFF;
  if (e == 0xFF) return uint16_t(s | 0x7C00 | (m ? 0x200 : 0));
  int32_t he = int32_t(e) - 112;
  if (he >= 31) return uint16_t(s | 0x7C00);
  if (he <= 0) {
    if (he < -10) return uint16_t(s);
    m |= 0x800000;
    uint32_t shift = uint32_t(14 - he), half = m >> shift, rest = m & ((1u << shift) - 1), mid = 1u << (shift - 1);
    if (rest > mid || (rest == mid && (half & 1))) half++;
    return uint16_t(s | half);
  }
  uint32_t half = uint32_t(he) << 10 | m >> 13, rest = m & 0x1FFF;
  if (rest > 0x1000 || (rest == 0x1000 && (half & 1))) half++;
  return uint16_t(s | half);
}

// DXT colour endpoints, 5:6:5 (red high) to 8:8:8 with the low bits
// replicated, and the four colours (Xenia's: thirds truncated; in the
// three-colour mode of DXT1 the half truncated and transparent black).
void DxtColors(uint32_t endpoints, bool dxt1, uint8_t out[4][4]) {
  for (uint32_t k = 0; k < 2; k++) {
    uint32_t c = endpoints >> (16 * k) & 0xFFFF;
    uint32_t r = c >> 11, g = c >> 5 & 63, b = c & 31;
    out[k][0] = uint8_t(r << 3 | r >> 2);
    out[k][1] = uint8_t(g << 2 | g >> 4);
    out[k][2] = uint8_t(b << 3 | b >> 2);
    out[k][3] = 255;
  }
  bool four = !dxt1 || (endpoints & 0xFFFF) > (endpoints >> 16);
  for (uint32_t i = 0; i < 3; i++) {
    if (four) {
      out[2][i] = uint8_t((2 * out[0][i] + out[1][i]) / 3);
      out[3][i] = uint8_t((out[0][i] + 2 * out[1][i]) / 3);
    } else {
      out[2][i] = uint8_t((out[0][i] + out[1][i]) >> 1);
      out[3][i] = 0;
    }
  }
  out[2][3] = 255;
  out[3][3] = four ? 255 : 0;
}
// A DXT5 alpha (BC4) block: the 16 values (Xenia's: sevenths and fifths
// truncated).
void Dxt5Alpha(const uint8_t* b, uint8_t out[16]) {
  uint32_t a0 = b[0], a1 = b[1];
  uint64_t codes = 0;
  for (uint32_t k = 0; k < 6; k++) codes |= uint64_t(b[2 + k]) << (8 * k);
  for (uint32_t i = 0; i < 16; i++) {
    uint32_t c = uint32_t(codes >> (3 * i) & 7);
    if (c == 0) out[i] = uint8_t(a0);
    else if (c == 1) out[i] = uint8_t(a1);
    else if (a0 > a1) out[i] = uint8_t((a0 * (8 - c) + a1 * (c - 1)) / 7);
    else if (c < 6) out[i] = uint8_t((a0 * (6 - c) + a1 * (c - 1)) / 5);
    else out[i] = c == 6 ? 0 : 255;
  }
}

// A block's texels, row by row, as floats per component (the block's
// bytes after the endian swap).
void DecodeBlock(uint32_t f, const uint8_t* b, uint32_t signs, float out[16][4]) {
  auto le16 = [&](uint32_t at) { return uint32_t(b[at] | b[at + 1] << 8); };
  auto le32 = [&](uint32_t at) { return le16(at) | le16(at + 2) << 16; };
  auto sign = [&](uint32_t c) { return signs >> (2 * c) & 3; };
  // Packed integer components: their widths from bit 0.
  auto fields = [&](uint32_t v, std::initializer_list<uint32_t> widths) {
    uint32_t at = 0, c = 0;
    for (uint32_t w : widths) {
      out[0][c] = Component(v >> at & ((1ull << w) - 1), w, sign(c));
      at += w;
      c++;
    }
  };
  switch (f) {
    case 2: case 8: fields(b[0], {8}); break;
    case 3: fields(le16(0), {5, 5, 5, 1}); break;
    case 4: fields(le16(0), {5, 6, 5}); break;
    case 5: fields(le16(0), {5, 5, 6}); break;
    case 6: case 14: case 50: fields(le32(0), {8, 8, 8, 8}); break;
    case 7: case 54: fields(le32(0), {10, 10, 10, 2}); break;
    case 10: fields(le16(0), {8, 8}); break;
    case 15: fields(le16(0), {4, 4, 4, 4}); break;
    case 16: case 55: fields(le32(0), {11, 11, 10}); break;
    case 17: case 56: fields(le32(0), {10, 11, 11}); break;
    case 24: fields(le16(0), {16}); break;
    case 25: fields(le32(0), {16, 16}); break;
    case 26:
      fields(le32(0), {16, 16});
      out[0][2] = Component(le16(4), 16, sign(2));
      out[0][3] = Component(le16(6), 16, sign(3));
      break;
    case 11: case 12: {
      // Cr, Y and Cb as red, green and blue of both texels (Xenia's).
      uint32_t y0 = f == 11 ? b[0] : b[1], y1 = f == 11 ? b[2] : b[3], cb = f == 11 ? b[1] : b[0],
               cr = f == 11 ? b[3] : b[2];
      for (uint32_t t = 0; t < 2; t++) {
        out[t][0] = float(cr) / 255;
        out[t][1] = float(t ? y1 : y0) / 255;
        out[t][2] = float(cb) / 255;
      }
      break;
    }
    case 22: out[0][0] = Unorm24(le32(0) >> 8); break;
    case 23: out[0][0] = F20e4(le32(0) >> 8); break;
    case 27: case 30: out[0][0] = Half(uint16_t(le16(0))); break;
    case 28: case 31: for (uint32_t c = 0; c < 2; c++) out[0][c] = Half(uint16_t(le16(2 * c))); break;
    case 29: case 32: for (uint32_t c = 0; c < 4; c++) out[0][c] = Half(uint16_t(le16(2 * c))); break;
    case 36: case 37: case 38: case 57:
      for (uint32_t c = 0; c < kFormats[f].components; c++) {
        uint32_t v = le32(4 * c);
        std::memcpy(&out[0][c], &v, 4);
      }
      break;
    case 18: case 51: case 19: case 52: case 20: case 53: {
      bool dxt1 = f == 18 || f == 51;
      uint32_t colour = dxt1 ? 0 : 8;
      uint8_t c[4][4], alpha[16];
      DxtColors(le32(colour), dxt1, c);
      uint32_t idx = le32(colour + 4);
      if (f == 20 || f == 53) Dxt5Alpha(b, alpha);
      for (uint32_t i = 0; i < 16; i++) {
        const uint8_t* t = c[idx >> (2 * i) & 3];
        for (uint32_t k = 0; k < 3; k++) out[i][k] = float(t[k]) / 255;
        uint32_t a = t[3];
        if (f == 19 || f == 52) a = (b[i / 2] >> (4 * (i & 1)) & 15) * 17;
        if (f == 20 || f == 53) a = alpha[i];
        out[i][3] = float(a) / 255;
      }
      break;
    }
    case 49: {  // two DXT5 alpha blocks: X, Y
      uint8_t x[16], y[16];
      Dxt5Alpha(b, x);
      Dxt5Alpha(b + 8, y);
      for (uint32_t i = 0; i < 16; i++) {
        out[i][0] = float(x[i]) / 255;
        out[i][1] = float(y[i]) / 255;
      }
      break;
    }
    case 59: {
      uint8_t x[16];
      Dxt5Alpha(b, x);
      for (uint32_t i = 0; i < 16; i++) out[i][0] = float(x[i]) / 255;
      break;
    }
    case 58: case 61:
      // 4 bits a texel: alpha to X, or (Xenia's order, MSB red) one bit to
      // each of X, Y, Z (from bit 3 down) and W (bit 0).
      for (uint32_t i = 0; i < 16; i++) {
        uint32_t n = b[i / 2] >> (4 * (i & 1)) & 15;
        if (f == 58) {
          out[i][0] = float(n * 17) / 255;
        } else {
          for (uint32_t k = 0; k < 4; k++) out[i][k] = float(n >> (3 - k) & 1);
        }
      }
      break;
    case 60: {
      // CTX1: endpoints X, Y a byte each (the first in bits 8-15 and 0-7,
      // the second in 24-31 and 16-23), 2-bit indices as DXT's, thirds
      // truncated (Xenia's).
      uint32_t e = le32(0), idx = le32(4);
      uint32_t x0 = e >> 8 & 0xFF, y0 = e & 0xFF, x1 = e >> 24, y1 = e >> 16 & 0xFF;
      static const uint32_t kWeight[4] = {0, 3, 1, 2};  // of the second endpoint, in thirds
      for (uint32_t i = 0; i < 16; i++) {
        uint32_t w = kWeight[idx >> (2 * i) & 3];
        out[i][0] = float((x0 * (3 - w) + x1 * w) / 3) / 255;
        out[i][1] = float((y0 * (3 - w) + y1 * w) / 3) / 255;
      }
      break;
    }
    default:
      Fatal(nullptr, "GPU: texture format %s (not implemented yet)", kFormats[f].name);
  }
  uint32_t n = kFormats[f].bw * kFormats[f].bh, comps = kFormats[f].components;
  for (uint32_t i = 0; i < n; i++)
    for (uint32_t c = comps; c < 4; c++) out[i][c] = out[i][comps - 1];
}

// ---- layout ------------------------------------------------------------------
// The fetch constant's fields (xe_gpu_texture_fetch_t).
struct Fetch {
  uint32_t w[6];
  uint32_t type() const { return w[0] & 3; }
  uint32_t signs() const { return w[0] >> 2 & 0xFF; }
  uint32_t clamp(uint32_t axis) const { return w[0] >> (10 + 3 * axis) & 7; }
  uint32_t pitch() const { return w[0] >> 22 & 0x1FF; }  // texels / 32
  bool tiled() const { return w[0] >> 31; }
  uint32_t format() const { return w[1] & 0x3F; }
  uint32_t endian() const { return w[1] >> 6 & 3; }
  bool stacked() const { return w[1] >> 10 & 1; }
  uint32_t base_page() const { return w[1] >> 12 & 0x1FFFF; }
  uint32_t swizzle() const { return w[3] >> 1 & 0xFFF; }
  int32_t exp_adjust() const { return Signed(w[3] >> 13, 6); }
  uint32_t mag() const { return w[3] >> 19 & 3; }
  uint32_t min() const { return w[3] >> 21 & 3; }
  uint32_t mip() const { return w[3] >> 23 & 3; }
  uint32_t aniso() const { return w[3] >> 25 & 7; }
  bool vol_mag() const { return w[4] & 1; }
  uint32_t mip_min() const { return w[4] >> 2 & 0xF; }
  uint32_t mip_max() const { return w[4] >> 6 & 0xF; }
  float lod_bias() const { return float(Signed(w[4] >> 12, 10)) / 32; }
  uint32_t dimension() const { return w[5] >> 9 & 3; }
  bool packed() const { return w[5] >> 11 & 1; }
  uint32_t mip_page() const { return w[5] >> 12 & 0x1FFFF; }
};

// A texture's sizes and levels as Xenia's GetSubresourcesFromFetchConstant
// and GetGuestTextureLayout derive them.
struct Layout {
  uint32_t dimension, width, height, depth, layers;  // depth: 3D; layers: stacked or cube
  uint32_t base, mips;                               // physical addresses, 0 none
  uint32_t mip_min, mip_max, packed_level;
  uint32_t format;
  bool tiled;
  uint32_t pitch_texels;
};

Layout GetLayout(const Fetch& f) {
  Layout l{};
  l.dimension = f.dimension();
  l.format = f.format();
  l.tiled = f.tiled();
  l.pitch_texels = f.pitch() << 5;
  l.depth = l.layers = 1;
  switch (l.dimension) {
    case 0: l.width = (f.w[2] & 0xFFFFFF) + 1; l.height = 1; break;
    case 1:
      l.width = (f.w[2] & 0x1FFF) + 1;
      l.height = (f.w[2] >> 13 & 0x1FFF) + 1;
      l.layers = f.stacked() ? (f.w[2] >> 26) + 1 : 1;
      break;
    case 2:
      l.width = (f.w[2] & 0x7FF) + 1;
      l.height = (f.w[2] >> 11 & 0x7FF) + 1;
      l.depth = (f.w[2] >> 22) + 1;
      break;
    default:
      l.width = (f.w[2] & 0x1FFF) + 1;
      l.height = (f.w[2] >> 13 & 0x1FFF) + 1;
      l.layers = 6;
  }
  uint32_t longest = std::max(l.width, l.height);
  if (l.dimension == 2) longest = std::max(longest, l.depth);
  uint32_t size_max = Log2Floor(longest);
  uint32_t base_page = f.base_page(), mip_page = f.mip_page(), mip_min = 0, mip_max = 0;
  if (mip_page) {
    mip_min = std::min(f.mip_min(), size_max);
    mip_max = std::max(std::min(f.mip_max(), size_max), mip_min);
  }
  if (mip_max) {
    if (!base_page) mip_min = std::max(mip_min, 1u);
    if (mip_min) base_page = 0;
  } else {
    mip_page = 0;
  }
  l.base = base_page << 12;
  l.mips = mip_page << 12;
  l.mip_min = mip_min;
  l.mip_max = mip_max;
  uint32_t smaller = Log2Ceil(std::min(l.width, l.height));
  l.packed_level = f.packed() ? (smaller > 4 ? smaller - 4 : 0) : UINT32_MAX;
  return l;
}

// Where a mip of a packed tail sits in it, in blocks (Xenia's
// GetPackedMipOffset).
void PackedMipOffset(const Layout& l, uint32_t mip, uint32_t& x, uint32_t& y, uint32_t& z) {
  uint32_t lw = Log2Ceil(l.width), lh = Log2Ceil(l.height), ls = std::min(lw, lh);
  uint32_t base = ls > 4 ? ls - 4 : 0, packed = mip - base;
  z = 0;
  if (packed < 3) {
    if (lw > lh) {
      x = 0;
      y = 16 >> packed;
    } else {
      x = 16 >> packed;
      y = 0;
    }
  } else {
    uint32_t offset;
    if (lw > lh) {
      offset = (1u << (lw - base)) >> (packed - 2);
      x = offset;
      y = 0;
    } else {
      offset = (1u << (lh - base)) >> (packed - 2);
      x = 0;
      y = offset;
    }
    if (offset < 4) {
      uint32_t ld = Log2Ceil(l.depth);
      z = ld > 1 + packed ? (ld - packed) * 4 : 4;
    }
  }
  const Format& fm = kFormats[l.format];
  x /= fm.bw;
  y /= fm.bh;
}

// The storage of a level: its address, row pitch in blocks, slice rows in
// blocks, the stride between array slices, and where the level sits in it
// (a packed tail's offset).
struct Storage {
  uint32_t address, pitch_blocks, rows, slice_bytes;
  uint32_t ox, oy, oz;
};

// The storage of `level` stored as level `s` (s 0 is the base or, for a
// tail packed at level 0, the mips' tail), and the bytes of all its
// slices.
Storage StorageOf(const Layout& l, uint32_t s, bool base) {
  const Format& fm = kFormats[l.format];
  uint32_t pitch_texels = base ? l.pitch_texels : std::max(NextPow2(l.width) >> s, 1u);
  uint32_t rows_texels = base ? l.height : std::max(NextPow2(l.height) >> s, 1u);
  Storage st{};
  st.pitch_blocks = Align(Align(pitch_texels, fm.bw) / fm.bw, 32);
  uint32_t row_bytes = st.pitch_blocks * fm.bytes;
  if (!l.tiled && !base) row_bytes = Align(row_bytes, 256);
  st.pitch_blocks = row_bytes / fm.bytes;
  st.rows = l.dimension ? Align(Align(rows_texels, fm.bh) / fm.bh, 32) : 1;
  st.slice_bytes = row_bytes * st.rows;
  if (l.dimension == 2) st.slice_bytes *= Align(l.depth, 4);
  st.slice_bytes = Align(st.slice_bytes, 4096);
  return st;
}

// A level's storage, placed: at the base (level 0) or among the mips.
Storage LevelStorage(const Layout& l, uint32_t level) {
  uint32_t array = l.dimension == 2 ? 1 : l.layers;
  Storage st;
  if (level == 0) {
    st = StorageOf(l, 0, true);
    st.address = l.base;
  } else {
    uint32_t s = std::min(level, l.packed_level), first = l.packed_level == 0 ? 0 : 1, offset = 0;
    for (uint32_t k = first; k < s; k++) offset += StorageOf(l, k, false).slice_bytes * array;
    st = StorageOf(l, s, false);
    st.address = l.mips + offset;
  }
  st.ox = st.oy = st.oz = 0;
  if (level >= l.packed_level) PackedMipOffset(l, level, st.ox, st.oy, st.oz);
  return st;
}

// Xenia's Tiled3D.
int64_t Tiled3D(int32_t x, int32_t y, int32_t z, uint32_t pitch, uint32_t height, uint32_t bpb_log2) {
  int32_t outer = ((((z >> 2) * int32_t(height >> 4) + (y >> 4)) * int32_t(pitch >> 5)) + (x >> 5)) << 7;
  int32_t inner = ((z & 3) << 5) | (((y >> 1) & 3) << 3) | (x & 7);
  int64_t oi = int64_t(outer | inner) << bpb_log2;
  uint32_t bank = ((y >> 3) ^ (z >> 2)) & 1, pipe = ((x >> 3) & 3) ^ (bank << 1);
  return int64_t((y & 1) << 4 | pipe << 6 | bank << 11) | (oi & 15) | (((oi >> 4) & 1) << 5) |
         (((oi >> 5) & 7) << 8) | (oi >> 8 << 12);
}

// The guest address of block (x, y, z) of array slice `slice` of a level.
uint32_t BlockAddress(const Layout& l, const Storage& st, uint32_t x, uint32_t y, uint32_t z, uint32_t slice) {
  const Format& fm = kFormats[l.format];
  x += st.ox;
  y += st.oy;
  z += st.oz;
  uint32_t at = st.address + slice * st.slice_bytes;
  if (!l.tiled) return at + (z * st.rows + y) * st.pitch_blocks * fm.bytes + x * fm.bytes;
  uint32_t bpb_log2 = Log2Floor(fm.bytes);
  if (l.dimension == 2)
    return at + uint32_t(Tiled3D(int32_t(x), int32_t(y), int32_t(z), st.pitch_blocks, st.rows, bpb_log2));
  return at + uint32_t(Tiled2D(int32_t(x), int32_t(y), st.pitch_blocks, bpb_log2));
}

// ---- the cache ---------------------------------------------------------------
// A texture's image and what it holds (a choice: no write tracking):
// - stale: it must be decoded from memory before it is sampled (new, or
//   a resolve wrote its memory and it did not take the texels itself);
// - decoded, hash known: it holds what memory held when hashed;
// - updated by resolves still on the device, hash unknown: it holds what
//   memory will hold when they land.
// Events: a resolve into its memory updates it on the device where it
// matches (TexturesResolved; hash unknown) unless it is stale, else
// makes it stale. The resolves landing hash the memory of those updated
// (TexturesLanded; hash known). Its first use in a frame (a swap) with
// the hash known compares it with its memory's, unless a resolve still
// on the device wrote there: a difference makes it stale. A stale
// texture lands what it reads, decodes and hashes. So a CPU write is
// seen at the texture's first use in a later frame, except one between
// an update by a resolve and the resolve landing outside the bytes the
// resolve writes back (inside them, memory itself loses it).
struct Texture {
  gfx::TextureId image = 0;
  std::vector<std::pair<uint32_t, uint32_t>> ranges;  // guest physical [lo, hi)
  uint64_t hash = 0, hashed_frame = UINT64_MAX, used_frame = 0;
  bool stale = true, hash_known = false;
  // Whether resolves can update it in place (TexturesResolved): its level
  // 0 at base, pitch texels a row, in format, with endian swap endian.
  bool in_place = false;
  uint32_t base = 0, pitch = 0, format = 0, endian = 0;
  int32_t w = 0, h = 0;
};
std::map<std::array<uint32_t, 6>, std::unique_ptr<Texture>> g_textures;
gfx::TextureId g_null[4];  // by gfx::TextureKind: (0, 0, 0, 0)
uint64_t g_swept_frame;

// What of a fetch constant the decode depends on: not the clamps, filters,
// swizzle, exponent, LOD bias or border.
std::array<uint32_t, 6> Key(const Fetch& f) {
  return {f.w[0] & 0xFFC003FF, f.w[1], f.w[2], 0, f.w[4] & 0x3FC, f.w[5] & 0xFFFFFE00};
}

// A fast 64-bit hash of guest memory (not cryptographic: a change to the
// bytes changes it with overwhelming likelihood): four independent lanes
// of 8-byte words, big ranges in parts on several threads, combined.
uint64_t Mix(uint64_t h, uint64_t v) {
  h = (h ^ (v * 0x87C37B91114253D5ull)) * 0x4CF5AD432745937Full;
  return h ^ (h >> 31);
}
uint64_t HashBytes(const uint8_t* p, size_t n) {
  uint64_t lane[4] = {0x9E3779B97F4A7C15ull, 0xC2B2AE3D27D4EB4Full, 0x165667B19E3779F9ull, 0x27D4EB2F165667C5ull};
  size_t k = 0;
  for (; k + 32 <= n; k += 32)
    for (uint32_t i = 0; i < 4; i++) {
      uint64_t v;
      std::memcpy(&v, p + k + 8 * i, 8);
      lane[i] = Mix(lane[i], v);
    }
  uint64_t h = Mix(Mix(Mix(lane[0], lane[1]), lane[2]), lane[3]);
  for (; k < n; k++) h = (h ^ p[k]) * 0x100000001B3ull;
  return Mix(h, n);
}
uint64_t Hash(const std::vector<std::pair<uint32_t, uint32_t>>& ranges) {
  uint64_t h = 0;
  for (auto [lo, hi] : ranges) {
    const uint8_t* p = Mem() + PhysicalToVirtual(lo);
    size_t n = hi - lo;
    uint32_t parts = n >= (1u << 20) ? 8 : 1;
    uint64_t part[8];
    std::vector<std::thread> pool;
    for (uint32_t k = 1; k < parts; k++)
      pool.emplace_back([&, k] { part[k] = HashBytes(p + n * k / parts, n * (k + 1) / parts - n * k / parts); });
    part[0] = HashBytes(p, n / parts);
    for (std::thread& th : pool) th.join();
    for (uint32_t k = 0; k < parts; k++) h = Mix(h, part[k]);
  }
  return h;
}

gfx::Format HostFormat(HostClass c) {
  switch (c) {
    case kRgba8: return gfx::Format::R8G8B8A8;
    case kRgba16: return gfx::Format::R16G16B16A16;
    case kRgba16Snorm: return gfx::Format::R16G16B16A16Snorm;
    case kRgba16Float: return gfx::Format::A16B16G16R16F;
    default: return gfx::Format::A32B32G32R32F;
  }
}
uint32_t HostBytes(HostClass c) { return c == kRgba8 ? 4 : c == kRgba32Float ? 16 : 8; }

void Encode(HostClass c, const float v[4], uint8_t* out) {
  for (uint32_t k = 0; k < 4; k++) {
    float x = v[k];
    switch (c) {
      case kRgba8: out[k] = uint8_t(std::clamp(x, 0.0f, 1.0f) * 255 + 0.5f); break;
      case kRgba16: {
        uint16_t u = uint16_t(std::clamp(x, 0.0f, 1.0f) * 65535 + 0.5f);
        std::memcpy(out + 2 * k, &u, 2);
        break;
      }
      case kRgba16Snorm: {
        float v = std::clamp(x, -1.0f, 1.0f) * 32767;
        int16_t s = int16_t(v >= 0 ? v + 0.5f : v - 0.5f);
        std::memcpy(out + 2 * k, &s, 2);
        break;
      }
      case kRgba16Float: {
        uint16_t h = ToHalf(x);
        std::memcpy(out + 2 * k, &h, 2);
        break;
      }
      default: std::memcpy(out + 4 * k, &x, 4);
    }
  }
}

// Formats whose texels, unsigned, are their host texels' bytes (the
// missing components repeated): copied without converting; and
// 2_10_10_10 to 16-bit unorm rounded to nearest in integers (resolves'
// texture updates on the device, kUpdate in edram.cpp, give the same).
bool CopyTexel(uint32_t f, const uint8_t* b, uint8_t* out) {
  uint32_t size;
  switch (f) {
    case 7: case 54: {
      uint32_t v;
      std::memcpy(&v, b, 4);
      uint16_t c[4];
      for (uint32_t k = 0; k < 3; k++) c[k] = uint16_t(((v >> (10 * k) & 0x3FF) * 65535 + 511) / 1023);
      c[3] = uint16_t((v >> 30) * 21845);
      std::memcpy(out, c, 8);
      return true;
    }
    case 2: case 8: std::memset(out, b[0], 4); return true;
    case 6: case 14: case 50: std::memcpy(out, b, 4); return true;
    case 26: case 29: case 32: std::memcpy(out, b, 8); return true;
    case 38: std::memcpy(out, b, 16); return true;
    case 24: case 27: case 30: size = 2; break;
    case 25: case 28: case 31: std::memcpy(out, b, 4); out += 4; b += 2; size = 2; break;
    case 36: size = 4; break;
    case 37: std::memcpy(out, b, 8); out += 8; b += 4; size = 4; break;
    default: return false;
  }
  uint32_t bytes = f == 36 ? 16 : f == 37 || f == 24 || f == 27 || f == 30 ? 8 : 4;
  for (uint8_t* end = out + bytes; out < end; out += size) std::memcpy(out, b, size);
  return true;
}

// The guest physical ranges of a texture's levels in use.
std::vector<std::pair<uint32_t, uint32_t>> Ranges(const Layout& l) {
  std::vector<std::pair<uint32_t, uint32_t>> r;
  uint32_t array = l.dimension == 2 ? 1 : l.layers;
  for (uint32_t level = l.mip_min; level <= l.mip_max; level++) {
    if (level == 0 ? !l.base : !l.mips) continue;
    Storage st = LevelStorage(l, level);
    if (uint64_t(st.address) + uint64_t(st.slice_bytes) * array > kPhysicalSize)
      Fatal(nullptr, "GPU: a %s texture at %x, outside physical memory", kFormats[l.format].name, st.address);
    r.push_back({st.address, st.address + st.slice_bytes * array});
  }
  return r;
}

// The texture formats whose texels are a resolve's copy's words as they
// are, by the copy's texture format.
bool TakesCopy(uint32_t copy, uint32_t format) {
  switch (copy) {
    case 6: case 14: case 50: return format == 6 || format == 14 || format == 50;
    case 7: case 54: return format == 7 || format == 54;
    case 22: return format == 22 || format == 23;
    case 31: return format == 28 || format == 31;
    case 32: return format == 29 || format == 32;
    case 25: case 26: case 36: case 37: return format == copy;
    default: return false;
  }
}

// Decodes a fetch constant's texture into a new host image.
void Decode(Texture& t, const Fetch& f) {
  Layout l = GetLayout(f);
  const Format& fm = kFormats[l.format];
  if (!fm.bytes) Fatal(nullptr, "GPU: texture format %s (not implemented yet)", fm.name);
  if (l.tiled && (fm.bytes & (fm.bytes - 1)))
    Fatal(nullptr, "GPU: a tiled %s texture (not implemented yet)", fm.name);
  if (l.dimension == 0 && (l.tiled || l.packed_level != UINT32_MAX))
    Fatal(nullptr, "GPU: a tiled or packed 1D texture");
  bool any_signed = false;
  for (uint32_t c = 0; c < 4; c++) any_signed |= (f.signs() >> (2 * c) & 3) == 1;
  if (any_signed && (Compressed(l.format) || l.format == 11 || l.format == 12))
    Fatal(nullptr, "GPU: a signed %s texture (not implemented yet)", fm.name);
  HostClass hc = fm.host;
  if (any_signed && !FloatFormat(l.format)) hc = kRgba16Snorm;
  gfx::Device& dev = HostDevice();
  if (t.image) dev.DestroyTexture(t.image);
  gfx::ImageDesc d;
  d.w = int(l.width);
  d.h = int(l.height);
  d.levels = int(l.mip_max + 1);
  d.fmt = HostFormat(hc);
  d.kind = l.dimension == 3 ? gfx::TextureKind::Cube
           : l.dimension == 2 ? gfx::TextureKind::Tex3D
           : f.stacked()      ? gfx::TextureKind::Tex2DArray
                              : gfx::TextureKind::Tex2D;
  d.layers = int(l.dimension == 2 ? l.depth : l.layers);
  // A 2D tiled texture of one level in a format resolves write (signs
  // only for the float formats, which ignore them), which resolves can
  // update in place: a colour target image too.
  uint32_t written = l.format == 23 ? 22 : l.format == 28 ? 31 : l.format == 29 ? 32 : l.format;
  t.in_place = d.kind == gfx::TextureKind::Tex2D && l.dimension == 1 && l.tiled && l.base && !l.mip_max &&
               l.packed_level && (!any_signed || FloatFormat(l.format)) && TakesCopy(written, l.format);
  d.color_target = t.in_place;
  t.base = l.base;
  t.pitch = l.pitch_texels;
  t.format = l.format;
  t.endian = f.endian();
  t.w = d.w;
  t.h = d.h;
  t.image = dev.CreateImage(d);
  if (!t.image) Fatal(nullptr, "GPU: no host image for a %ux%u %s texture", l.width, l.height, fm.name);
  t.ranges = Ranges(l);
  uint32_t array = l.dimension == 2 ? 1 : l.layers, hb = HostBytes(hc);
  uint8_t probe[16] = {}, sink[16];
  bool copy = !any_signed && CopyTexel(l.format, probe, sink);
  const uint8_t* mem = Mem() + PhysicalToVirtual(0);
  uint32_t words = std::max(fm.bytes / 4u, 1u);
  for (uint32_t level = l.mip_min; level <= l.mip_max; level++) {
    if (level == 0 ? !l.base : !l.mips) continue;
    Storage st = LevelStorage(l, level);
    uint32_t w = std::max(l.width >> level, 1u), h = std::max(l.height >> level, 1u);
    uint32_t depth = l.dimension == 2 ? std::max(l.depth >> level, 1u) : 1;
    uint32_t bw = (w + fm.bw - 1) / fm.bw, bh = (h + fm.bh - 1) / fm.bh;
    std::vector<uint8_t> image(size_t(w) * h * hb);
    for (uint32_t slice = 0; slice < array; slice++) {
      for (uint32_t z = 0; z < depth; z++) {
        // Rows of blocks decoded in parallel for big levels (they read
        // guest memory and write their own rows).
        auto rows = [&](uint32_t first, uint32_t end) {
          float block[16][4];
          for (uint32_t by = first; by < end; by++) {
            for (uint32_t bx = 0; bx < bw; bx++) {
              uint32_t pa = BlockAddress(l, st, bx, by, z, slice);
              // The block's 32-bit words little-endian (as the host loads
              // them), endian swapped.
              uint8_t bytes[16];
              for (uint32_t k = 0; k < words; k++) {
                uint32_t word;
                std::memcpy(&word, mem + (pa & ~3u) + 4 * k, 4);
                word = GpuSwap(word, f.endian()) >> 8 * (pa & 3);
                std::memcpy(bytes + 4 * k, &word, 4);
              }
              if (copy) {
                uint32_t x = bx, y = by;
                if (x < w && y < h) CopyTexel(l.format, bytes, &image[(size_t(y) * w + x) * hb]);
                continue;
              }
              DecodeBlock(l.format, bytes, f.signs(), block);
              for (uint32_t ty = 0; ty < fm.bh; ty++)
                for (uint32_t tx = 0; tx < fm.bw; tx++) {
                  uint32_t x = bx * fm.bw + tx, y = by * fm.bh + ty;
                  if (x < w && y < h) Encode(hc, block[ty * fm.bw + tx], &image[(size_t(y) * w + x) * hb]);
                }
            }
          }
        };
        uint32_t threads = w * h >= 65536 ? std::min(8u, std::max(1u, std::thread::hardware_concurrency() / 2)) : 1;
        std::vector<std::thread> pool;
        for (uint32_t k = 1; k < threads; k++) pool.emplace_back(rows, bh * k / threads, bh * (k + 1) / threads);
        rows(0, bh / threads);
        for (std::thread& th : pool) th.join();
        uint32_t layer = l.dimension == 2 ? z : slice;
        dev.UpdateImage(t.image, int(level), int(layer), {0, 0, int(w), int(h)}, image.data(), w * hb);
      }
    }
  }
}

void Sweep(uint64_t frame) {
  if (frame == g_swept_frame) return;
  g_swept_frame = frame;
  for (auto it = g_textures.begin(); it != g_textures.end();) {
    if (frame - it->second->used_frame > kEvictFrames) {
      HostDevice().DestroyTexture(it->second->image);
      it = g_textures.erase(it);
    } else {
      ++it;
    }
  }
}

bool Pending(const std::vector<std::pair<uint32_t, uint32_t>>& ranges) {
  for (auto [lo, hi] : ranges)
    if (ResolvePending(lo, hi)) return true;
  return false;
}

gfx::TextureId Null(gfx::TextureKind kind) {
  gfx::TextureId& id = g_null[uint32_t(kind)];
  if (id) return id;
  gfx::ImageDesc d;
  d.kind = kind;
  d.layers = kind == gfx::TextureKind::Cube ? 6 : 1;
  id = HostDevice().CreateImage(d);
  const uint8_t zero[4] = {};
  for (int layer = 0; layer < d.layers; layer++) HostDevice().UpdateImage(id, 0, layer, {0, 0, 1, 1}, zero, 4);
  return id;
}

}  // namespace

TextureBinding BindTexture(const ShaderTexture& st) {
  Fetch f;
  for (uint32_t k = 0; k < 6; k++) f.w[k] = GpuRegister(kRegShaderConstantFetch + 6 * st.constant + k);
  uint64_t frame = SwapCount();
  Sweep(frame);
  TextureBinding b;
  // The image kinds of the instruction's dimension (0 1D, 1 2D, 2 3D or
  // stacked, 3 cube) and of the constant's (0 1D, 1 2D or stacked, 2 3D,
  // 3 cube).
  static const gfx::TextureKind kKinds[4] = {gfx::TextureKind::Tex2D, gfx::TextureKind::Tex2D,
                                             gfx::TextureKind::Tex3D, gfx::TextureKind::Cube};
  gfx::TextureKind want = kKinds[st.dimension];
  b.stacked = st.dimension == 2 && f.dimension() == 1 && f.stacked();
  if (b.stacked) want = gfx::TextureKind::Tex2DArray;
  gfx::TextureKind have = f.dimension() == 3   ? gfx::TextureKind::Cube
                          : f.dimension() == 2 ? gfx::TextureKind::Tex3D
                          : f.stacked()        ? gfx::TextureKind::Tex2DArray
                                               : gfx::TextureKind::Tex2D;
  // A constant that is not a texture, has no memory, or is of another kind
  // than the instruction samples: the null texture, zeros (Xenia's).
  Layout l = GetLayout(f);
  if (f.type() != 2 || (!l.base && !l.mips) || have != want) {
    b.image = Null(want);
    return b;
  }
  std::unique_ptr<Texture>& t = g_textures[Key(f)];
  if (!t) t = std::make_unique<Texture>();
  // Its memory checked at its first use in a frame (Texture says how).
  if (!t->stale && t->hash_known && t->hashed_frame != frame) {
    t->hashed_frame = frame;
    if (!Pending(t->ranges)) t->stale = Hash(t->ranges) != t->hash;
  }
  if (t->stale) {
    for (auto [lo, hi] : Ranges(l)) LandResolvesLocked(lo, hi);
    Decode(*t, f);
    t->hash = Hash(t->ranges);
    t->hash_known = true;
    t->hashed_frame = frame;
    t->stale = false;
  }
  t->used_frame = frame;
  b.image = t->image;

  // The sampler: the instruction's filters where it sets them, else the
  // constant's (0 point, 1 linear, 2 the base map for mips); clamps per
  // axis (those of axes the dimension has; mirror clamps as mirrored
  // repeat, clamp to halfway as clamp to edge, as Xenia does on a host
  // without mirror-clamp); the constant's LOD bias in 1/32; mips from
  // mip_min_level; anisotropy 1-16.
  auto filter = [](uint32_t instr, uint32_t constant) { return instr == 3 ? constant : instr; };
  gfx::SamplerDesc& s = b.sampler;
  s.mag_linear = filter(st.mag, f.mag()) == 1;
  s.min_linear = filter(st.min, f.min()) == 1;
  uint32_t mip = filter(st.mip, f.mip());
  s.mip_linear = mip == 1;
  s.min_lod = float(l.mip_min);
  s.max_lod = mip == 2 ? s.min_lod + 0.25f : 1000.0f;
  s.lod_bias = f.lod_bias();
  uint32_t aniso = st.aniso == 7 ? f.aniso() : st.aniso;
  s.max_anisotropy = aniso >= 1 && aniso <= 5 ? float(1u << (aniso - 1)) : 1.0f;
  gfx::AddressMode* modes[3] = {&s.u, &s.v, &s.w};
  uint32_t axes = f.dimension() == 3 ? 0 : f.dimension() == 2 ? 3 : f.dimension() == 1 ? 2 : 1;
  for (uint32_t a = 0; a < 3; a++) {
    uint32_t c = a < axes ? f.clamp(a) : 2;
    static const gfx::AddressMode kModes[6] = {gfx::AddressMode::Repeat, gfx::AddressMode::MirroredRepeat,
                                               gfx::AddressMode::ClampToEdge, gfx::AddressMode::MirroredRepeat,
                                               gfx::AddressMode::ClampToEdge, gfx::AddressMode::MirroredRepeat};
    if (c == 6) Fatal(nullptr, "GPU: a texture clamped to its border (the host device has no border colour)");
    *modes[a] = kModes[c == 7 ? 5 : c];
  }
  // xe_sample's values, and linear filtering between a stacked texture's
  // layers where the constant's magnifying volume filter says so (for
  // minifying too: no gradients here; Xenia picks by them).
  b.tp[0] = int32_t(f.swizzle());
  b.tp[1] = int32_t(f.signs());
  b.tp[2] = f.exp_adjust();
  b.tp[3] = b.stacked && f.vol_mag() ? kTextureStackedLinear : 0;
  return b;
}

void TexturesLanded() {
  for (auto& [key, t] : g_textures)
    if (!t->stale && !t->hash_known) {
      t->hash = Hash(t->ranges);
      t->hash_known = true;
    }
}

std::vector<TextureUpdate> TexturesResolved(uint32_t lo, uint32_t hi, int32_t x0, int32_t y0, int32_t x1, int32_t y1,
                                            uint32_t base, uint32_t pitch, uint32_t format, uint32_t endian) {
  std::vector<TextureUpdate> updates;
  for (auto& [key, t] : g_textures) {
    bool touched = false;
    for (auto [a, b] : t->ranges) touched |= a < hi && lo < b;
    if (!touched) continue;
    // The copy's base is the texture's, or that of one of its 32x32
    // blocks (the copy then writes the texture's texels that far right and
    // down, its rectangle within the rows).
    uint32_t block = 1024 * kFormats[t->format].bytes, delta = base - t->base, blocks = delta / block,
             row = t->pitch / 32;
    int32_t dx = int32_t(32 * (blocks % row)), dy = int32_t(32 * (blocks / row));
    if (!t->stale && t->in_place && base >= t->base && delta % block == 0 && t->pitch == pitch &&
        t->endian == endian && TakesCopy(format, t->format) && x0 >= 0 && y0 >= 0 && x1 + dx <= int32_t(pitch)) {
      updates.push_back({t->image, t->format, t->w, t->h, dx, dy});
      t->hash_known = false;
    } else {
      t->stale = true;
    }
  }
  return updates;
}

}  // namespace recomp::xbox360
