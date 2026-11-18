// The Xenos's draws on the host GPU (render.h). The vertex position's
// path to host clip space, the rectangle list's fourth vertex, the
// render state's encodings and conversions and the draw packets' fields
// are Xenia's (BSD-3: its draw_util.cc, shader translators, pipeline
// caches, render target caches, primitive processor and command
// processor); docs/xenos.md, "Drawing", says what is taken from where.
//
// xe_gamma is ported from Xenia's LinearToPWLGamma (its shader
// translators):
// Copyright (c) 2015, Ben Vanik. All rights reserved. BSD 3-Clause; the
// licence is in NOTICE.

#include "xbox360/render.h"

#include <algorithm>
#include <atomic>
#include <cmath>
#include <cstring>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <tuple>
#include <unordered_map>
#include <vector>

#include "ppc/fatal.h"
#include "runtime/gfx/device.h"
#include "xbox360/display.h"
#include "xbox360/edram.h"
#include "xbox360/gpu.h"
#include "xbox360/hle.h"
#include "xbox360/memory.h"
#include "xbox360/shader.h"
#include "xbox360/texture.h"

namespace recomp::xbox360 {
namespace {

using ppc::Fatal;

// Registers draws read.
constexpr uint32_t kRegRbSurfaceInfo = 0x2000, kRegRbDepthInfo = 0x2002, kRegScreenScissorTl = 0x200E,
                   kRegScreenScissorBr = 0x200F, kRegWindowOffset = 0x2080,
                   kRegWindowScissorTl = 0x2081, kRegWindowScissorBr = 0x2082,
                   kRegVgtIndxOffset = 0x2102, kRegRbColorMask = 0x2104, kRegRbBlendRed = 0x2105,
                   kRegRbStencilRefMaskBf = 0x210C, kRegRbStencilRefMask = 0x210D,
                   kRegRbAlphaRef = 0x210E, kRegPaClVportXscale = 0x210F,
                   kRegSqProgramCntl = 0x2180, kRegRbDepthControl = 0x2200,
                   kRegRbColorControl = 0x2202, kRegPaClClipCntl = 0x2204,
                   kRegPaSuScModeCntl = 0x2205, kRegPaClVteCntl = 0x2206,
                   kRegPaSuVtxCntl = 0x2302, kRegPaSuPolyOffsetFrontScale = 0x2380,
                   kRegAluConstants = 0x4000, kRegBoolConstants = 0x4900,
                   kRegLoopConstants = 0x4908, kRegVgtMinVtxIndx = 0x2101,
                   kRegVgtMultiPrimIbResetIndx = 0x2103, kRegSqContextMisc = 0x2181,
                   kRegPaSuPointSize = 0x2280, kRegPaSuPointMinmax = 0x2281;
// Each colour target's RB_COLOR_INFO and RB_BLENDCONTROL.
constexpr uint32_t kRegRbColorInfo[4] = {0x2001, 0x2003, 0x2004, 0x2005};
constexpr uint32_t kRegRbBlendControl[4] = {0x2201, 0x2209, 0x220A, 0x220B};
// RB_MODECONTROL's EDRAM modes a draw draws in.
constexpr uint32_t kEdramModeIgnore = 0, kEdramModeColorDepth = 4, kEdramModeDepth = 5;
// VGT_DRAW_INITIATOR's primitive types and index sources.
constexpr uint32_t kPrimPointList = 1, kPrimLineList = 2, kPrimLineStrip = 3, kPrimTriangleList = 4,
                   kPrimTriangleFan = 5, kPrimTriangleStrip = 6, kPrimRectangleList = 8, kPrimLineLoop = 12,
                   kPrimQuadList = 13;
constexpr uint32_t kSourceDma = 0, kSourceImmediate = 1, kSourceAuto = 2;
// How the host draws a primitive type: a list of the guest's indices; a
// list each of whose host vertices runs its primitive's guest vertices
// (the vertex shader kills vertices; lines or triangles); rectangles and
// point sprites expanded from their guest vertices.
enum Kind { kKindList, kKindKillLines, kKindKillTriangles, kKindRectangles, kKindPoints };
// Depth formats (RB_DEPTH_INFO bit 16).
constexpr uint32_t kDepthD24S8 = 0;

float RegFloat(uint32_t r) {
  uint32_t v = GpuRegister(r);
  float f;
  std::memcpy(&f, &v, 4);
  return f;
}

int32_t Signed(uint32_t v, uint32_t bits) { return int32_t(v << (32 - bits)) >> (32 - bits); }

// ---- shaders ---------------------------------------------------------------
// A loaded shader's microcode, translated at its first draw.
struct Shader {
  bool pixel;
  std::vector<uint32_t> words;
  bool translated = false;
  ShaderInfo info;
};
// By the stage and the microcode's words (the key's hash is theirs).
std::unordered_map<std::string, std::unique_ptr<Shader>> g_shaders;
Shader *g_vertex_shader, *g_pixel_shader;

const ShaderInfo& Info(Shader& s) {
  if (!s.translated) {
    s.info = TranslateShader(s.words.data(), uint32_t(s.words.size()), s.pixel);
    s.translated = true;
  }
  return s.info;
}

// The uniform block of a stage (shader.h's, then the system values the
// mains read), std140, and its bytes. The float constants are not in it:
// SDL_GPU's Vulkan backend binds 4 KiB of a uniform block, which 256
// constants fill, and reads past it as zeros.
std::string UniformGlsl(const ShaderInfo& s) {
  std::string g = "layout(R_UNIFORMS) uniform XeUniforms {\n";
  g += "  uvec4 xe_bool[2];\n  uvec4 xe_loop[8];\n";
  g += "  uvec4 xe_vf[" + std::to_string(std::max<size_t>(1, s.vertex_buffers.size())) + "];\n";
  g += "  ivec4 xe_tp[" + std::to_string(std::max<size_t>(1, s.textures.size())) + "];\n";
  // System values: NDC scale and offset (xyz); PA_CL_VTE_CNTL,
  // VGT_INDX_OFFSET, whether a rectangle or point list's indices are in
  // the last buffer, the alpha test's function; per colour target its
  // output's scale and whether it is gamma; the alpha test's reference;
  // points' constant diameter (xy) and the per-vertex one's bounds, in
  // pixels, and a diameter in pixels as a radius in NDC; vertex kill when
  // any vertex is killed (else all), the register pixel parameters go to
  // (-1 none), the primitive (0 polygon, 1 line, 2 point); the index
  // bounds VGT_MIN_VTX_INDX and VGT_MAX_VTX_INDX, the sample shifts.
  g += "  vec4 xe_ndc_scale, xe_ndc_offset;\n  ivec4 xe_sys;\n  vec4 xe_color_scale;\n"
       "  ivec4 xe_color_gamma;\n  vec4 xe_alpha_ref;\n  vec4 xe_point, xe_point_ndc;\n  ivec4 xe_prim;\n"
       "  uvec4 xe_index;\n};\n";
  return g;
}
constexpr uint32_t kSysWords = 40;
uint32_t UniformBytes(const ShaderInfo& s) {
  return uint32_t(16 * (2 + 8 + std::max<size_t>(1, s.vertex_buffers.size()) +
                        std::max<size_t>(1, s.textures.size())) +
                  4 * kSysWords);
}

// A stage's float constants (shader.h's xe_c): read-only buffer `b`, four
// words a constant.
std::string ConstantGlsl(const ShaderInfo& s, uint32_t b) {
  if (!s.constants) return "";
  std::string n = std::to_string(b) + "u";
  return "vec4 xe_c(int i) {\n  uint k = 4u * uint(i);\n  return uintBitsToFloat(uvec4(r_ld(" + n + ", k), r_ld(" + n +
         ", k + 1u), r_ld(" + n + ", k + 2u), r_ld(" + n + ", k + 3u)));\n}\n";
}

// The globals shader.h leaves to the renderer, per stage.
const char kVertexGlobals[] = "vec4 xe_pos, xe_pts;\nvec4 xe_i[16];\n";
const char kPixelGlobals[] = "vec4 xe_oc[4];\nvec4 xe_od;\n";

// The vertex shader run for one guest index: the index offset by
// VGT_INDX_OFFSET in 24 bits and clamped to VGT_MIN/MAX_VTX_INDX (Xenia's
// Direct3D 12 path); xe_pts starts with -1 for the point size and no
// kill. Its position in host clip space: W is 1/W unless PA_CL_VTE_CNTL's
// vtx_w0_fmt (bit 10) says it is W; XY and Z are divided by W where
// vtx_xy_fmt (8) and vtx_z_fmt (9) say so, and multiplied back; then the
// viewport, window and half-pixel offsets map window pixels onto the host
// target (Xenia's vertex epilogue). A vertex is killed where xe_pts.z has
// bits other than the sign (Xenia's test); a primitive is dropped when
// any (PA_CL_CLIP_CNTL's vtx_kill_or, bit 21) or all of its vertices are,
// its vertices put outside the clip volume (Xenia makes the position NaN
// or uses a cull distance; this has their effect, defined).
const char kVertexRun[] = R"(
vec4 xe_vertex(uint raw) {
  uint index = clamp((raw + uint(xe_sys.y)) & 0xFFFFFFu, xe_index.x, xe_index.y);
  for (int k = 0; k < 64; k++) xe_r[k] = vec4(0.0);
  xe_r[0].x = float(index);
  xe_ps = 0.0;
  xe_lod = 0.0;
  xe_p0 = false;
  xe_a0 = 0;
  xe_al = 0;
  xe_ldepth = 0;
  xe_rdepth = 0;
  xe_gh = vec3(0.0);
  xe_gv = vec3(0.0);
  xe_pos = vec4(0.0, 0.0, 0.0, 1.0);
  xe_pts = vec4(-1.0, 0.0, 0.0, 0.0);
  for (int k = 0; k < 16; k++) xe_i[k] = vec4(0.0);
  xe_shader();
  vec4 p = xe_pos;
  float w = (xe_sys.x & 0x400) != 0 ? p.w : 1.0 / p.w;
  if ((xe_sys.x & 0x100) != 0) p.xy *= w;
  if ((xe_sys.x & 0x200) != 0) p.z *= w;
  return vec4(p.xyz * xe_ndc_scale.xyz + xe_ndc_offset.xyz * w, w);
}
bool xe_killed() { return (floatBitsToUint(xe_pts.z) & 0x7FFFFFFFu) != 0u; }
bool xe_dropped(int killed, int n) { return xe_prim.x != 0 ? killed > 0 : killed == n; }
const vec4 XE_DROPPED = vec4(2.0, 2.0, 0.0, 1.0);
)";

// A list's main: the host draws its guest indices.
const char kListMain[] = R"(
void main() {
  gl_Position = xe_vertex(uint(gl_VertexIndex));
  xe_emit(xe_i);
  xe_pc = vec2(0.0);
}
)";

