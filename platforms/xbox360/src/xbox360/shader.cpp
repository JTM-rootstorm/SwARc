// Xenos shader microcode to GLSL (shader.h).
//
// The microcode's encoding and its instructions' meaning are as Xenia
// documents them (its gpu/ucode.h, BSD-3, from the XNA assembler, the
// R400 sequencer documents and freedreno); docs/xenos.md, "Shaders",
// says what is taken from where.
//
// xe_degamma is ported from Xenia's PWLGammaToLinear (its shader
// translators):
// Copyright (c) 2015, Ben Vanik. All rights reserved. BSD 3-Clause; the
// licence is in NOTICE.

#include "xbox360/shader.h"

#include <algorithm>
#include <cstdarg>
#include <cstdio>

#include "ppc/fatal.h"

namespace recomp::xbox360 {
namespace {

using ppc::Fatal;

uint32_t Bits(uint32_t v, uint32_t lo, uint32_t n) { return v >> lo & ((1u << n) - 1); }
int32_t SBits(uint32_t v, uint32_t lo, uint32_t n) { return int32_t(v << (32 - lo - n)) >> (32 - n); }

// Control flow opcodes.
enum : uint32_t {
  kCfNop = 0, kCfExec, kCfExecEnd, kCfCondExec, kCfCondExecEnd, kCfCondExecPred, kCfCondExecPredEnd,
  kCfLoopStart, kCfLoopEnd, kCfCondCall, kCfReturn, kCfCondJmp, kCfAlloc, kCfCondExecPredClean,
  kCfCondExecPredCleanEnd, kCfMarkVsFetchDone,
};
bool IsExec(uint32_t op) {
  return (op >= kCfExec && op <= kCfCondExecPredEnd) || op == kCfCondExecPredClean || op == kCfCondExecPredCleanEnd;
}
bool EndsShader(uint32_t op) {
  return op == kCfExecEnd || op == kCfCondExecEnd || op == kCfCondExecPredEnd || op == kCfCondExecPredCleanEnd;
}

struct Cf {
  uint32_t d0, d1;
  uint32_t op() const { return d1 >> 12; }
};

struct Translator {
  const uint32_t* w;
  uint32_t count;
  ShaderInfo info;
  std::string out;
  bool const_relative = false;
  uint32_t max_const = 0;
  // The last full vertex fetch, whose address a mini fetch uses.
  int vf_buffer = -1;
  uint32_t vf_stride = 0;
  // Whether r0, the vertex index, may be written (vertex buffers' ranges).
  bool r0_written = false;

  [[gnu::format(printf, 2, 3)]] void P(const char* fmt, ...) {
    char b[1024];
    va_list ap;
    va_start(ap, fmt);
    std::vsnprintf(b, sizeof b, fmt, ap);
    va_end(ap);
    out += b;
  }

  static std::string F(const char* fmt, ...) {
    char b[512];
    va_list ap;
    va_start(ap, fmt);
    std::vsnprintf(b, sizeof b, fmt, ap);
    va_end(ap);
    return b;
  }

  static std::string Bool(uint32_t i) {
    return F("((xe_bool[%u][%u] >> %uu & 1u) != 0u)", i >> 7, i >> 5 & 3, i & 31);
  }

  // A temporary register written, relative to the loop index or not.
  std::string Dest(uint32_t index, bool relative) {
    r0_written |= relative || index == 0;
    return Reg(index, relative);
  }

  // A temporary register, relative to the loop index or not.
  static std::string Reg(uint32_t index, bool relative) {
    return relative ? F("xe_r[(%u + xe_al) & 63]", index) : F("xe_r[%u]", index);
  }

  // ---- ALU ------------------------------------------------------------------
  // Operand i (1-3) of an ALU instruction as a vec4, swizzled.
  std::string Operand(uint32_t x, uint32_t y, uint32_t z, uint32_t i) {
    uint32_t reg = Bits(z, 8 * (3 - i), 8), swiz = Bits(y, 8 * (3 - i), 8);
    bool temp = Bits(z, 32 - i, 1), negate = Bits(y, 27 - i, 1);
    std::string e;
    bool abs;
    if (temp) {
      e = Reg(reg & 0x3F, reg & 0x40);
      abs = reg & 0x80;
    } else {
      e = Constant(x, y, z, i, reg);
      abs = Bits(x, 7, 1);
    }
    char sw[5] = {};
    for (uint32_t c = 0; c < 4; c++) sw[c] = "xyzw"[((swiz >> (2 * c)) + c) & 3];
    e += std::string(".") + sw;
    if (abs) e = "abs(" + e + ")";
    if (negate) e = "(-" + e + ")";
    return e;
  }

  // Constant `reg` as operand i: plain, or relative to a0 or the loop
  // index. Which of the two addressing bits is operand i's depends on
  // how many operands before it are constants.
  std::string Constant(uint32_t, uint32_t y, uint32_t z, uint32_t i, uint32_t reg) {
    bool t1 = Bits(z, 31, 1), t2 = Bits(z, 30, 1);
    bool first = i == 1 || (i == 2 && t1) || (i == 3 && t1 && t2);
    bool addressed = Bits(y, first ? 31 : 30, 1);
    if (!addressed) {
      max_const = std::max(max_const, reg + 1);
      return F("xe_c(%u)", reg);
    }
    const_relative = true;
    return F("xe_c(clamp(%u + %s, 0, 255))", reg, Bits(y, 29, 1) ? "xe_a0" : "xe_al");
  }

