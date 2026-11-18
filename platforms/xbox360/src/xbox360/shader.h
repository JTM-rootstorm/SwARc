#pragma once
// Xenos shader microcode translated to GLSL (docs/xenos.md, "Shaders"),
// for the host device's programs. The result is the shader as a function,
// xe_shader(), over globals; the renderer puts a main around it that
// loads its inputs and stores its outputs, and with what the function
// reads: vec4 xe_c(int i), the stage's float constant i (i below
// `constants`); and the uniform block:
//   uvec4 xe_bool[2] the 256 bool constants; uvec4 xe_loop[8] the 32
//                    loop constants;
//   uvec4 xe_vf[]    per vertex buffer (ShaderInfo::vertex_buffers, read
//                    with r_ld): the endian swap, the first word of it
//                    given, how many (fetches outside them read 0);
//   ivec4 xe_tp[]    per texture (ShaderInfo::textures, bound as xe_t<n>,
//                    which the function's source declares): what to do
//                    with a sample (xe_sample: swizzle, signs, exponent
//                    bias; the renderer's flags in w).
// A texture of dimension 2 (3D or stacked, which the fetch constant says)
// is the renderer's to declare as xe_t<n>, with the functions the source
// calls: vec3 xe_t<n>_size(), vec4 xe_t<n>_lod(vec3, float),
// xe_t<n>_grad(vec3, vec3, vec3) and, in a pixel shader,
// xe_t<n>_bias(vec3, float) and float xe_t<n>_qlod(vec3).
// Globals: vec4 xe_r[64] (a vertex shader's r0.x is the vertex index, a
// pixel shader's first registers the interpolators), then a vertex
// shader's xe_pos, xe_pts (point size, edge flag, kill) and xe_i[16], a
// pixel shader's xe_oc[4] and xe_od (depth in x).

#include <cstdint>
#include <string>
#include <vector>

namespace recomp::xbox360 {

struct ShaderTexture {
  uint32_t constant;   // texture fetch constant 0-31
  uint32_t dimension;  // 0 1D, 1 2D, 2 3D or stacked, 3 cube
  // Filters the instruction sets instead of the fetch constant's (the
  // instruction's fields: 3 for a filter, 7 for aniso, keep the
  // constant's).
  uint32_t mag, min, mip, aniso;
  bool operator==(const ShaderTexture&) const = default;
};

struct ShaderVertexBuffer {
  uint32_t constant;  // vertex fetch constant 0-95
  // A vertex's fetches read words [stride * index, stride * index +
  // extent) of it, the index the vertex's; 0 stride when they read
  // others (an index from elsewhere, strides that disagree).
  uint32_t stride, extent;
};

struct ShaderInfo {
  bool pixel = false;
  std::string glsl;
  uint32_t constants = 0;                 // float constants read, from 0
  std::vector<ShaderVertexBuffer> vertex_buffers;
  std::vector<ShaderTexture> textures;
  uint32_t interpolators = 0;  // a vertex shader's, written (a bit each)
  uint32_t colors = 0;         // a pixel shader's targets written
  bool depth = false, kills = false;
  // A vertex shader writes xe_pts.x (the point size), xe_pts.z (vertex
  // kill).
  bool point_size = false, vertex_kill = false;
};

// The microcode's `count` words as the GPU reads them. Anything outside
// what is translated is fatal and names it.
ShaderInfo TranslateShader(const uint32_t* words, uint32_t count, bool pixel);

// GLSL the translated functions use, for both stages, after the uniform
// block and before the function's source.
extern const char kShaderCommon[];

}  // namespace recomp::xbox360