// A list whose vertex shader kills vertices: each host vertex runs its
// primitive's XE_N vertices (guest indices in buffer XE_INDICES) to know
// whether the primitive is dropped.
const char kKillMain[] = R"(
void main() {
  int p = gl_VertexIndex / XE_N, k = gl_VertexIndex - XE_N * p, killed = 0;
  vec4 own = vec4(0.0), oi[16];
  for (int j = 0; j < XE_N; j++) {
    vec4 q = xe_vertex(r_ld(XE_INDICES, uint(XE_N * p + j)));
    if (xe_killed()) killed++;
    if (j == k) {
      own = q;
      oi = xe_i;
    }
  }
  gl_Position = xe_dropped(killed, XE_N) ? XE_DROPPED : own;
  xe_emit(oi);
  xe_pc = vec2(0.0);
}
)";

// A rectangle: six host vertices, the guest's three and the triangle
// across the longest edge (in XY/W) to the fourth, which completes the
// parallelogram, v3 = a + b - c with c the vertex opposite that edge, in
// the position and every interpolator (Xenia's rule), wound as the first
// (b, a, v3 after c, a, b). Its guest indices are in buffer XE_INDICES or
// counted.
const char kRectangleMain[] = R"(
void main() {
  int r = gl_VertexIndex / 6, k = gl_VertexIndex - 6 * r, killed = 0;
  vec4 p[3];
  vec4 v[3][16];
  for (int j = 0; j < 3; j++) {
    p[j] = xe_vertex(xe_sys.z != 0 ? r_ld(XE_INDICES, uint(3 * r + j)) : uint(3 * r + j));
    if (xe_killed()) killed++;
    v[j] = xe_i;
  }
  vec2 q0 = p[0].xy / p[0].w, q1 = p[1].xy / p[1].w, q2 = p[2].xy / p[2].w;
  float e0 = dot(q1 - q2, q1 - q2), e1 = dot(q2 - q0, q2 - q0), e2 = dot(q0 - q1, q0 - q1);
  int c = e0 > e1 && e0 > e2 ? 0 : e1 > e2 ? 1 : 2;
  int a = c == 2 ? 0 : c + 1, b = c == 0 ? 2 : c - 1;
  if (k < 3) {
    gl_Position = p[k];
    xe_emit(v[k]);
  } else if (k == 3) {
    gl_Position = p[b];
    xe_emit(v[b]);
  } else if (k == 4) {
    gl_Position = p[a];
    xe_emit(v[a]);
  } else {
    gl_Position = p[a] + p[b] - p[c];
    vec4 o[16];
    for (int i = 0; i < 16; i++) o[i] = v[a][i] + v[b][i] - v[c][i];
    xe_emit(o);
  }
  if (xe_dropped(killed, 3)) gl_Position = XE_DROPPED;
  xe_pc = vec2(0.0);
}
)";

// A point sprite: six host vertices, the corners top left, bottom left,
// top right and bottom right as two triangles (0 1 2, 1 3 2: Xenia's
// strip), the diameter the vertex shader's point size where it writes one
// (clamped to the bounds as signed integers, Xenia's clamp) else the
// constant, in pixels around the vertex; the sprite coordinates 0-1 left
// to right and top to bottom. Its guest index is in buffer XE_INDICES or
// counted.
const char kPointMain[] = R"(
void main() {
  int pt = gl_VertexIndex / 6, k = gl_VertexIndex - 6 * pt;
  vec4 p = xe_vertex(xe_sys.z != 0 ? r_ld(XE_INDICES, uint(pt)) : uint(pt));
  vec2 d = xe_point.xy;
#ifdef XE_POINT_SIZE
  d = vec2(intBitsToFloat(clamp(floatBitsToInt(xe_pts.x), floatBitsToInt(xe_point.z), floatBitsToInt(xe_point.w))));
#endif
  int c = k == 3 ? 1 : k == 4 ? 3 : k == 5 ? 2 : k;
  vec2 s = vec2((c & 2) != 0 ? 1.0 : 0.0, (c & 1) != 0 ? 1.0 : 0.0);
  gl_Position = vec4(p.xy + (s * 2.0 - 1.0) * d * xe_point_ndc.xy * p.w, p.zw);
  if (xe_killed()) gl_Position = XE_DROPPED;
  xe_emit(xe_i);
  xe_pc = s;
}
)";

// The 16 interpolators between the stages, one variable each (an array
// of them reads as zeros on Mesa's lavapipe), and a point sprite's
// coordinates: xe_emit writes them, xe_inputs reads them into r0-r15.
std::string Varyings(bool vertex) {
  std::string g, f = vertex ? "void xe_emit(vec4 v[16]) {\n" : "void xe_inputs() {\n";
  for (int k = 0; k < 16; k++) {
    std::string n = std::to_string(k);
    g += "layout(location = " + n + (vertex ? ") out" : ") in") + " vec4 xe_v" + n + ";\n";
    f += vertex ? "  xe_v" + n + " = v[" + n + "];\n" : "  xe_r[" + n + "] = xe_v" + n + ";\n";
  }
  g += std::string("layout(location = 16) ") + (vertex ? "out" : "in") + " vec2 xe_pc;\n";
  return g + f + "}\n";
}

// The pixel stage: the interpolators into r0-r15, the rest zero; the
// alpha test on colour 0 (its function's bits less, equal, greater; not
// equal true for NaN; Xenia's); each colour written scaled by its
// target's exponent bias and, to a gamma target, converted to the
// Xenos's piecewise-linear gamma (Xenia's LinearToPWLGamma, with its
// truncation), so blending happens on the gamma values as in Xenia.
const char kPixelCommon[] = R"(
float xe_gamma(float l) {
  l = isnan(l) ? 0.0 : clamp(l, 0.0, 1.0);
  if (l >= 512.0 / 1023.0) return trunc(l * (1023.0 / 8.0)) / 255.0 + 128.0 / 255.0;
  if (l >= 128.0 / 1023.0) return trunc(l * (1023.0 / 4.0)) / 255.0 + 64.0 / 255.0;
  if (l >= 64.0 / 1023.0) return trunc(l * (1023.0 / 2.0)) / 255.0 + 32.0 / 255.0;
  return trunc(l * 1023.0) / 255.0;
}
vec4 xe_out(int i) {
  vec4 c = xe_oc[i] * xe_color_scale[i];
  if (xe_color_gamma[i] != 0) c.rgb = vec3(xe_gamma(c.r), xe_gamma(c.g), xe_gamma(c.b));
  return c;
}
void xe_pixel() {
  xe_inputs();
  for (int k = 16; k < 64; k++) xe_r[k] = vec4(0.0);
  if (xe_prim.y >= 0) {
    // The pixel parameters (Xenia's): the pixel's position, |X| and |Y|,
    // X negative on a polygon's back face, Y negative for a point; a
    // point's sprite coordinates in ZW, else Z -0 for a line.
    // Built as bits: some hosts lose the sign of a zero chosen as a float.
    uvec2 xy = floatBitsToUint(abs(floor(floor(gl_FragCoord.xy) / vec2(uvec2(1u) << xe_index.zw))));
    uvec2 zw = xe_prim.z == 2 ? floatBitsToUint(clamp(xe_pc, 0.0, 1.0)) : uvec2(xe_prim.z == 1 ? 0x80000000u : 0u, 0u);
    if (xe_prim.z == 0 && !gl_FrontFacing) xy.x |= 0x80000000u;
    if (xe_prim.z == 2) xy.y |= 0x80000000u;
    xe_r[xe_prim.y] = uintBitsToFloat(uvec4(xy, zw));
  }
  for (int k = 0; k < 4; k++) xe_oc[k] = vec4(0.0);
  xe_od = vec4(gl_FragCoord.z);
  xe_shader();
}
bool xe_alpha_test() {
  int f = xe_sys.w;
  float a = xe_oc[0].a, r = xe_alpha_ref.x;
  if (f == 7) return true;
  if (f == 5) return a != r;
  return ((f & 1) != 0 && a < r) || ((f & 2) != 0 && a == r) || ((f & 4) != 0 && a > r);
}
)";

// The pixel stage's main for a pixel shader: its colours out, colour 0
// alpha tested, and its depth.
std::string PixelMain(const ShaderInfo& p) {
  std::string g, body = "void main() {\n  xe_pixel();\n";
  if (p.colors & 1) body += "  if (!xe_alpha_test()) discard;\n";
  for (uint32_t i = 0; i < 4; i++) {
    if (!(p.colors >> i & 1)) continue;
    g += "layout(location = " + std::to_string(i) + ") out vec4 xe_color" + std::to_string(i) + ";\n";
    body += "  xe_color" + std::to_string(i) + " = xe_out(" + std::to_string(i) + ");\n";
  }
  if (p.depth) body += "  gl_FragDepth = xe_od.x;\n";
  return g + body + "}\n";
}

// ---- render targets ------------------------------------------------------------
// The host image of an EDRAM colour or depth target, in samples (each
// sample a host pixel: MSAA is supersampled), as wide as its pitch in
// tiles and as high as the EDRAM holds rows of 32bpp tiles of that pitch
// (at most the host's 4096), so the targets one surface binds are all of
// one size. A sample (x, y) is the EDRAM word EdramWord gives for pixel
// (x, y) without sample shifts: the layout resolves read.
constexpr int kStencilLoaded = -1, kStencilUnloaded = -2;
struct Target {
  bool depth;
  uint32_t base, pitch, msaa, format;  // base tile, pitch in 32bpp tiles, RB_SURFACE_INFO's MSAA, format
  bool wide;                           // 64bpp: two words a sample, tiles of 40x16
  gfx::TextureId image = 0;            // a colour target's
  gfx::DepthId ds = 0;                 // a depth target's, and the image its stencil is read
  gfx::TextureId stencil_image = 0;    // into
  int w = 0, h = 0;
  bool valid = false;  // holds what the EDRAM holds (in its rows in use)
  bool dirty = false;  // drawn into since its last store
  uint32_t rows = 0;   // in use: what draws' scissors reach
  // A depth target's stencil in its rows in use: one value all of them
  // hold, where known (>= 0); loaded per sample (kStencilLoaded); or not
  // loaded (kStencilUnloaded): the host image's is not used, the EDRAM's
  // words hold it. They do while no draw has written it (stencil_dirty):
  // a valid target holds what the EDRAM holds in those rows but for what
  // draws changed, as any other writer of their tiles first stores the
  // target and makes it not valid; and a draw that writes the stencil
  // tests it, which loads it first.
  int stencil = kStencilUnloaded;
  bool stencil_dirty = false;  // a draw wrote the stencil since it last matched the EDRAM's
};
// Read by the test harness: loads, and the draws of one stencil bit each
// that loads and stores make.
std::atomic<uint64_t> g_loads, g_load_bits, g_store_bits;