  static const char* Mask(uint32_t m) {
    static const char* const k[16] = {"", "x", "y", "xy", "z", "xz", "yz", "xyz",
                                      "w", "xw", "yw", "xyw", "zw", "xzw", "yzw", "xyzw"};
    return k[m];
  }

  // Where an export register is kept.
  std::string Export(uint32_t r) {
    if (info.pixel) {
      if (r < 4) {
        info.colors |= 1u << r;
        return F("xe_oc[%u]", r);
      }
      if (r == 61) {
        info.depth = true;
        return "xe_od";
      }
    } else {
      if (r < 16) {
        info.interpolators |= 1u << r;
        return F("xe_i[%u]", r);
      }
      if (r == 62) return "xe_pos";
      if (r == 63) return "xe_pts";
    }
    Fatal(nullptr, "GPU: a %s shader exports to register %u (memory export is not implemented yet)",
          info.pixel ? "pixel" : "vertex", r);
  }

  void Kill(const char* cond) {
    if (!info.pixel) Fatal(nullptr, "GPU: a vertex shader with a pixel kill");
    info.kills = true;
    P("    if (%s) { discard; }\n", cond);
  }

  void Alu(uint32_t x, uint32_t y, uint32_t z) {
    uint32_t vop = Bits(z, 24, 5), sop = Bits(x, 26, 6);
    uint32_t vmask = Bits(x, 16, 4), smask = Bits(x, 20, 4);
    bool exp = Bits(x, 15, 1);
    bool predicated = Bits(y, 28, 1);
    if (predicated) P("  if (xe_p0 == %s) {\n", Bits(y, 27, 1) ? "true" : "false");
    else P("  {\n");
    // The vector operation.
    bool v_state = (vop >= 20 && vop <= 27) || vop == 29;
    bool v_used = vmask || v_state;
    if (v_used) {
      std::string a = Operand(x, y, z, 1), b, c;
      bool two = !(vop >= 8 && vop <= 10) && vop != 19;
      bool three = (vop >= 11 && vop <= 14) || vop == 17;
      if (two) b = Operand(x, y, z, 2);
      if (three) c = Operand(x, y, z, 3);
      const char* A = a.c_str();
      const char* B = b.c_str();
      const char* Cc = c.c_str();
      switch (vop) {
        case 0: P("    vec4 v = %s + %s;\n", A, B); break;
        case 1: P("    vec4 v = xe_mul(%s, %s);\n", A, B); break;
        case 2: P("    vec4 v = xe_max(%s, %s);\n", A, B); break;
        case 3: P("    vec4 v = xe_min(%s, %s);\n", A, B); break;
        case 4: P("    vec4 v = vec4(equal(%s, %s));\n", A, B); break;
        case 5: P("    vec4 v = vec4(greaterThan(%s, %s));\n", A, B); break;
        case 6: P("    vec4 v = vec4(greaterThanEqual(%s, %s));\n", A, B); break;
        case 7: P("    vec4 v = vec4(notEqual(%s, %s));\n", A, B); break;
        case 8: P("    vec4 v = %s; v = v - floor(v);\n", A); break;
        case 9: P("    vec4 v = trunc(%s);\n", A); break;
        case 10: P("    vec4 v = floor(%s);\n", A); break;
        case 11: P("    vec4 v = xe_mul(%s, %s) + %s;\n", A, B, Cc); break;
        case 12: P("    vec4 v = mix(%s, %s, equal(%s, vec4(0.0)));\n", Cc, B, A); break;
        case 13: P("    vec4 v = mix(%s, %s, greaterThanEqual(%s, vec4(0.0)));\n", Cc, B, A); break;
        case 14: P("    vec4 v = mix(%s, %s, greaterThan(%s, vec4(0.0)));\n", Cc, B, A); break;
        case 15: P("    vec4 m = xe_mul(%s, %s); vec4 v = vec4(m.x + m.y + m.z + m.w);\n", A, B); break;
        case 16: P("    vec4 m = xe_mul(%s, %s); vec4 v = vec4(m.x + m.y + m.z);\n", A, B); break;
        case 17: P("    vec4 m = xe_mul(%s, %s); vec4 v = vec4(m.x + m.y + (%s).x);\n", A, B, Cc); break;
        case 18: P("    vec4 v = xe_cube(%s);\n", A); break;
        case 19: P("    vec4 v = xe_max4(%s);\n", A); break;
        case 20: case 21: case 22: case 23: {
          static const char* const k[4] = {"==", "!=", ">", ">="};
          P("    vec4 va = %s, vb = %s;\n", A, B);
          P("    bool vp = va.w == 0.0 && vb.w %s 0.0;\n", k[vop - 20]);
          P("    vec4 v = vec4(va.x == 0.0 && vb.x %s 0.0 ? 0.0 : va.x + 1.0);\n", k[vop - 20]);
          break;
        }
        case 24: case 25: case 26: case 27: {
          static const char* const k[4] = {"equal", "greaterThan", "greaterThanEqual", "notEqual"};
          P("    bool vk = any(%s(%s, %s));\n", k[vop - 24], A, B);
          P("    vec4 v = vec4(vk ? 1.0 : 0.0);\n");
          break;
        }
        case 28: P("    vec4 va = %s, vb = %s; vec4 v = vec4(1.0, xe_muls(va.y, vb.y), va.z, vb.w);\n", A, B); break;
        case 29:
          P("    vec4 va = %s;\n", A);
          P("    int vaddr = int(clamp(floor(va.w + 0.5), -256.0, 255.0));\n");
          P("    vec4 v = xe_max(va, %s);\n", B);
          break;
        default: Fatal(nullptr, "GPU: shader vector operation %u", vop);
      }
    }
    // The scalar operation, on the third operand's W and X, or on a
    // constant's W and a register's X.
    bool s_used = smask || sop != 50;
    if (sop == 41 || sop > 50) Fatal(nullptr, "GPU: shader scalar operation %u", sop);
    if (s_used) {
      if (sop >= 42 && sop <= 47) {
        uint32_t reg = Bits(z, 0, 8), swiz = Bits(y, 0, 8);
        // The constant is the third operand whatever its select bit says.
        std::string cst = Constant(x, y, z, 3, reg);
        uint32_t treg = (sop & 1) | Bits(z, 29, 1) << 1 | (swiz & 0x3C);
        std::string tmp = F("xe_r[%u]", treg);
        char ca = "xyzw"[((swiz >> 6) + 3) & 3], cb = "xyzw"[swiz & 3];
        std::string a = cst + "." + ca, b = tmp + "." + cb;
        if (Bits(x, 7, 1)) {
          a = "abs(" + a + ")";
          b = "abs(" + b + ")";
        }
        if (Bits(y, 24, 1)) {
          a = "(-" + a + ")";
          b = "(-" + b + ")";
        }
        P("    float sa = %s, sb = %s;\n", a.c_str(), b.c_str());
      } else if (sop != 33 && sop != 50) {
        P("    vec4 so = %s; float sa = so.w, sb = so.x;\n", Operand(x, y, z, 3).c_str());
      }
      switch (sop) {
        case 0: P("    float s = sa + sb;\n"); break;
        case 1: P("    float s = sa + xe_ps;\n"); break;
        case 2: P("    float s = xe_muls(sa, sb);\n"); break;
        case 3: P("    float s = xe_muls(sa, xe_ps);\n"); break;
        case 4:
          P("    float s = xe_ps == -XE_MAX || isinf(xe_ps) || isnan(xe_ps) || isinf(sb) || isnan(sb) || "
            "sb <= 0.0 ? -XE_MAX : xe_muls(sa, xe_ps);\n");
          break;
        case 5: case 23: case 24: P("    float s = sa >= sb ? sa : sb;\n"); break;
        case 6: P("    float s = sa < sb ? sa : sb;\n"); break;
        case 7: P("    float s = float(sa == 0.0);\n"); break;
        case 8: P("    float s = float(sa > 0.0);\n"); break;
        case 9: P("    float s = float(sa >= 0.0);\n"); break;
        case 10: P("    float s = float(sa != 0.0);\n"); break;
        case 11: P("    float s = sa - floor(sa);\n"); break;
        case 12: P("    float s = trunc(sa);\n"); break;
        case 13: P("    float s = floor(sa);\n"); break;
        case 14: P("    float s = exp2(sa);\n"); break;
        case 15: P("    float s = log2(sa); if (isinf(s) && s < 0.0) s = -XE_MAX;\n"); break;
        case 16: P("    float s = log2(sa);\n"); break;
        case 17: P("    float s = xe_clampinf(1.0 / sa);\n"); break;
        case 18: P("    float s = xe_zeroinf(1.0 / sa);\n"); break;
        case 19: P("    float s = 1.0 / sa;\n"); break;
        case 20: P("    float s = xe_clampinf(inversesqrt(sa));\n"); break;
        case 21: P("    float s = xe_zeroinf(inversesqrt(sa));\n"); break;
        case 22: P("    float s = inversesqrt(sa);\n"); break;
        case 25: P("    float s = sa - sb;\n"); break;
        case 26: P("    float s = sa - xe_ps;\n"); break;
        case 27: P("    bool sp = sa == 0.0; float s = sp ? 0.0 : 1.0;\n"); break;
        case 28: P("    bool sp = sa != 0.0; float s = sp ? 0.0 : 1.0;\n"); break;
        case 29: P("    bool sp = sa > 0.0; float s = sp ? 0.0 : 1.0;\n"); break;
        case 30: P("    bool sp = sa >= 0.0; float s = sp ? 0.0 : 1.0;\n"); break;
        case 31: P("    bool sp = sa == 1.0; float s = sp ? 0.0 : sa == 0.0 ? 1.0 : sa;\n"); break;
        case 32: P("    bool sp = sa - 1.0 <= 0.0; float s = sp ? 0.0 : sa - 1.0;\n"); break;
        case 33: P("    bool sp = false; float s = XE_MAX;\n"); break;
        case 34: P("    bool sp = sa == 0.0; float s = sp ? 0.0 : sa;\n"); break;
        case 35: P("    bool sk = sa == 0.0; float s = float(sk);\n"); break;
        case 36: P("    bool sk = sa > 0.0; float s = float(sk);\n"); break;
        case 37: P("    bool sk = sa >= 0.0; float s = float(sk);\n"); break;
        case 38: P("    bool sk = sa != 0.0; float s = float(sk);\n"); break;
        case 39: P("    bool sk = sa == 1.0; float s = float(sk);\n"); break;
        case 40: P("    float s = sqrt(sa);\n"); break;
        case 42: case 43: P("    float s = xe_muls(sa, sb);\n"); break;
        case 44: case 45: P("    float s = sa + sb;\n"); break;
        case 46: case 47: P("    float s = sa - sb;\n"); break;
        case 48: P("    float s = sin(sa);\n"); break;
        case 49: P("    float s = cos(sa);\n"); break;
        case 50: P("    float s = xe_ps;\n"); break;
      }
    }
    // State, after both have read theirs: the address register, the
    // predicate, kills, the previous scalar.
    if (v_used && vop == 29) P("    xe_a0 = vaddr;\n");
    if (v_used && vop >= 20 && vop <= 23) P("    xe_p0 = vp;\n");
    if (s_used) {
      if (sop == 23) P("    xe_a0 = int(clamp(floor(sa + 0.5), -256.0, 255.0));\n");
      if (sop == 24) P("    xe_a0 = int(clamp(floor(sa), -256.0, 255.0));\n");
      if (sop >= 27 && sop <= 34) P("    xe_p0 = sp;\n");
      if (sop != 50) P("    xe_ps = s;\n");
    }
    // Results. An export takes both operations' in one register, with
    // constants where both masks or (with the relative bit) neither are set.
    const char* vclamp = Bits(x, 24, 1) ? "clamp(v, 0.0, 1.0)" : "v";
    const char* sclamp = Bits(x, 25, 1) ? "clamp(s, 0.0, 1.0)" : "s";
    if (exp) {
      std::string d = Export(Bits(x, 0, 6));
      uint32_t vm = vmask & ~smask, sm = smask & ~vmask, one = vmask & smask;
      uint32_t zero = Bits(x, 14, 1) ? 0xF & ~(vmask | smask) : 0;
      if (!info.pixel && Bits(x, 0, 6) == 63) {
        info.point_size |= ((vmask | smask | zero) & 1) != 0;
        info.vertex_kill |= ((vmask | smask | zero) & 4) != 0;
      }
      if (vm) P("    %s.%s = (%s).%s;\n", d.c_str(), Mask(vm), vclamp, Mask(vm));
      if (sm) P("    %s.%s = vec4(%s).%s;\n", d.c_str(), Mask(sm), sclamp, Mask(sm));
      if (one) P("    %s.%s = vec4(1.0).%s;\n", d.c_str(), Mask(one), Mask(one));
      if (zero) P("    %s.%s = vec4(0.0).%s;\n", d.c_str(), Mask(zero), Mask(zero));
    } else {
      if (vmask)
        P("    %s.%s = (%s).%s;\n", Dest(Bits(x, 0, 6), Bits(x, 6, 1)).c_str(), Mask(vmask), vclamp, Mask(vmask));
      if (smask)
        P("    %s.%s = vec4(%s).%s;\n", Dest(Bits(x, 8, 6), Bits(x, 14, 1)).c_str(), Mask(smask), sclamp,
          Mask(smask));
    }
    if (v_used && vop >= 24 && vop <= 27) Kill("vk");
    if (s_used && sop >= 35 && sop <= 39) Kill("sk");
    P("  }\n");
  }

