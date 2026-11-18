#pragma once
// The Xenos's textures on the host GPU (docs/xenos.md, "Textures"): a
// texture fetch constant's texture decoded from guest memory into a host
// image, kept while guest memory holds the same, and its sampler. From
// the command processor's thread, the display device's lock held.

#include <cstdint>
#include <vector>

#include "runtime/gfx/device.h"
#include "xbox360/shader.h"

namespace recomp::xbox360 {

// What a draw binds for a shader's texture: the host image (or the null
// texture of the kind), its sampler, xe_tp's values (swizzle, signs,
// exponent bias, flags) and, for dimension 2, whether it is a stacked 2D
// texture rather than a 3D one.
struct TextureBinding {
  gfx::TextureId image = 0;
  gfx::SamplerDesc sampler;
  int32_t tp[4] = {};
  bool stacked = false;
};
// The constant's flags in tp[3]: a stacked texture filtered linearly
// between its layers.
constexpr int32_t kTextureStackedLinear = 1;

TextureBinding BindTexture(const ShaderTexture& t);

// A resolve's copy writes [lo, hi), texels [x0, x1) x [y0, y1) of a tiled
// texture at physical `base`, `pitch` texels a row, in texture format
// `format` with endian swap `endian` (the display device's lock held). The textures there whose
// level 0 is that texture (or holds it from one of its 32x32 blocks on),
// in a format that decodes the copy's words as they are, are returned to
// take its texels on the device (in the image `image`, w x h, decoded as
// `format`, the copy's texel (x, y) at (x + dx, y + dy)); the others
// decode again before they are next sampled.
struct TextureUpdate {
  gfx::TextureId image;
  uint32_t format;
  int32_t w, h, dx, dy;
};
std::vector<TextureUpdate> TexturesResolved(uint32_t lo, uint32_t hi, int32_t x0, int32_t y0, int32_t x1, int32_t y1,
                                            uint32_t base, uint32_t pitch, uint32_t format, uint32_t endian);
// The resolves landed (the display device's lock held): the memory of
// textures they updated is hashed.
void TexturesLanded();

}  // namespace recomp::xbox360