// The host format of each colour target format: 8_8_8_8 as unorm (the
// gamma one holding the gamma values, as Xenia does); 2_10_10_10 as unorm;
// its 7e3 float variant and 16_16_16_16_FLOAT as half floats; the
// fixed-point 16_16(_16_16), [-32, 32], as snorm holding a 32nd of the
// value (Xenia's); the float ones as themselves.
gfx::Format HostFormat(uint32_t format) {
  switch (format) {
    case 0: case 1: return gfx::Format::R8G8B8A8;
    case 2: case 10: return gfx::Format::R10G10B10A2;
    case 3: case 12: case 7: return gfx::Format::A16B16G16R16F;
    case 4: return gfx::Format::R16G16Snorm;
    case 5: return gfx::Format::R16G16B16A16Snorm;
    case 6: return gfx::Format::G16R16F;
    case 14: return gfx::Format::R32F;
    case 15: return gfx::Format::G32R32F;
    default: Fatal(nullptr, "GPU: a draw into colour target format %u", format);
  }
}
bool Wide(uint32_t format) { return format == 5 || format == 7 || format == 15; }

// What a load gives a colour target for its EDRAM words (Unpack and
// HostScale), on the CPU.
gfx::Color LoadedColor(uint32_t format, uint32_t v0, uint32_t v1) {
  auto un = [](uint32_t v, uint32_t at, uint32_t bits) {
    return float(v >> at & ((1u << bits) - 1)) / float((1u << bits) - 1);
  };
  auto fixed = [](uint32_t v) { return std::max(-32.0f, float(int16_t(v)) * (32.0f / 32767.0f)) * (1.0f / 32.0f); };
  auto half = [](uint32_t v) { return Half(uint16_t(v)); };
  auto bits = [](uint32_t v) {
    float f;
    std::memcpy(&f, &v, 4);
    return f;
  };
  switch (format) {
    case 0: case 1: return {un(v0, 0, 8), un(v0, 8, 8), un(v0, 16, 8), un(v0, 24, 8)};
    case 2: case 10: return {un(v0, 0, 10), un(v0, 10, 10), un(v0, 20, 10), un(v0, 30, 2)};
    case 3: case 12: return {F7e3(v0), F7e3(v0 >> 10), F7e3(v0 >> 20), un(v0, 30, 2)};
    case 4: return {fixed(v0), fixed(v0 >> 16), 0, 0};
    case 5: return {fixed(v0), fixed(v0 >> 16), fixed(v1), fixed(v1 >> 16)};
    case 6: return {half(v0 & 0xFFFF), half(v0 >> 16), 0, 0};
    case 7: return {half(v0 & 0xFFFF), half(v0 >> 16), half(v1 & 0xFFFF), half(v1 >> 16)};
    case 14: return {bits(v0), 0, 0, 0};
    default: return {bits(v0), bits(v1), 0, 0};  // 15
  }
}
// What the host target holds of a value: a 32nd of the fixed-point
// formats'.
const char kHostScaleGlsl[] = R"(
float HostScale(uint fmt) { return fmt == 4u || fmt == 5u ? 1.0 / 32.0 : 1.0; }
)";

// Loads draw the EDRAM's words into a target: a colour target's unpacked
// (Unpack, as resolves read them); a depth target's depth (r.x < 0), or
// one bit of its stencil (the samples whose bit r.x is set pass).
const char kLoadUniforms[] = R"(
layout(R_UNIFORMS) uniform U {
  uvec4 t;  // base, pitch, format, 64bpp
  ivec4 r;  // the stencil bit
};
)";
const char kLoadColor[] = R"(
layout(location = 0) out vec4 xe_out;
void main() {
  bool wide = t.w != 0u;
  uint w = EdramWord(ivec2(gl_FragCoord.xy), 0u, t.x, t.y, uvec2(0u), wide, false);
  xe_out = Unpack(t.z, r_ld(0u, w), wide ? r_ld(0u, w + 1u) : 0u) * HostScale(t.z);
}
)";
const char kLoadDepth[] = R"(
void main() {
  uint v = r_ld(0u, EdramWord(ivec2(gl_FragCoord.xy), 0u, t.x, t.y, uvec2(0u), false, true));
  if (r.x >= 0 && (v >> uint(r.x) & 1u) == 0u) discard;
  gl_FragDepth = t.z == 0u ? Unorm24(v >> 8) : min(F20e4(v >> 8), 1.0);
}
)";
// A depth target's stencil read into an R8 image a bit at a time: the
// samples whose stencil has bit r.x add 2^r.x (the device samples depth,
// not stencil).
const char kStencilBit[] = R"(
layout(R_UNIFORMS) uniform U { ivec4 r; };
layout(location = 0) out vec4 xe_out;
void main() { xe_out = vec4(float(1 << r.x) / 255.0); }
)";
// Stores pack a target's rows into the EDRAM: a colour target's as
// PackTarget packs them, a depth target's as 24-bit depth (D24S8 unorm
// rounded to the nearest even, D24FS8 20e4) over 8 bits of stencil.
const char kStoreUniforms[] = R"(
layout(R_UNIFORMS) uniform U {
  uvec4 t;  // base, pitch, format, 64bpp
  ivec4 r;  // width, first row, end row, the stencil: where known; -1 its image's; -2 the EDRAM's kept
};
)";
const char kStoreColor[] = R"(
layout(R_TEXTURE(0)) uniform sampler2D xe_target;
void main() {
  ivec2 p = ivec2(gl_GlobalInvocationID.xy) + ivec2(0, r.y);
  if (p.x >= r.x || p.y >= r.z) return;
  bool wide = t.w != 0u;
  uvec2 v = PackTarget(t.z, texelFetch(xe_target, p, 0) / HostScale(t.z));
  uint w = r_rwbuf[0].x + EdramWord(p, 0u, t.x, t.y, uvec2(0u), wide, false);
  r_rw[w] = v.x;
  if (wide) r_rw[w + 1u] = v.y;
}
)";
const char kStoreDepth[] = R"(
layout(R_TEXTURE(0)) uniform sampler2DArray xe_depth;
layout(R_TEXTURE(1)) uniform sampler2D xe_stencil;
void main() {
  ivec2 p = ivec2(gl_GlobalInvocationID.xy) + ivec2(0, r.y);
  if (p.x >= r.x || p.y >= r.z) return;
  float d = texelFetch(xe_depth, ivec3(p, 0), 0).r;
  uint w = r_rwbuf[0].x + EdramWord(p, 0u, t.x, t.y, uvec2(0u), false, true);
  uint s = r.w >= 0    ? uint(r.w)
           : r.w == -1 ? uint(texelFetch(xe_stencil, p, 0).r * 255.0 + 0.5)
                       : r_rw[w] & 0xFFu;
  uint d24 = t.z == 0u ? F32ToUnorm24(d) : F32To20e4(d);
  r_rw[w] = d24 << 8 | s;
}
)";

// The renderer's state on the display's device (its lock held).
struct State {
  // By vertex shader, pixel shader (none: depth only), kind of draw and
  // which textures of dimension 2 are stacked (a bit each, the pixel
  // shader's from bit 32).
  std::map<std::tuple<const Shader*, const Shader*, Kind, uint64_t>, gfx::ProgramId> programs;
  gfx::ProgramId load_color = 0, load_depth = 0, stencil_bit = 0, store_color = 0, store_depth = 0;
  std::vector<std::unique_ptr<Target>> targets;
  // Each EDRAM tile's stencil: the low byte all its words hold, as a depth
  // target reads them, where known (else -1). The buffer starts zeroed;
  // stores and resolves' clears, its only writers, keep this.
  int16_t tile_stencil[kEdramTiles] = {};
};
State g_state;

// The EDRAM tiles of a target's rows in use.
uint32_t Tiles(const Target& t, uint32_t rows) { return (rows + 15) / 16 * t.pitch * (t.wide ? 2 : 1); }
// Whether two targets' tiles, rows in use, share one (the EDRAM wraps).
bool Overlap(const Target& a, uint32_t a_rows, const Target& b, uint32_t b_rows) {
  uint32_t an = Tiles(a, a_rows), bn = Tiles(b, b_rows);
  if (!an || !bn) return false;
  return ((b.base - a.base) & (kEdramTiles - 1)) < an || ((a.base - b.base) & (kEdramTiles - 1)) < bn;
}

// A write of stencil v (-1: unknown, or not depth words) to samples
// [x0, x1) x [y0, y1) of a target at base tile `base`, its rows of tiles
// `row` tiles of `tile_w` samples: the tiles all of whose words it writes
// hold v; others it touches keep v only where they held it.
void TilesWritten(uint32_t base, uint32_t row, uint32_t tile_w, uint32_t x0, uint32_t y0, uint32_t x1,
                  uint32_t y1, int v) {
  if (x1 <= x0 || y1 <= y0) return;
  for (uint32_t ty = y0 / 16; ty * 16 < y1; ty++)
    for (uint32_t tx = x0 / tile_w; tx * tile_w < x1; tx++) {
      int16_t& s = g_state.tile_stencil[(base + ty * row + tx) & (kEdramTiles - 1)];
      bool all = x0 <= tx * tile_w && (tx + 1) * tile_w <= x1 && y0 <= ty * 16 && (ty + 1) * 16 <= y1;
      s = int16_t(all || s == v ? v : -1);
    }
}
// The stencil all the tiles of a depth target's rows [first, end) hold,
// where it is one known value; else -1.
int TilesStencil(const Target& t, uint32_t first, uint32_t end) {
  int v = g_state.tile_stencil[(t.base + first / 16 * t.pitch) & (kEdramTiles - 1)];
  for (uint32_t ty = first / 16; ty * 16 < end; ty++)
    for (uint32_t tx = 0; tx < t.pitch; tx++)
      if (g_state.tile_stencil[(t.base + ty * t.pitch + tx) & (kEdramTiles - 1)] != v) return -1;
  return v;
}

gfx::ProgramId Make(gfx::ProgramId& id, const std::string& vs, const std::string& fs, uint32_t fs_buffers,
                    uint32_t fs_uniforms) {
  if (id) return id;
  gfx::ProgramDesc d;
  d.vs.glsl = vs;
  d.fs.glsl = fs;
  d.fs.buffers = fs_buffers;
  d.fs.uniform_bytes = fs_uniforms;
  id = HostDevice().CreateProgram(d);
  return id;
}
gfx::ProgramId MakeCompute(gfx::ProgramId& id, const std::string& cs, uint32_t textures, bool array) {
  if (id) return id;
  gfx::ProgramDesc d;
  d.cs.glsl = cs;
  d.cs.textures = textures;
  d.cs.texture_kinds[0] = array ? gfx::TextureKind::Tex2DArray : gfx::TextureKind::Tex2D;
  d.cs.rw_buffers = 1;
  d.cs.uniform_bytes = 32;
  d.local_size[0] = d.local_size[1] = 8;
  id = HostDevice().CreateProgram(d);
  return id;
}