  // ---- fetches --------------------------------------------------------------
  // A fetch's result `f` to its register through the destination swizzle
  // (a component, 0, 1, or 7 to keep what is there).
  void StoreFetch(uint32_t x, uint32_t y) {
    std::string d = Dest(Bits(x, 12, 6), Bits(x, 18, 1));
    for (uint32_t c = 0; c < 4; c++) {
      uint32_t s = Bits(y, 3 * c, 3);
      if (s == 7) continue;
      if (s < 4) P("    %s.%c = f.%c;\n", d.c_str(), "xyzw"[c], "xyzw"[s]);
      else if (s < 6) P("    %s.%c = %s;\n", d.c_str(), "xyzw"[c], s == 5 ? "1.0" : "0.0");
      else Fatal(nullptr, "GPU: a shader fetch with destination swizzle 6");
    }
  }

  void VertexFetch(uint32_t x, uint32_t y, uint32_t z) {
    if (info.pixel) Fatal(nullptr, "GPU: a pixel shader with a vertex fetch");
    bool mini = Bits(y, 30, 1);
    if (!mini) {
      uint32_t constant = Bits(x, 20, 5) * 3 + Bits(x, 25, 2);
      auto& vb = info.vertex_buffers;
      auto it = std::find_if(vb.begin(), vb.end(), [&](const ShaderVertexBuffer& b) { return b.constant == constant; });
      vf_buffer = int(it - vb.begin());
      vf_stride = Bits(z, 0, 8);
      // Fetched by the vertex index (r0.x), or not.
      bool by_index = Bits(x, 5, 7) == 0 && Bits(x, 30, 2) == 0;
      if (it == vb.end()) vb.push_back({constant, by_index ? vf_stride : 0, 0});
      else if (!by_index || it->stride != vf_stride) it->stride = 0;
      // The index, floored or rounded to nearest.
      P("    xe_vi = int(floor(%s.%c%s));\n", Reg(Bits(x, 5, 6), Bits(x, 11, 1)).c_str(), "xyzw"[Bits(x, 30, 2)],
        Bits(y, 15, 1) ? " + 0.5" : "");
    } else if (vf_buffer < 0) {
      Fatal(nullptr, "GPU: a shader's mini vertex fetch before any full one");
    }
    uint32_t format = Bits(y, 16, 6);
    bool is_signed = Bits(y, 12, 1), normalized = !Bits(y, 13, 1), rf1 = Bits(y, 14, 1);
    int32_t offset = SBits(z, 8, 23);
    struct Field {
      uint32_t word, lo, bits;
    };
    static const Field k8888[] = {{0, 0, 8}, {0, 8, 8}, {0, 16, 8}, {0, 24, 8}};
    static const Field k2101010[] = {{0, 0, 10}, {0, 10, 10}, {0, 20, 10}, {0, 30, 2}};
    static const Field k101111[] = {{0, 0, 11}, {0, 11, 11}, {0, 22, 10}};
    static const Field k111110[] = {{0, 0, 10}, {0, 10, 11}, {0, 21, 11}};
    static const Field k1616[] = {{0, 0, 16}, {0, 16, 16}, {1, 0, 16}, {1, 16, 16}};
    static const Field k32[] = {{0, 0, 32}, {1, 0, 32}, {2, 0, 32}, {3, 0, 32}};
    const Field* fields = nullptr;
    uint32_t n = 0, words = 1;
    int kind = 0;  // 0 integer fields, 1 halves, 2 floats
    switch (format) {
      case 6: fields = k8888, n = 4; break;
      case 7: fields = k2101010, n = 4; break;
      case 16: fields = k101111, n = 3; break;
      case 17: fields = k111110, n = 3; break;
      case 25: fields = k1616, n = 2; break;
      case 26: fields = k1616, n = 4, words = 2; break;
      case 31: kind = 1, n = 2; break;
      case 32: kind = 1, n = 4, words = 2; break;
      case 33: fields = k32, n = 1; break;
      case 34: fields = k32, n = 2, words = 2; break;
      case 35: fields = k32, n = 4, words = 4; break;
      case 36: kind = 2, n = 1; break;
      case 37: kind = 2, n = 2, words = 2; break;
      case 38: kind = 2, n = 4, words = 4; break;
      case 57: kind = 2, n = 3, words = 3; break;
      default: Fatal(nullptr, "GPU: a shader vertex fetch of format %u", format);
    }
    for (uint32_t k = 0; k < words; k++)
      P("    uint w%u = xe_vw(%du, xe_vi * %d + %d);\n", k, vf_buffer, int(vf_stride), offset + int(k));
    ShaderVertexBuffer& b = info.vertex_buffers[vf_buffer];
    if (offset < 0) b.stride = 0;
    else b.extent = std::max(b.extent, uint32_t(offset) + words);
    std::string comp[4] = {"0.0", "0.0", "0.0", "1.0"};
    for (uint32_t c = 0; c < n; c++) {
      if (kind == 2) {
        comp[c] = F("uintBitsToFloat(w%u)", c);
      } else if (kind == 1) {
        comp[c] = F("unpackHalf2x16(w%u).%c", c / 2, "xy"[c & 1]);
      } else {
        const Field& f = fields[c];
        std::string u = f.bits == 32 ? F("w%u", f.word) : F("(w%u >> %uu & %uu)", f.word, f.lo, (1u << f.bits) - 1);
        double max = f.bits == 32 ? 4294967295.0 : double((1u << f.bits) - 1);
        if (!is_signed) {
          comp[c] = normalized ? F("float(%s) / %.1f", u.c_str(), max) : F("float(%s)", u.c_str());
        } else {
          std::string sx = f.bits == 32 ? F("float(int(w%u))", f.word)
                                        : F("float(int(w%u << %uu) >> %u)", f.word, 32 - f.lo - f.bits, 32 - f.bits);
          if (!normalized) comp[c] = sx;
          else if (rf1) comp[c] = F("(%s * 2.0 + 1.0) / %.1f", sx.c_str(), max);
          else comp[c] = F("max(%s / %.1f, -1.0)", sx.c_str(), (max - 1) / 2);
        }
      }
    }
    P("    vec4 f = vec4(%s, %s, %s, %s);\n", comp[0].c_str(), comp[1].c_str(), comp[2].c_str(), comp[3].c_str());
    if (int32_t e = SBits(y, 24, 6)) P("    f = ldexp(f, ivec4(%d));\n", e);
    StoreFetch(x, y);
  }

