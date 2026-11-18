#pragma once
// The Xenos's textures on the host GPU (docs/xenos.md, "Textures"): a
// texture fetch constant's texture decoded from guest memory into a host
// image, kept while guest memory holds the same, and its sampler. From
// the command processor's thread, the display device's lock held.

#include <cstdint>

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

// Guest physical memory [lo, hi) was written by the GPU (a resolve):
// textures there decode again before they are next sampled.
void InvalidateTextures(uint32_t lo, uint32_t hi);

}  // namespace recomp::xbox360