// A full-target draw over rows [first, end) of a target.
gfx::ProgramDraw FullDraw(gfx::ProgramId prog, const Target& t, uint32_t first, uint32_t end) {
  gfx::ProgramDraw d;
  d.program = prog;
  d.viewport = {0, 0, uint32_t(t.w), uint32_t(t.h), 0.0f, 1.0f};
  d.scissor = {0, int32_t(first), t.w, int32_t(end)};
  d.vertex_count = 3;
  return d;
}

// A depth target's rows [first, end) drawn from the EDRAM: the stencil
// all v (v >= 0), else cleared, then each bit set; the depth too with the
// first draw where `depth`, else left as it is (no depth test or write).
void LoadStencil(Target& t, uint32_t first, uint32_t end, int v, bool depth) {
  gfx::Device& dev = HostDevice();
  struct {
    uint32_t t[4];
    int32_t r[4];
  } u = {{t.base, t.pitch, t.format, 0}, {-1, 0, 0, 0}};
  gfx::BufferData edram{nullptr, kEdramBytes, EdramBuffer(), 0};
  std::string fs = std::string(kLoadUniforms) + kEdramGlsl + kHostScaleGlsl + kLoadDepth;
  gfx::ProgramDraw d = FullDraw(Make(g_state.load_depth, kFullTargetVertex, fs, 1, 32), t, first, end);
  d.depth = t.ds;
  d.fs.buffers = &edram;
  d.fs.uniforms = &u;
  d.state.depth_test = d.state.depth_write = depth;
  d.state.stencil = true;
  d.state.stencil_front.pass = d.state.stencil_back.pass = gfx::StencilOp::Replace;
  d.state.stencil_ref = uint8_t(std::max(v, 0));
  dev.DrawProgram(d);
  if (v >= 0) return;
  d.state.depth_test = d.state.depth_write = false;
  d.state.stencil_ref = 0xFF;
  for (int bit = 0; bit < 8; bit++) {
    u.r[0] = bit;
    d.state.stencil_write_mask = uint8_t(1 << bit);
    dev.DrawProgram(d);
    g_load_bits++;
  }
}

void Load(Target& t, uint32_t first, uint32_t end) {
  if (end <= first) return;
  g_loads++;
  gfx::Device& dev = HostDevice();
  struct {
    uint32_t t[4];
    int32_t r[4];
  } u = {{t.base, t.pitch, t.format, t.wide ? 1u : 0u}, {-1, 0, 0, 0}};
  gfx::BufferData edram{nullptr, kEdramBytes, EdramBuffer(), 0};
  std::string fs = std::string(kLoadUniforms) + kEdramGlsl + kHostScaleGlsl;
  if (!t.depth) {
    gfx::ProgramDraw d =
        FullDraw(Make(g_state.load_color, kFullTargetVertex, fs + kLoadColor, 1, 32), t, first, end);
    d.targets[0] = {t.image, 0};
    d.fs.buffers = &edram;
    d.fs.uniforms = &u;
    dev.DrawProgram(d);
    return;
  }
  // The depth; the stencil the EDRAM's tiles there all hold where it is
  // known, else not loaded until a draw tests it. Rows added to a target
  // whose stencil is loaded load theirs at once.
  int v = TilesStencil(t, first, end);
  bool bits = false;
  if (!first) {
    t.stencil = v >= 0 ? v : kStencilUnloaded;
    t.stencil_dirty = false;
  } else if (t.stencil != kStencilUnloaded && (v < 0 || t.stencil != v)) {
    bits = v < 0;
    t.stencil = kStencilLoaded;
  }
  LoadStencil(t, first, end, bits ? -1 : std::max(v, 0), true);
}

void Store(Target& t) {
  if (!t.dirty) return;
  gfx::Device& dev = HostDevice();
  struct {
    uint32_t t[4];
    int32_t r[4];
  } u = {{t.base, t.pitch, t.format, t.wide ? 1u : 0u}, {t.w, 0, int32_t(t.rows), t.stencil}};
  // A depth target's stencil no draw wrote is the EDRAM's already: its
  // words keep their low bytes, and the tiles their record.
  bool keep = t.depth && !t.stencil_dirty;
  if (keep) u.r[3] = -2;
  std::string cs = std::string(kStoreUniforms) + kEdramGlsl + kHostScaleGlsl;
  gfx::ProgramDispatch d;
  gfx::TextureId textures[2] = {t.image, 0};
  if (t.depth && !keep && t.stencil < 0) {
    // The stencil into its image first, unless it is known.
    dev.ClearImage(t.stencil_image, 0, {0, 0, 0, 0});
    gfx::ProgramDraw s = FullDraw(Make(g_state.stencil_bit, kFullTargetVertex, kStencilBit, 0, 16), t, 0, t.rows);
    s.targets[0] = {t.stencil_image, 0};
    s.depth = t.ds;
    s.state.stencil = true;
    s.state.stencil_front.compare = s.state.stencil_back.compare = gfx::CompareOp::Equal;
    s.state.stencil_write_mask = 0;
    s.state.stencil_ref = 0xFF;
    s.state.blend[0] = {true, gfx::BlendFactor::One, gfx::BlendFactor::One, gfx::BlendOp::Add,
                        gfx::BlendFactor::One, gfx::BlendFactor::One, gfx::BlendOp::Add, 1};
    int32_t bit[4] = {0, 0, 0, 0};
    s.fs.uniforms = bit;
    for (; bit[0] < 8; bit[0]++) {
      s.state.stencil_read_mask = uint8_t(1 << bit[0]);
      dev.DrawProgram(s);
      g_store_bits++;
    }
  }
  if (t.depth) {
    textures[0] = t.ds;
    textures[1] = t.stencil_image;
    d.program = MakeCompute(g_state.store_depth, cs + kStoreDepth, 2, true);
  } else {
    d.program = MakeCompute(g_state.store_color, cs + kStoreColor, 1, false);
  }
  gfx::RwBufferData out{nullptr, kEdramBytes, EdramBuffer(), false};
  d.groups[0] = uint32_t(t.w + 7) / 8;
  d.groups[1] = (t.rows + 7) / 8;
  d.cs.uniforms = &u;
  d.cs.textures = textures;
  d.rw_buffers = &out;
  dev.DispatchProgram(d);
  if (!keep)
    TilesWritten(t.base, t.pitch * (t.wide ? 2 : 1), t.wide ? 40 : 80, 0, 0, uint32_t(t.w), t.rows,
                 t.depth ? t.stencil : -1);
  t.dirty = t.stencil_dirty = false;
}

// The target of a surface's: found, or made.
Target& GetTarget(bool depth, uint32_t base, uint32_t pitch, uint32_t msaa, uint32_t format) {
  for (auto& c : g_state.targets)
    if (c->depth == depth && c->base == base && c->pitch == pitch && c->msaa == msaa && c->format == format)
      return *c;
  auto n = std::make_unique<Target>();
  n->depth = depth;
  n->base = base;
  n->pitch = pitch;
  n->msaa = msaa;
  n->format = format;
  n->wide = !depth && Wide(format);
  n->w = int(pitch * 80);
  n->h = int(std::min(kEdramTiles / pitch * 16, 4096u));
  gfx::Device& dev = HostDevice();
  if (depth) {
    n->ds = dev.CreateDepthArray(n->w, n->h, 1, gfx::Format::D32FS8);
    gfx::ImageDesc s;
    s.w = n->w;
    s.h = n->h;
    s.fmt = gfx::Format::R8;
    s.color_target = true;
    n->stencil_image = dev.CreateImage(s);
    if (!n->ds || !n->stencil_image) Fatal(nullptr, "GPU: no host image for a %dx%d depth target", n->w, n->h);
  } else {
    gfx::ImageDesc d;
    d.w = n->w;
    d.h = n->h;
    d.fmt = HostFormat(format);
    d.color_target = true;
    n->image = dev.CreateImage(d);
    if (!n->image)
      Fatal(nullptr, "GPU: no host image for a %dx%d colour target of format %u", n->w, n->h, format);
  }
  g_state.targets.push_back(std::move(n));
  return *g_state.targets.back();
}

// A draw's targets, holding the EDRAM's contents of their rows up to
// `rows`, each up to the next bound target's base tile (wrapping) too: a
// title lays a draw's targets out one after the other, whatever its
// scissor. Other targets they overlap are stored first if drawn into, and
// become stale; a stale target, or rows it had not used, loads.
void BindTargets(const std::vector<Target*>& bound, uint32_t rows) {
  std::vector<uint32_t> limit(bound.size());
  for (size_t i = 0; i < bound.size(); i++) {
    const Target& t = *bound[i];
    uint32_t tiles = kEdramTiles;
    for (const Target* c : bound) {
      if (c == &t) continue;
      if (c->base == t.base) Fatal(nullptr, "GPU: a draw's targets share base tile %u", t.base);
      tiles = std::min(tiles, (c->base - t.base) & (kEdramTiles - 1));
    }
    limit[i] = std::min(tiles / (t.pitch * (t.wide ? 2 : 1)) * 16, uint32_t(t.h));
  }
  for (size_t i = 0; i < bound.size(); i++) {
    uint32_t r = std::max(std::min(rows, limit[i]), std::min(bound[i]->rows, limit[i]));
    for (auto& c : g_state.targets) {
      if (std::find(bound.begin(), bound.end(), c.get()) != bound.end()) continue;
      if (!Overlap(*bound[i], r, *c, c->rows)) continue;
      Store(*c);
      c->valid = false;
    }
  }
  for (size_t i = 0; i < bound.size(); i++) {
    // Rows past the limit (from an earlier draw alone) are stored and
    // dropped, so the targets' rows do not overlap.
    Target& t = *bound[i];
    if (t.rows > limit[i]) {
      Store(t);
      t.rows = limit[i];
    }
    uint32_t r = std::max(std::min(rows, limit[i]), t.rows);
    Load(t, t.valid ? t.rows : 0, r);
    t.valid = true;
    t.rows = r;
  }
}

// A guest index that is the primitive reset index.
constexpr uint32_t kReset = 0xFFFFFFFF;