  uint32_t Texture(uint32_t x, uint32_t y, uint32_t z) {
    ShaderTexture t{Bits(x, 20, 5), Bits(z, 14, 2), Bits(y, 12, 2), Bits(y, 14, 2), Bits(y, 16, 2), Bits(y, 18, 3)};
    if (Bits(y, 24, 4) != 0xF) Fatal(nullptr, "GPU: a shader texture fetch that sets the volume filters");
    auto it = std::find(info.textures.begin(), info.textures.end(), t);
    uint32_t n = uint32_t(it - info.textures.begin());
    if (it == info.textures.end()) info.textures.push_back(t);
    return n;
  }

  void TextureFetch(uint32_t x, uint32_t y, uint32_t z) {
    uint32_t op = Bits(x, 0, 5);
    std::string src = Reg(Bits(x, 5, 6), Bits(x, 11, 1));
    char sw[4] = {"xyzw"[Bits(x, 26, 2)], "xyzw"[Bits(x, 28, 2)], "xyzw"[Bits(x, 30, 2)], 0};
    P("    vec3 tc = %s.%s;\n", src.c_str(), sw);
    switch (op) {
      case 1: break;
      case 17: {  // the computed level of detail
        if (!info.pixel) Fatal(nullptr, "GPU: a vertex shader asks for a texture's computed LOD");
        uint32_t n = Texture(x, y, z);
        if (Bits(z, 14, 2) == 2) P("    vec4 f = vec4(xe_t%u_qlod(tc));\n", n);
        else P("    vec4 f = vec4(textureQueryLod(xe_t%u, tc.%s).y);\n", n, Bits(z, 14, 2) == 3 ? "xyz" : "xy");
        StoreFetch(x, y);
        return;
      }
      case 18:
        if (!info.pixel) Fatal(nullptr, "GPU: a vertex shader asks for gradients");
        P("    vec4 f = vec4(dFdx(tc.xy), dFdy(tc.xy)).xzyw;\n");
        StoreFetch(x, y);
        return;
      case 24: P("    xe_lod = tc.x;\n"); return;
      case 25: P("    xe_gh = tc;\n"); return;
      case 26: P("    xe_gv = tc;\n"); return;
      default: Fatal(nullptr, "GPU: shader fetch operation %u is not implemented yet", op);
    }
    uint32_t n = Texture(x, y, z), dim = Bits(z, 14, 2);
    bool denorm = Bits(x, 25, 1), comp_lod = Bits(y, 28, 1), reg_lod = Bits(y, 29, 1), reg_grad = Bits(z, 0, 1);
    double bias = SBits(z, 2, 7) / 16.0;
    double off[3] = {SBits(z, 16, 5) / 2.0, SBits(z, 21, 5) / 2.0, SBits(z, 26, 5) / 2.0};
    const char* cs = dim == 2 ? "xyz" : "xy";
    const char* vec = dim == 2 ? "vec3" : "vec2";
    std::string coord;
    if (dim == 3) {
      if (denorm || off[0] || off[1] || off[2])
        Fatal(nullptr, "GPU: a cube texture fetch with offsets or unnormalized coordinates");
      coord = "xe_cubedir(tc)";
    } else {
      if (dim == 2) P("    vec3 ts = xe_t%u_size();\n", n);
      else P("    vec2 ts = vec2(textureSize(xe_t%u, 0));\n", n);
      coord = F("tc.%s", cs);
      if (denorm) coord = "(" + coord + " / ts)";
      if (off[0] || off[1] || (dim == 2 && off[2]))
        coord = dim == 2 ? F("(%s + vec3(%.1f, %.1f, %.1f) / ts)", coord.c_str(), off[0], off[1], off[2])
                         : F("(%s + vec2(%.1f, %.1f) / ts)", coord.c_str(), off[0], dim ? off[1] : 0.0);
    }
    std::string lod = reg_lod ? F("xe_lod + %.4f", bias) : F("%.4f", bias);
    // A 3D or stacked texture (dimension 2) through the renderer's
    // functions, which know which it is.
    if (reg_grad) {
      const char* g = dim >= 2 ? "" : ".xy";
      if (dim == 2) P("    vec4 f = xe_t%u_grad(%s, xe_gh, xe_gv);\n", n, coord.c_str());
      else P("    vec4 f = textureGrad(xe_t%u, %s, xe_gh%s, xe_gv%s);\n", n, coord.c_str(), g, g);
    } else if (!comp_lod || !info.pixel) {
      if (dim == 2) P("    vec4 f = xe_t%u_lod(%s, %s);\n", n, coord.c_str(), lod.c_str());
      else P("    vec4 f = textureLod(xe_t%u, %s, %s);\n", n, coord.c_str(), lod.c_str());
    } else {
      if (dim == 2) P("    vec4 f = xe_t%u_bias(%s, %s);\n", n, coord.c_str(), lod.c_str());
      else P("    vec4 f = texture(xe_t%u, %s, %s);\n", n, coord.c_str(), lod.c_str());
    }
    P("    f = xe_sample(f, xe_tp[%u]);\n", n);
    StoreFetch(x, y);
  }