// The guest's indices of a draw, from memory or (`immediate`) the packet,
// as the GPU takes them: endian swapped (memory's; the packet's words as
// they are, 16-bit ones the low half first, the R6xx's packing), the low
// 24 bits; where the primitive reset index restarts strips, kReset.
std::vector<uint32_t> RawIndices(uint32_t initiator, uint32_t count, uint32_t base, uint32_t size,
                                 const uint32_t* immediate, bool reset, uint32_t reset_index) {
  bool wide = initiator >> 11 & 1;
  uint32_t endian = size >> 30, bytes = wide ? 4 : 2, pa = base & ~(bytes - 1);
  if (!immediate) {
    if ((size & 0xFFFFFF) < count)
      Fatal(nullptr, "GPU: a draw of %u indices from a buffer of %u", count, size & 0xFFFFFF);
    if (!wide && endian > 1) Fatal(nullptr, "GPU: 16-bit indices with endian swap %u", endian);
    if (pa >= kPhysicalSize || uint64_t(count) * bytes > kPhysicalSize - pa)
      Fatal(nullptr, "GPU: %u indices at %x, outside physical memory", count, base);
    LandResolves(pa, pa + count * bytes);
  }
  std::vector<uint32_t> out(count);
  for (uint32_t i = 0; i < count; i++) {
    uint32_t v;
    if (immediate) {
      v = wide ? immediate[i] : immediate[i / 2] >> (16 * (i & 1)) & 0xFFFF;
    } else if (wide) {
      v = GpuSwap(__builtin_bswap32(Rd32(PhysicalToVirtual(pa + 4 * i))), endian);
    } else {
      // The halfword as this big-endian CPU reads it is the one swapped
      // bytes in halfwords (1) give; as stored (0) it is byte-reversed.
      uint16_t h = Rd16(PhysicalToVirtual(pa + 2 * i));
      v = endian ? h : uint16_t(h >> 8 | h << 8);
    }
    // The reset index is compared with all of a 16-bit index, the low 24
    // bits of a 32-bit one (Xenia's).
    out[i] = reset && (v & 0xFFFFFF) == reset_index ? kReset : v & 0xFFFFFF;
  }
  return out;
}

// A primitive type's guest indices as a host list of triangles or lines,
// in the orders Xenia's primitive processor makes: a strip's odd
// triangles as v1 v3 v2 (the host's own strip order, so its winding and
// first vertex), a fan's as (v[i-1], v[i], v0), a quad's as (0 1 2),
// (0 2 3), a line strip's and loop's segments as pairs (a loop's last
// back to its first); strips and fans restart after a reset index.
std::vector<uint32_t> Decompose(uint32_t prim, const std::vector<uint32_t>& in) {
  std::vector<uint32_t> out;
  size_t n = in.size();
  switch (prim) {
    case kPrimLineList:
      for (size_t i = 0; i + 1 < n; i += 2) out.insert(out.end(), {in[i], in[i + 1]});
      break;
    case kPrimTriangleList:
      for (size_t i = 0; i + 2 < n; i += 3) out.insert(out.end(), {in[i], in[i + 1], in[i + 2]});
      break;
    case kPrimQuadList:
      for (size_t i = 0; i + 3 < n; i += 4)
        out.insert(out.end(), {in[i], in[i + 1], in[i + 2], in[i], in[i + 2], in[i + 3]});
      break;
    case kPrimLineLoop:
      for (size_t i = 0; i + 1 < n; i++) out.insert(out.end(), {in[i], in[i + 1]});
      if (n > 1) out.insert(out.end(), {in[n - 1], in[0]});
      break;
    default:
      // The strips and the fan, a run between resets at a time.
      for (size_t first = 0; first < n;) {
        size_t end = first;
        while (end < n && in[end] != kReset) end++;
        const uint32_t* v = in.data() + first;
        size_t m = end - first;
        for (size_t i = 0; prim == kPrimLineStrip && i + 1 < m; i++) out.insert(out.end(), {v[i], v[i + 1]});
        for (size_t i = 0; prim == kPrimTriangleStrip && i + 2 < m; i++) {
          if (i & 1) out.insert(out.end(), {v[i], v[i + 2], v[i + 1]});
          else out.insert(out.end(), {v[i], v[i + 1], v[i + 2]});
        }
        for (size_t i = 2; prim == kPrimTriangleFan && i < m; i++) out.insert(out.end(), {v[i - 1], v[i], v[0]});
        first = end + 1;
      }
  }
  return out;
}

// The scissor: the window scissor (moved by the window offset unless its
// TL bit 31 says not to) within the screen scissor, as resolves take it.
gfx::Rect Scissor() {
  uint32_t off = GpuRegister(kRegWindowOffset);
  int32_t ox = Signed(off, 15), oy = Signed(off >> 16, 15);
  uint32_t tl = GpuRegister(kRegWindowScissorTl), br = GpuRegister(kRegWindowScissorBr);
  int32_t x0 = tl & 0x3FFF, y0 = tl >> 16 & 0x3FFF, x1 = br & 0x3FFF, y1 = br >> 16 & 0x3FFF;
  if (!(tl >> 31)) {
    x0 += ox;
    x1 += ox;
    y0 += oy;
    y1 += oy;
  }
  uint32_t stl = GpuRegister(kRegScreenScissorTl), sbr = GpuRegister(kRegScreenScissorBr);
  x0 = std::max({x0, Signed(stl, 15), 0});
  y0 = std::max({y0, Signed(stl >> 16, 15), 0});
  x1 = std::max(std::min(x1, Signed(sbr, 15)), x0);
  y1 = std::max(std::min(y1, Signed(sbr >> 16, 15)), y0);
  return {x0, y0, x1, y1};
}

// The host image kinds of a stage's textures (by the instruction's
// dimension; dimension 2 by the constant's: stacked or 3D).
void Kinds(const ShaderInfo& s, uint32_t stacked, gfx::TextureKind* kinds) {
  for (uint32_t n = 0; n < s.textures.size() && n < 16; n++) {
    uint32_t d = s.textures[n].dimension;
    kinds[n] = d == 3 ? gfx::TextureKind::Cube
               : d == 2 ? (stacked >> n & 1 ? gfx::TextureKind::Tex2DArray : gfx::TextureKind::Tex3D)
                        : gfx::TextureKind::Tex2D;
  }
}

// The declarations and functions of a stage's textures of dimension 2
// (shader.h): a 3D texture's as the host's; a stacked one's as an array
// whose layer is Z in layers, floored (the Xenos addresses layers as 3D
// textures' slices, Xenia's), and, where xe_tp's flag says the filter
// between layers is linear, half a layer back and the next layer mixed in
// (Xenia's).
std::string TextureGlsl(const ShaderInfo& s, uint32_t stacked) {
  std::string g;
  for (uint32_t n = 0; n < s.textures.size(); n++) {
    if (s.textures[n].dimension != 2) continue;
    std::string t = "xe_t" + std::to_string(n), body;
    bool stack = stacked >> n & 1;
    g += "layout(R_TEXTURE(" + std::to_string(n) + ")) uniform " + (stack ? "sampler2DArray " : "sampler3D ") + t +
         ";\n";
    g += "vec3 " + t + "_size() { return vec3(textureSize(" + t + ", 0)); }\n";
    // Each function's sampling of a coordinate c; for a stacked texture at
    // layer L.
    auto fn = [&](const std::string& head, const std::string& call) {
      if (!stack) {
        std::string c = call;
        c.replace(c.find("CL"), 2, "c");
        return head + " { return " + c + "; }\n";
      }
      std::string at = call, next = call, floor_ = call;
      at.replace(at.find("CL"), 2, "vec3(c.xy, f)");
      next.replace(next.find("CL"), 2, "vec3(c.xy, f + 1.0)");
      floor_.replace(floor_.find("CL"), 2, "vec3(c.xy, floor(z))");
      return head + " {\n  float z = c.z * float(textureSize(" + t + ", 0).z);\n  if ((xe_tp[" + std::to_string(n) +
             "].w & 1) != 0) {\n    z -= 0.5;\n    float f = floor(z);\n    return mix(" + at + ", " + next +
             ", z - f);\n  }\n  return " + floor_ + ";\n}\n";
    };
    g += fn("vec4 " + t + "_lod(vec3 c, float l)", "textureLod(" + t + ", CL, l)");
    g += fn("vec4 " + t + "_grad(vec3 c, vec3 h, vec3 v)",
            stack ? "textureGrad(" + t + ", CL, h.xy, v.xy)" : "textureGrad(" + t + ", CL, h, v)");
    if (s.pixel) {
      g += fn("vec4 " + t + "_bias(vec3 c, float b)", "texture(" + t + ", CL, b)");
      g += "float " + t + "_qlod(vec3 c) { return textureQueryLod(" + t + (stack ? ", c.xy" : ", c") + ").y; }\n";
    }
  }
  return g;
}

// ---- draws -----------------------------------------------------------------------
// The program of a vertex shader and a pixel shader, or none (depth only:
// the pixel shader does not run, as Xenia's draws in that mode), for a
// kind of host draw.
gfx::ProgramId Program(Shader& vs, Shader* ps, Kind kind, uint64_t stacked) {
  gfx::ProgramId& id = g_state.programs[{&vs, ps, kind, stacked}];
  if (id) return id;
  const ShaderInfo& v = Info(vs);
  gfx::ProgramDesc d;
  // The vertex stage's buffers: the vertex buffers, the guest indices
  // (but for lists, which the host draws indexed), the constants.
  uint32_t constants = uint32_t(v.vertex_buffers.size()) + (kind != kKindList ? 1 : 0);
  d.vs.glsl = UniformGlsl(v) + kVertexGlobals + kShaderCommon + TextureGlsl(v, uint32_t(stacked)) +
              ConstantGlsl(v, constants) + v.glsl + kVertexRun + Varyings(true);
  d.vs.textures = uint32_t(v.textures.size());
  Kinds(v, uint32_t(stacked), d.vs.texture_kinds);
  d.vs.buffers = uint32_t(v.vertex_buffers.size());
  std::string main = kind == kKindList          ? kListMain
                     : kind == kKindRectangles ? kRectangleMain
                     : kind == kKindPoints     ? kPointMain
                                               : kKillMain;
  if (kind == kKindPoints && v.point_size) main = "#define XE_POINT_SIZE\n" + main;
  for (size_t at; (at = main.find("XE_INDICES")) != std::string::npos;)
    main.replace(at, 10, std::to_string(d.vs.buffers) + "u");
  for (size_t at; (at = main.find("XE_N")) != std::string::npos;)
    main.replace(at, 4, kind == kKindKillLines ? "2" : "3");
  d.vs.buffers = constants + (v.constants ? 1 : 0);
  d.vs.glsl += main;
  d.vs.uniform_bytes = UniformBytes(v);
  if (ps) {
    const ShaderInfo& p = Info(*ps);
    d.fs.glsl = UniformGlsl(p) + kPixelGlobals + kShaderCommon + TextureGlsl(p, uint32_t(stacked >> 32)) +
                ConstantGlsl(p, 0) + p.glsl + Varyings(false) + kPixelCommon + PixelMain(p);
    d.fs.textures = uint32_t(p.textures.size());
    d.fs.buffers = p.constants ? 1 : 0;
    Kinds(p, uint32_t(stacked >> 32), d.fs.texture_kinds);
    d.fs.uniform_bytes = UniformBytes(p);
  } else {
    d.fs.glsl = "void main() {}\n";
  }
  id = HostDevice().CreateProgram(d);
  return id;
}

// A stage's float constants (ConstantGlsl) from the register file, a
// pixel shader's from 256.
std::vector<uint32_t> Constants(const ShaderInfo& s) {
  std::vector<uint32_t> c;
  uint32_t first = kRegAluConstants + (s.pixel ? 4 * 256 : 0);
  for (uint32_t k = 0; k < 4 * s.constants; k++) c.push_back(GpuRegister(first + k));
  return c;
}

// A stage's uniform block (UniformGlsl): bools, loops, then the vertex
// buffers' and system values given.
std::vector<uint32_t> Uniforms(const ShaderInfo& s, const std::vector<uint32_t>& vf,
                               const std::vector<TextureBinding>& textures, const uint32_t sys[kSysWords]) {
  std::vector<uint32_t> u;
  for (uint32_t k = 0; k < 8; k++) u.push_back(GpuRegister(kRegBoolConstants + k));
  for (uint32_t k = 0; k < 32; k++) u.push_back(GpuRegister(kRegLoopConstants + k));
  u.insert(u.end(), vf.begin(), vf.end());
  u.resize(u.size() + 4 * (std::max<size_t>(1, s.vertex_buffers.size()) - vf.size() / 4));
  for (const TextureBinding& t : textures) u.insert(u.end(), t.tp, t.tp + 4);
  u.resize(u.size() + 4 * (std::max<size_t>(1, s.textures.size()) - textures.size()));
  u.insert(u.end(), sys, sys + kSysWords);
  return u;
}

// RB_BLENDCONTROL's factors, by their encoding (2 and 3 and above 16 are
// not factors); constant alpha is the constant colour the caller makes
// all alpha.
gfx::BlendFactor Factor(uint32_t f, bool* constant_alpha) {
  using F = gfx::BlendFactor;
  static const F kFactors[17] = {F::Zero, F::One, F::Zero, F::Zero, F::SrcColor, F::OneMinusSrcColor,
                                 F::SrcAlpha, F::OneMinusSrcAlpha, F::DstColor, F::OneMinusDstColor,
                                 F::DstAlpha, F::OneMinusDstAlpha, F::ConstantColor,
                                 F::OneMinusConstantColor, F::ConstantColor, F::OneMinusConstantColor,
                                 F::SrcAlphaSaturate};
  if (f == 2 || f == 3 || f > 16) Fatal(nullptr, "GPU: blend factor %u", f);
  if (f == 14 || f == 15) *constant_alpha = true;
  return kFactors[f];
}
gfx::BlendOp Op(uint32_t op) {
  using O = gfx::BlendOp;
  static const O kOps[5] = {O::Add, O::Subtract, O::Min, O::Max, O::ReverseSubtract};
  if (op > 4) Fatal(nullptr, "GPU: blend operation %u", op);
  return kOps[op];
}

}  // namespace

void LoadShader(bool pixel, const uint32_t* words, uint32_t count) {
  std::string key(1, pixel ? 'p' : 'v');
  key.append(reinterpret_cast<const char*>(words), 4 * size_t(count));
  std::unique_ptr<Shader>& s = g_shaders[key];
  if (!s) s.reset(new Shader{pixel, std::vector<uint32_t>(words, words + count)});
  (pixel ? g_pixel_shader : g_vertex_shader) = s.get();
}