  void Fetch(uint32_t x, uint32_t y, uint32_t z) {
    if (Bits(y, 31, 1)) P("  if (xe_p0 == %s) {\n", Bits(z, 31, 1) ? "true" : "false");
    else P("  {\n");
    if (Bits(x, 0, 5) == 0) VertexFetch(x, y, z);
    else TextureFetch(x, y, z);
    P("  }\n");
  }

  // ---- control flow ---------------------------------------------------------
  // An exec's condition, or "" for none.
  static std::string ExecCondition(const Cf& cf) {
    const char* want = Bits(cf.d1, 10, 1) ? "true" : "false";
    switch (cf.op()) {
      case kCfCondExec: case kCfCondExecEnd: case kCfCondExecPredClean: case kCfCondExecPredCleanEnd:
        return Bool(Bits(cf.d1, 2, 8)) + " == " + want;
      case kCfCondExecPred: case kCfCondExecPredEnd: return std::string("xe_p0 == ") + want;
      default: return "";
    }
  }

  // An exec's instructions; `end` is what ends the shader.
  void Exec(const Cf& cf, const char* end) {
    uint32_t address = Bits(cf.d0, 0, 12), n = Bits(cf.d0, 12, 3), sequence = Bits(cf.d0, 16, 12);
    std::string cond = ExecCondition(cf);
    if (!cond.empty()) P(" if (%s) {\n", cond.c_str());
    for (uint32_t k = 0; k < n; k++) {
      uint32_t at = 3 * (address + k);
      if (at + 3 > count) Fatal(nullptr, "GPU: a shader's instruction %u is past its end", address + k);
      if (sequence >> (2 * k) & 1) Fetch(w[at], w[at + 1], w[at + 2]);
      else Alu(w[at], w[at + 1], w[at + 2]);
    }
    if (EndsShader(cf.op())) P("  %s\n", end);
    if (!cond.empty()) P(" }\n");
  }