void Draw(uint32_t initiator, uint32_t index_base, uint32_t index_size, const uint32_t* immediate) {
  uint32_t prim = initiator & 0x3F, source = initiator >> 6 & 3, count = initiator >> 16;
  uint32_t mode = GpuRegister(kRegRbModeControl) & 7, program_cntl = GpuRegister(kRegSqProgramCntl);
  // EDRAM mode 0 draws nothing (Xenia skips such draws), nor does a
  // multipass vertex shader (SQ_PROGRAM_CNTL's vs_export_mode 7: Xenia
  // rasterizes nothing then).
  // A surface of pitch 0 rasterizes nothing either (Xenia skips it).
  uint32_t export_mode = program_cntl >> 24 & 7;
  if (mode == kEdramModeIgnore || !count || export_mode == 7 || !(GpuRegister(kRegRbSurfaceInfo) & 0x3FFF)) return;
  if (mode != kEdramModeColorDepth && mode != kEdramModeDepth) Fatal(nullptr, "GPU: a draw in EDRAM mode %u", mode);
  if (prim != kPrimPointList && prim != kPrimLineList && prim != kPrimLineStrip && prim != kPrimTriangleList &&
      prim != kPrimTriangleFan && prim != kPrimTriangleStrip && prim != kPrimRectangleList &&
      prim != kPrimLineLoop && prim != kPrimQuadList)
    Fatal(nullptr, "GPU: a draw of primitive type %u (not implemented yet)", prim);
  if (source != kSourceDma && source != kSourceImmediate && source != kSourceAuto)
    Fatal(nullptr, "GPU: a draw with index source %u", source);
  // The vertex shader's exports: position and interpolators, with the
  // point size or vertex kill (modes 0, 2, 4, 5); edge flags (3, 6) are
  // not implemented (nor in Xenia), mode 1 is none. Indices made by the
  // sequencer (gen_index_vtx, bit 31) are not implemented either. The
  // register counts (vs_num_reg, ps_num_reg) and ps_export_mode are not
  // used: shaders here have all 64 registers, and the pixel shader's own
  // exports say whether it writes depth (Xenia uses none of them; console
  // check).
  if (export_mode == 1 || export_mode == 3 || export_mode == 6)
    Fatal(nullptr, "GPU: a vertex shader export mode %u (not implemented yet)", export_mode);
  if (program_cntl >> 31) Fatal(nullptr, "GPU: a draw with generated vertex indices (not implemented yet)");
  if (!g_vertex_shader || (mode == kEdramModeColorDepth && !g_pixel_shader))
    Fatal(nullptr, "GPU: a draw before its shaders are loaded");
  Shader* ps = mode == kEdramModeColorDepth ? g_pixel_shader : nullptr;
  const ShaderInfo& vi = Info(*g_vertex_shader);
  static const ShaderInfo kNoPixelShader = [] {
    ShaderInfo s;
    s.pixel = true;
    return s;
  }();
  const ShaderInfo& pi = ps ? Info(*ps) : kNoPixelShader;
  // Render state beyond this: polygon modes, alpha to mask.
  uint32_t mode_cntl = GpuRegister(kRegPaSuScModeCntl), color_control = GpuRegister(kRegRbColorControl);
  if (mode_cntl >> 3 & 3) Fatal(nullptr, "GPU: a draw with polygon mode %u (not implemented yet)", mode_cntl >> 3 & 3);
  if (color_control >> 4 & 1) Fatal(nullptr, "GPU: a draw with alpha to mask (not implemented yet)");
  // Interpolation as Xenia does it: SQ_INTERPOLATOR_CNTL's flat shading
  // (param_shade) and sampling pattern, PA_SU_SC_MODE_CNTL's
  // provoking_vtx_last and persp_corr_dis are not modelled (all
  // interpolators perspective-correct, at sample centres here; console
  // check).
  // Culling both faces draws no polygons.
  bool lines = prim == kPrimLineList || prim == kPrimLineStrip || prim == kPrimLineLoop,
       polygons = !lines && prim != kPrimPointList;
  if (polygons && (mode_cntl & 3) == 3) return;

  // The targets: the colours the pixel shader writes (from 0 on), and
  // depth where its test or the stencil test is on.
  uint32_t surface = GpuRegister(kRegRbSurfaceInfo), msaa = surface >> 16 & 3;
  if (msaa > 2) Fatal(nullptr, "GPU: a draw into a surface with MSAA mode 3");
  uint32_t shift_x = msaa == 2, shift_y = msaa >= 1;
  uint32_t pitch = (((surface & 0x3FFF) << shift_x) + 79) / 80;
  uint32_t colors = 0;
  while (pi.colors >> colors & 1) colors++;
  if (pi.colors >> colors)
    Fatal(nullptr, "GPU: a pixel shader writes colour targets %x, not from 0 on (not implemented yet)", pi.colors);
  uint32_t depth_control = GpuRegister(kRegRbDepthControl);
  // A stencil test that can neither fail nor write is as if off: for each
  // face in use (the back's own where bit 7 says), compare Always, and a
  // write mask of 0 or Keep where it passes and where the depth fails.
  // Without it or the Z test the draw does nothing to depth or stencil,
  // so the depth target is not bound.
  bool backface = depth_control >> 7 & 1;
  auto no_op = [&](uint32_t at, uint32_t ref_mask) {
    return (depth_control >> at & 7) == 7 &&
           (!(ref_mask >> 16 & 0xFF) || !(depth_control >> (at + 6) & 7 || depth_control >> (at + 9) & 7));
  };
  bool z_enable = depth_control >> 1 & 1,
       stencil = depth_control & 1 && !(no_op(8, GpuRegister(kRegRbStencilRefMask)) &&
                                        (!backface || no_op(20, GpuRegister(kRegRbStencilRefMaskBf)))),
       depth = z_enable || stencil;
  if (!colors && !depth) return;

  // The guest indices (counted from 0, or the packet's or memory's, with
  // strips' and fans' reset index where PA_SU_SC_MODE_CNTL's
  // multi_prim_ib_ena, bit 21, enables VGT_MULTI_PRIM_IB_RESET_INDX and a
  // 16-bit index can hold it, Xenia's rule), as the host draws them: a list
  // of the guest's, or rectangles and points expanded from them. Then the
  // range of the vertex indices (offset, clamped) the vertex buffers'
  // fetches read.
  bool indexed = source != kSourceAuto, wide = initiator >> 11 & 1;
  uint32_t reset_index = GpuRegister(kRegVgtMultiPrimIbResetIndx) & 0xFFFFFF;
  bool reset = indexed && mode_cntl >> 21 & 1 && (wide || reset_index <= 0xFFFF) &&
               (prim == kPrimLineStrip || prim == kPrimTriangleFan || prim == kPrimTriangleStrip ||
                prim == kPrimLineLoop);
  if (reset && prim == kPrimLineLoop) Fatal(nullptr, "GPU: a line loop with a reset index (not implemented yet)");
  std::vector<uint32_t> raw;
  if (indexed) raw = RawIndices(initiator, count, index_base, index_size, immediate, reset, reset_index);
  Kind kind = prim == kPrimRectangleList ? kKindRectangles : prim == kPrimPointList ? kKindPoints : kKindList;
  std::vector<uint32_t> host;
  if (kind == kKindList) {
    if (!indexed)
      for (uint32_t i = 0; i < count; i++) raw.push_back(i);
    host = Decompose(prim, raw);
    if (host.empty()) return;
    if (vi.vertex_kill) kind = lines ? kKindKillLines : kKindKillTriangles;
  }
  uint32_t offset = GpuRegister(kRegVgtIndxOffset), index_min = GpuRegister(kRegVgtMinVtxIndx) & 0xFFFFFF,
           index_max = GpuRegister(kRegVgtMaxVtxIndx) & 0xFFFFFF;
  auto vertex_index = [&](uint32_t i) { return std::clamp((i + offset) & 0xFFFFFF, index_min, index_max); };
  uint32_t lo = UINT32_MAX, hi = 0;
  const std::vector<uint32_t>& used = kind == kKindRectangles || kind == kKindPoints ? raw : host;
  for (uint32_t i : used) {
    lo = std::min(lo, vertex_index(i));
    hi = std::max(hi, vertex_index(i));
  }
  for (uint32_t i = 0; used.empty() && i < count; i++) {
    lo = std::min(lo, vertex_index(i));
    hi = std::max(hi, vertex_index(i));
  }
  std::vector<uint32_t> vf;
  std::vector<gfx::BufferData> buffers;
  for (const ShaderVertexBuffer& b : vi.vertex_buffers) {
    uint32_t w0 = GpuRegister(kRegShaderConstantFetch + 2 * b.constant),
             w1 = GpuRegister(kRegShaderConstantFetch + 2 * b.constant + 1);
    if ((w0 & 3) != 3) Fatal(nullptr, "GPU: vertex fetch constant %u is of type %u", b.constant, w0 & 3);
    uint32_t pa = w0 & 0x1FFFFFFC, words = w1 >> 2 & 0xFFFFFF;
    if (uint64_t(pa) + 4 * uint64_t(words) > kPhysicalSize)
      Fatal(nullptr, "GPU: vertex buffer %x (%u words) outside physical memory", pa, words);
    int64_t first = 0, end = words;
    if (b.stride) {
      first = std::clamp<int64_t>(int64_t(lo) * b.stride, 0, words);
      end = std::clamp<int64_t>(int64_t(hi) * b.stride + b.extent, first, words);
    }
    vf.insert(vf.end(), {w1 & 3, uint32_t(first), uint32_t(end - first), 0});
    // The words are copied as the draw is recorded.
    LandResolves(pa + 4 * uint32_t(first), pa + 4 * uint32_t(end));
    // A buffer of no words still names one, which nothing reads.
    buffers.push_back(
        {Mem() + PhysicalToVirtual(pa + 4 * uint32_t(first)), std::max(4u, 4 * uint32_t(end - first)), 0, 0});
  }
  if (kind == kKindRectangles || kind == kKindPoints) {
    if (indexed) buffers.push_back({raw.data(), 4 * count, 0, 0});
    else buffers.push_back({&offset, 4, 0, 0});
  } else if (kind != kKindList) {
    buffers.push_back({host.data(), uint32_t(4 * host.size()), 0, 0});
  }

  // Window pixels onto the whole host target, in samples: the viewport's
  // scale and offset where PA_CL_VTE_CNTL enables them, the window offset
  // where PA_SU_SC_MODE_CNTL does, half a pixel for Direct3D's pixel
  // centres (PA_SU_VTX_CNTL bit 0 clear), each pixel 2^shift samples, then
  // to NDC over the target's size (Xenia's GetHostViewportInfo, as for
  // draws without clipping). The host's NDC Y points up the target
  // (SDL_GPU's, checked by the test title's draws), window Y down.
  gfx::Rect sc = Scissor();
  sc = {sc.left << shift_x, sc.top << shift_y, sc.right << shift_x, sc.bottom << shift_y};
  uint32_t vte = GpuRegister(kRegPaClVteCntl), clip = GpuRegister(kRegPaClClipCntl);
  float scale[3], vport_offset[3];
  for (uint32_t i = 0; i < 3; i++) {
    scale[i] = vte >> (2 * i) & 1 ? RegFloat(kRegPaClVportXscale + 2 * i) : 1.0f;
    vport_offset[i] = vte >> (2 * i + 1) & 1 ? RegFloat(kRegPaClVportXscale + 2 * i + 1) : 0.0f;
  }
  float add[2] = {0, 0};
  if (mode_cntl >> 16 & 1) {
    uint32_t off = GpuRegister(kRegWindowOffset);
    add[0] = float(Signed(off, 15));
    add[1] = float(Signed(off >> 16, 15));
  }
  if (!(GpuRegister(kRegPaSuVtxCntl) & 1)) {
    add[0] += 0.5f;
    add[1] += 0.5f;
  }
  // Z, Xenia's way: without clipping (PA_CL_CLIP_CNTL bit 16) the
  // viewport's Z in a 0-1 host range, unclipped; with it, the guest's clip
  // space Z (0..W, or -W..W where bit 19 is clear, made 0..W) clipped and
  // mapped by the host viewport's depth range, the viewport's (offset,
  // offset + scale; for -W..W offset - scale, offset + scale) within 0-1,
  // or all of it when the pixel shader writes depth; reversed ranges as
  // forward ones with Z flipped.
  float ndc_z[2] = {scale[2], vport_offset[2]}, z_min = 0, z_max = 1;
  bool clip_disable = clip >> 16 & 1;
  if (!clip_disable) {
    bool dx = clip >> 19 & 1;
    float z0 = dx ? vport_offset[2] : vport_offset[2] - scale[2], zs = dx ? scale[2] : 2 * scale[2];
    ndc_z[0] = ndc_z[1] = dx ? 0.0f : 0.5f;
    if (dx) ndc_z[0] = 1;
    if (!pi.depth) {
      z_min = std::clamp(z0, 0.0f, 1.0f);
      z_max = std::clamp(z0 + zs, 0.0f, 1.0f);
      if (z_min > z_max) {
        std::swap(z_min, z_max);
        ndc_z[0] = -ndc_z[0];
        ndc_z[1] = 1 - ndc_z[1];
      }
    }
  }

  std::lock_guard<std::mutex> l(HostDeviceLock());
  // The targets, their rows in use reaching the scissor's bottom.
  std::vector<Target*> bound;
  uint32_t color_info[4] = {};
  for (uint32_t i = 0; i < colors; i++) {
    color_info[i] = GpuRegister(kRegRbColorInfo[i]);
    bound.push_back(&GetTarget(false, color_info[i] & 0xFFF, pitch, msaa, color_info[i] >> 16 & 0xF));
  }
  uint32_t depth_info = GpuRegister(kRegRbDepthInfo), depth_format = depth_info >> 16 & 1;
  if (depth) bound.push_back(&GetTarget(true, depth_info & 0xFFF, pitch, msaa, depth_format));
  BindTargets(bound, uint32_t(std::max(sc.bottom, 0)));
  // A stencil test reads the depth target's stencil, in all its rows in
  // use as they now are: loaded first where it is not (by Target's
  // reasoning the EDRAM's words still hold it).
  if (depth && stencil && bound.back()->stencil == kStencilUnloaded) {
    Target& t = *bound.back();
    int v = TilesStencil(t, 0, t.rows);
    LoadStencil(t, 0, t.rows, v, false);
    t.stencil = v >= 0 ? v : kStencilLoaded;
  }
  const Target& first = *bound[0];

  float extent[2] = {float(first.w), float(first.h)};
  uint32_t shift[2] = {shift_x, shift_y};
  float sys[12] = {};
  for (uint32_t i = 0; i < 2; i++) {
    float samples = float(1u << shift[i]);
    sys[i] = scale[i] * samples * 2.0f / extent[i];
    sys[4 + i] = ((vport_offset[i] + add[i]) * samples - extent[i] * 0.5f) * 2.0f / extent[i];
  }
  sys[1] = -sys[1];
  sys[5] = -sys[5];
  sys[2] = ndc_z[0];
  sys[6] = ndc_z[1];
  // The alpha test (RB_COLORCONTROL bits 0-3, its function when bit 3 is
  // set, else always); each target's output scaled by 2^RB_COLOR_INFO's
  // exponent bias (bits 20-25), a further 32nd for the fixed-point
  // formats (Xenia's).
  uint32_t sys_words[kSysWords] = {};
  std::memcpy(sys_words, sys, 8 * 4);
  sys_words[8] = vte;
  sys_words[9] = offset;
  sys_words[10] = indexed ? 1 : 0;
  sys_words[11] = color_control >> 3 & 1 ? color_control & 7 : 7;
  for (uint32_t i = 0; i < 4; i++) {
    uint32_t format = color_info[i] >> 16 & 0xF;
    int32_t bias = Signed(color_info[i] >> 20, 6) - (format == 4 || format == 5 ? 5 : 0);
    float color_scale = std::ldexp(1.0f, bias);
    std::memcpy(&sys_words[12 + i], &color_scale, 4);
    sys_words[16 + i] = format == 1;
  }
  float alpha_ref = RegFloat(kRegRbAlphaRef);
  std::memcpy(&sys_words[20], &alpha_ref, 4);
  // Points: PA_SU_POINT_SIZE's half width (bits 16-31) and height (0-15)
  // and PA_SU_POINT_MINMAX's half sizes (min 0-15, max 16-31), 12.4 fixed
  // point, as diameters (Xenia's); a diameter in pixels as a radius in
  // NDC, in samples.
  uint32_t point_size = GpuRegister(kRegPaSuPointSize), point_minmax = GpuRegister(kRegPaSuPointMinmax);
  float point[8] = {float(point_size >> 16) / 8.0f, float(point_size & 0xFFFF) / 8.0f,
                    float(point_minmax & 0xFFFF) / 8.0f, float(point_minmax >> 16) / 8.0f,
                    float(1u << shift_x) / extent[0], -float(1u << shift_y) / extent[1], 0, 0};
  std::memcpy(&sys_words[24], point, sizeof point);
  // Vertex kill (PA_CL_CLIP_CNTL's vtx_kill_or, bit 21); the register the
  // pixel parameters go to (SQ_CONTEXT_MISC's param_gen_pos, bits 8-15,
  // where SQ_PROGRAM_CNTL's param_gen, bit 18, enables them and it is an
  // interpolator, Xenia's rule); the primitive; the index bounds; the
  // sample shifts.
  uint32_t param_gen = GpuRegister(kRegSqContextMisc) >> 8 & 0xFF;
  sys_words[32] = clip >> 21 & 1;
  sys_words[33] = ps && program_cntl >> 18 & 1 && param_gen < 16 ? param_gen : uint32_t(-1);
  sys_words[34] = polygons ? 0 : lines ? 1 : 2;
  sys_words[36] = index_min;
  sys_words[37] = index_max;
  sys_words[38] = shift_x;
  sys_words[39] = shift_y;
  // The stages' textures.
  std::vector<TextureBinding> vt, pt;
  std::vector<gfx::TextureId> vti, pti;
  std::vector<gfx::SamplerDesc> vts, pts;
  uint64_t stacked = 0;
  for (uint32_t n = 0; n < vi.textures.size(); n++) {
    vt.push_back(BindTexture(vi.textures[n]));
    vti.push_back(vt.back().image);
    vts.push_back(vt.back().sampler);
    stacked |= uint64_t(vt.back().stacked) << n;
  }
  for (uint32_t n = 0; n < pi.textures.size(); n++) {
    pt.push_back(BindTexture(pi.textures[n]));
    pti.push_back(pt.back().image);
    pts.push_back(pt.back().sampler);
    stacked |= uint64_t(pt.back().stacked) << (32 + n);
  }
  std::vector<uint32_t> vu = Uniforms(vi, vf, vt, sys_words), pu = Uniforms(pi, {}, pt, sys_words);
  std::vector<uint32_t> vc = Constants(vi), pc = Constants(pi);
  if (!vc.empty()) buffers.push_back({vc.data(), uint32_t(4 * vc.size()), 0, 0});
  gfx::BufferData pcb{pc.data(), uint32_t(4 * pc.size()), 0, 0};

  gfx::ProgramDraw d;
  d.program = Program(*g_vertex_shader, ps, kind, stacked);
  d.state.topology = lines ? gfx::Topology::LineList : gfx::Topology::TriangleList;
  // Blending per target (RB_BLENDCONTROL: colour source factor bits 0-4,
  // operation 5-7, destination 8-12, alpha the same from 16), off where it
  // is ONE, ZERO, ADD for both; the blend constant RB_BLEND_RED-ALPHA,
  // made all alpha where a colour factor is constant alpha (the host has
  // no such factor). RB_COLOR_MASK: four bits a target.
  uint32_t mask = GpuRegister(kRegRbColorMask);
  bool constant_color = false, constant_alpha = false;
  for (uint32_t i = 0; i < colors; i++) {
    uint32_t bc = GpuRegister(kRegRbBlendControl[i]);
    gfx::TargetBlend& b = d.state.blend[i];
    b.write_mask = uint8_t(mask >> (4 * i) & 0xF);
    d.targets[i] = {bound[i]->image, 0};
    if ((bc & 0x1FFF1FFF) == 0x00010001) continue;
    bool ca = false, unused = false;
    b.enable = true;
    b.src_color = Factor(bc & 0x1F, &ca);
    b.color_op = Op(bc >> 5 & 7);
    b.dst_color = Factor(bc >> 8 & 0x1F, &ca);
    b.src_alpha = Factor(bc >> 16 & 0x1F, &unused);
    b.alpha_op = Op(bc >> 21 & 7);
    b.dst_alpha = Factor(bc >> 24 & 0x1F, &unused);
    constant_alpha |= ca;
    for (uint32_t f : {bc & 0x1F, bc >> 8 & 0x1F}) constant_color |= f == 12 || f == 13;
  }
  if (constant_color && constant_alpha)
    Fatal(nullptr, "GPU: a draw blends by both the constant colour and its alpha (not implemented yet)");
  float blend[4];
  for (uint32_t i = 0; i < 4; i++) blend[i] = RegFloat(kRegRbBlendRed + (constant_alpha ? 3 : i));
  d.state.blend_constant = {blend[0], blend[1], blend[2], blend[3]};

  // Depth and stencil (RB_DEPTHCONTROL; RB_STENCILREFMASK: reference bits
  // 0-7, read mask 8-15, write mask 16-23); back faces their own where bit
  // 7 says, else the front's. Compare functions and stencil operations
  // are encoded in the host's order.
  if (depth) {
    d.depth = bound.back()->ds;
    d.state.depth_test = z_enable;
    d.state.depth_write = z_enable && (depth_control >> 2 & 1);
    d.state.depth_compare = gfx::CompareOp(depth_control >> 4 & 7);
    d.state.stencil = stencil;
    auto face = [&](gfx::StencilFace& f, uint32_t at) {
      f.compare = gfx::CompareOp(depth_control >> at & 7);
      f.fail = gfx::StencilOp(depth_control >> (at + 3) & 7);
      f.pass = gfx::StencilOp(depth_control >> (at + 6) & 7);
      f.depth_fail = gfx::StencilOp(depth_control >> (at + 9) & 7);
    };
    face(d.state.stencil_front, 8);
    face(d.state.stencil_back, backface ? 20 : 8);
    uint32_t ref = GpuRegister(kRegRbStencilRefMask);
    if (stencil && backface && (GpuRegister(kRegRbStencilRefMaskBf) & 0xFFFFFF) != (ref & 0xFFFFFF))
      Fatal(nullptr, "GPU: back faces with their own stencil reference or masks (not implemented yet)");
    d.state.stencil_ref = uint8_t(ref);
    d.state.stencil_read_mask = uint8_t(ref >> 8);
    // A draw whose stencil test can write: its stencil no longer known,
    // nor the EDRAM's.
    auto writes = [](const gfx::StencilFace& f) {
      return f.fail != gfx::StencilOp::Keep || f.pass != gfx::StencilOp::Keep || f.depth_fail != gfx::StencilOp::Keep;
    };
    if (stencil && ref >> 16 & 0xFF && (writes(d.state.stencil_front) || writes(d.state.stencil_back))) {
      bound.back()->stencil = kStencilLoaded;
      bound.back()->stencil_dirty = true;
    }
    d.state.stencil_write_mask = uint8_t(ref >> 16);
    // Polygon offset (PA_SU_SC_MODE_CNTL bits 11-12), the front's unless
    // front faces are culled, as Xenia picks it: the offset in units of
    // the depth format's (2^24 - 1 for D24S8, 2^24 for D24FS8, as Xenia
    // takes them to a 32-bit float buffer); the slope scale per 1/16
    // pixel, and per sample here.
    // Lines and points take the front's where bit 13 enables it.
    float slope = 0, constant = 0;
    if (polygons ? mode_cntl >> 11 & 1 && !(mode_cntl & 1) : mode_cntl >> 13 & 1) {
      slope = RegFloat(kRegPaSuPolyOffsetFrontScale);
      constant = RegFloat(kRegPaSuPolyOffsetFrontScale + 1);
    }
    if (polygons && mode_cntl >> 12 & 1 && !(mode_cntl >> 1 & 1) && !slope && !constant) {
      slope = RegFloat(kRegPaSuPolyOffsetFrontScale + 2);
      constant = RegFloat(kRegPaSuPolyOffsetFrontScale + 3);
    }
    d.state.depth_bias = constant * (depth_format == kDepthD24S8 ? 16777215.0f : 16777216.0f);
    d.state.depth_bias_slope = slope * (1.0f / 16.0f) * float(1u << std::max(shift_x, shift_y));
  }
  d.state.depth_clip = !clip_disable;
  // Culling of polygons (PA_SU_SC_MODE_CNTL bits 0-1) with the front
  // face's winding (bit 2: clockwise when set) as the window shows it.
  d.state.cull = !polygons       ? gfx::CullMode::None
                 : mode_cntl & 1 ? gfx::CullMode::Front
                 : mode_cntl & 2 ? gfx::CullMode::Back
                                 : gfx::CullMode::None;
  // The host's windings are as the window shows them too (checked by the
  // test title's culled draws).
  d.state.front_ccw = !(mode_cntl >> 2 & 1);
  d.viewport = {0, 0, uint32_t(first.w), uint32_t(first.h), z_min, z_max};
  d.scissor = sc;
  if (kind == kKindRectangles) {
    d.vertex_count = count / 3 * 6;
  } else if (kind == kKindPoints) {
    d.vertex_count = count * 6;
  } else if (kind == kKindList) {
    d.vertex_count = uint32_t(host.size());
    d.indices = host.data();
    d.index_size = 4;
  } else {
    d.vertex_count = uint32_t(host.size());
  }
  d.vs.buffers = buffers.data();
  d.vs.textures = vti.data();
  d.vs.samplers = vts.data();
  d.fs.textures = pti.data();
  d.fs.samplers = pts.data();
  d.fs.buffers = &pcb;
  d.vs.uniforms = vu.data();
  d.fs.uniforms = pu.data();
  HostDevice().DrawProgram(d);
  for (Target* t : bound) t->dirty = true;
}