  // A jump's or call's condition.
  static std::string JumpCondition(const Cf& cf) {
    if (Bits(cf.d0, 13, 1)) return "true";
    const char* want = Bits(cf.d1, 10, 1) ? "true" : "false";
    if (Bits(cf.d0, 14, 1)) return std::string("xe_p0 == ") + want;
    return Bool(Bits(cf.d1, 2, 8)) + " == " + want;
  }

  void Run() {
    // Control flow instructions come in pairs of 48 bits in three words,
    // up to the first instruction an exec names.
    std::vector<Cf> cfs;
    uint32_t bound = count / 3;
    for (uint32_t i = 0; i < bound; i++) {
      uint32_t a = w[3 * i], b = w[3 * i + 1], c = w[3 * i + 2];
      Cf pair[2] = {{a, b & 0xFFFF}, {b >> 16 | c << 16, c >> 16}};
      for (const Cf& cf : pair) {
        if (IsExec(cf.op()) && Bits(cf.d0, 12, 3)) bound = std::min(bound, Bits(cf.d0, 0, 12));
        cfs.push_back(cf);
      }
    }
    bool flat = true;
    for (const Cf& cf : cfs) flat &= cf.op() < kCfLoopStart || cf.op() > kCfCondJmp;
    std::string body;
    std::swap(out, body);
    if (flat) {
      for (const Cf& cf : cfs)
        if (IsExec(cf.op())) Exec(cf, "return;");
    } else {
      // Loops, jumps and calls: a program counter over the control flow
      // instructions.
      P(" int pc = 0;\n for (;;) {\n switch (pc) {\n");
      for (uint32_t i = 0; i < cfs.size(); i++) {
        const Cf& cf = cfs[i];
        uint32_t target = Bits(cf.d0, 0, 13), id = Bits(cf.d0, 16, 5);
        P(" case %u: {\n", i);
        switch (cf.op()) {
          case kCfLoopStart:
            // The loop constant: count, start and step, a byte each.
            P("  uint lc = xe_loop[%u][%u];\n", id >> 2, id & 3);
            P("  if ((lc & 0xFFu) == 0u) { pc = %u; break; }\n", target);
            P("  xe_lcount[xe_ldepth & 3] = 0;\n");
            if (!Bits(cf.d0, 13, 1)) P("  xe_al = int(lc >> 8 & 0xFFu);\n");
            P("  xe_lal[xe_ldepth & 3] = xe_al;\n  xe_ldepth++;\n");
            break;
          case kCfLoopEnd:
            P("  uint lc = xe_loop[%u][%u];\n  int d = (xe_ldepth - 1) & 3;\n", id >> 2, id & 3);
            P("  xe_lcount[d]++;\n");
            if (Bits(cf.d0, 21, 1))
              P("  if (uint(xe_lcount[d]) < (lc & 0xFFu) && xe_p0 != %s) {\n", Bits(cf.d1, 10, 1) ? "true" : "false");
            else
              P("  if (uint(xe_lcount[d]) < (lc & 0xFFu)) {\n");
            P("   xe_lal[d] += int(lc << 8) >> 24;\n   xe_al = xe_lal[d];\n   pc = %u;\n   break;\n  }\n", target);
            P("  xe_ldepth = d;\n  xe_al = d > 0 ? xe_lal[d - 1] : 0;\n");
            break;
          case kCfCondCall:
            P("  if (%s) { xe_ret[xe_rdepth++ & 3] = %u; pc = %u; break; }\n", JumpCondition(cf).c_str(), i + 1, target);
            break;
          case kCfReturn:
            P("  if (xe_rdepth > 0) { pc = xe_ret[--xe_rdepth & 3]; break; }\n");
            break;
          case kCfCondJmp:
            P("  if (%s) { pc = %u; break; }\n", JumpCondition(cf).c_str(), target);
            break;
          default:
            if (IsExec(cf.op())) Exec(cf, "return;");
        }
        P("  pc = %u;\n  break;\n }\n", i + 1);
      }
      P(" default: return;\n }\n }\n");
    }
    std::swap(out, body);
    // The textures, what fetches read through, then the function.
    // (3D or stacked ones the renderer declares, with their functions.)
    static const char* const kSampler[4] = {"sampler2D", "sampler2D", nullptr, "samplerCube"};
    for (uint32_t n = 0; n < info.textures.size(); n++)
      if (info.textures[n].dimension != 2)
        P("layout(R_TEXTURE(%u)) uniform %s xe_t%u;\n", n, kSampler[info.textures[n].dimension], n);
    if (!info.vertex_buffers.empty())
      P("int xe_vi = 0;\n"
        "uint xe_vw(uint b, int i) {\n"
        "  uint k = uint(i) - xe_vf[b].y;\n"
        "  if (i < 0 || k >= xe_vf[b].z) return 0u;\n"
        "  return xe_swap(r_ld(b, k), xe_vf[b].x);\n"
        "}\n");
    out += "void xe_shader() {\n" + body + "}\n";
    if (r0_written)
      for (ShaderVertexBuffer& b : info.vertex_buffers) b.stride = 0;
    info.constants = const_relative ? 256 : max_const;
    info.glsl = std::move(out);
  }
};

}  // namespace

const char kShaderCommon[] = R"(
const float XE_MAX = 3.402823466e+38;
vec4 xe_r[64];
float xe_ps = 0.0, xe_lod = 0.0;
bool xe_p0 = false;
int xe_a0 = 0, xe_al = 0, xe_ldepth = 0, xe_rdepth = 0;
int xe_lcount[4], xe_lal[4], xe_ret[4];
vec3 xe_gh = vec3(0.0), xe_gv = vec3(0.0);
// Direct3D 9's multiply: zero times anything is +0.
vec4 xe_mul(vec4 a, vec4 b) {
  vec4 z = vec4(equal(a, vec4(0.0))) + vec4(equal(b, vec4(0.0)));
  return mix(a * b, vec4(0.0), greaterThan(z, vec4(0.0)));
}
float xe_muls(float a, float b) { return a == 0.0 || b == 0.0 ? 0.0 : a * b; }
vec4 xe_max(vec4 a, vec4 b) { return mix(b, a, greaterThanEqual(a, b)); }
vec4 xe_min(vec4 a, vec4 b) { return mix(b, a, lessThan(a, b)); }
vec4 xe_max4(vec4 a) {
  if (a.x > a.y && a.x > a.z && a.x > a.w) return vec4(a.x);
  if (a.y > a.z && a.y > a.w) return vec4(a.y);
  return vec4(a.z > a.w ? a.z : a.w);
}
float xe_clampinf(float t) { return isinf(t) ? (t < 0.0 ? -XE_MAX : XE_MAX) : t; }
float xe_zeroinf(float t) { return isinf(t) ? (t < 0.0 ? -0.0 : 0.0) : t; }
// The cube instruction, its operand the direction as zzxy: T, S, twice
// the major axis and the face.
vec4 xe_cube(vec4 a) {
  float x = a.z, y = a.w, z = a.x;
  vec3 m = abs(vec3(x, y, z));
  if (m.z >= m.x && m.z >= m.y) return vec4(-y, z < 0.0 ? -x : x, 2.0 * z, z < 0.0 ? 5.0 : 4.0);
  if (m.y >= m.x) return vec4(y < 0.0 ? -z : z, x, 2.0 * y, y < 0.0 ? 3.0 : 2.0);
  return vec4(-y, x < 0.0 ? z : -z, 2.0 * x, x < 0.0 ? 1.0 : 0.0);
}
// A cube fetch's coordinates (S and T in [1, 2], the face) as a direction.
vec3 xe_cubedir(vec3 c) {
  vec2 st = c.xy * 2.0 - 3.0;
  int face = int(c.z);
  if (face == 0) return vec3(1.0, -st.y, -st.x);
  if (face == 1) return vec3(-1.0, -st.y, st.x);
  if (face == 2) return vec3(st.x, 1.0, st.y);
  if (face == 3) return vec3(st.x, -1.0, -st.y);
  if (face == 4) return vec3(st.x, -st.y, 1.0);
  return vec3(-st.x, -st.y, -1.0);
}
uint xe_swap(uint v, uint e) {
  if (e == 1u) return (v << 8 & 0xFF00FF00u) | (v >> 8 & 0x00FF00FFu);
  if (e == 2u) return v >> 24 | (v >> 8 & 0xFF00u) | (v << 8 & 0xFF0000u) | v << 24;
  if (e == 3u) return v >> 16 | v << 16;
  return v;
}
// A texture sample as its fetch constant has it: p.x the swizzle (three
// bits a component: a stored component, 4 for zero, 5 for one), p.y each
// stored component's sign (2: biased, 2x - 1), p.z the exponent bias.
// The Xenos's piecewise-linear gamma to linear (Xenia's PWLGammaToLinear,
// with its truncation).
float xe_degamma(float g) {
  g = isnan(g) ? 0.0 : clamp(g, 0.0, 1.0);
  float scale = 1.0 / 1024.0, offset = 0.0;
  if (g >= 192.0 / 255.0) {
    scale = 8.0 / 1024.0;
    offset = -1024.0;
  } else if (g >= 96.0 / 255.0) {
    scale = 4.0 / 1024.0;
    offset = -256.0;
  } else if (g >= 64.0 / 255.0) {
    scale = 2.0 / 1024.0;
    offset = -64.0;
  }
  float l = g * (255.0 * 1024.0 * scale) + offset;
  l += trunc(l * scale);
  return l / 1023.0;
}
vec4 xe_sample(vec4 t, ivec4 p) {
  for (int i = 0; i < 4; i++) {
    int s = p.y >> 2 * i & 3;
    if (s == 2) t[i] = t[i] * 2.0 - 1.0;
    else if (s == 3) t[i] = xe_degamma(t[i]);
  }
  vec4 o;
  for (int i = 0; i < 4; i++) {
    int s = p.x >> 3 * i & 7;
    o[i] = s < 4 ? t[s] : s == 5 ? 1.0 : 0.0;
  }
  return ldexp(o, ivec4(p.z));
}
)";

ShaderInfo TranslateShader(const uint32_t* words, uint32_t count, bool pixel) {
  Translator t{words, count};
  t.info.pixel = pixel;
  t.Run();
  return std::move(t.info);
}

}  // namespace recomp::xbox360