void StoreTargets() {
  std::lock_guard<std::mutex> l(HostDeviceLock());
  for (auto& t : g_state.targets) Store(*t);
}

void ClearTargets(bool depth, uint32_t base, uint32_t width, uint32_t pitch, uint32_t msaa, uint32_t format,
                  int32_t x0, int32_t y0, int32_t x1, int32_t y1, uint32_t lo, uint32_t hi) {
  uint32_t shift_x = msaa == 2, shift_y = msaa >= 1;
  // The tiles cleared: rows 0 to y1 of the surface (a superset).
  Target cleared{};
  cleared.base = base;
  cleared.pitch = pitch;
  cleared.wide = !depth && Wide(format);
  uint32_t rows = uint32_t(y1) << shift_y;
  // The EDRAM's tiles the clear wrote, every sample of the rectangle.
  TilesWritten(base, pitch * (cleared.wide ? 2 : 1), cleared.wide ? 40 : 80, uint32_t(x0) << shift_x,
               uint32_t(y0) << shift_y, uint32_t(x1) << shift_x, rows, depth ? int(lo & 0xFF) : -1);
  for (auto& c : g_state.targets) {
    Target& t = *c;
    if (!Overlap(cleared, rows, t, t.rows)) continue;
    // The target of the cleared surface, all of whose rows in use and the
    // surface's width the rectangle covers, holds the cleared words as a
    // load would give them (the columns of its last tile past the
    // surface's width too, which nothing of the surface uses); the others
    // there load again.
    if (t.depth != depth || t.base != base || t.pitch != pitch || t.msaa != msaa || t.format != format || x0 ||
        y0 || uint32_t(x1) < width || rows < t.rows) {
      t.valid = false;
      continue;
    }
    gfx::Device& dev = HostDevice();
    if (depth) {
      float z = format == kDepthD24S8 ? Unorm24(lo >> 8) : std::min(F20e4(lo >> 8), 1.0f);
      dev.ClearDepth(t.ds, z, int(lo & 0xFF), 0);
      t.stencil = int(lo & 0xFF);
      t.stencil_dirty = false;
    } else {
      dev.ClearImage(t.image, 0, LoadedColor(format, lo, hi));
    }
    t.valid = true;
    t.dirty = false;
  }
}

uint64_t TargetLoads() { return g_loads; }
uint64_t StencilBitDraws(bool stores) { return stores ? g_store_bits : g_load_bits; }

}  // namespace recomp::xbox360
