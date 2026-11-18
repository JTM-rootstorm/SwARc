#pragma once
// The Xenos's draws on the host GPU (docs/xenos.md, "Drawing"): the
// command processor's shader loads and draw packets, as programs
// translated from the microcode (shader.h) drawing into host render
// targets, which hold what the EDRAM (edram.h) holds between a resolve
// and the draws after it. From the command processor's thread.

#include <cstdint>

namespace recomp::xbox360 {

// IM_LOAD and IM_LOAD_IMMEDIATE: the microcode later draws run, as the
// GPU reads its words.
void LoadShader(bool pixel, const uint32_t* words, uint32_t count);

// A draw packet outside copy mode: VGT_DRAW_INITIATOR and, for indices in
// memory, their address and the word with their count and endian swap,
// or the packet's indices (`immediate`, else null).
void Draw(uint32_t initiator, uint32_t index_base, uint32_t index_size, const uint32_t* immediate);

// What the host targets hold that the EDRAM does not yet, into the EDRAM:
// before anything else reads it (a resolve).
void StoreTargets();
// A resolve's clear of pixels [x0, x1) x [y0, y1) of the colour (depth:
// depth) target at base tile `base` of a surface `width` pixels and
// `pitch` tiles wide with MSAA mode `msaa`, of format `format`, to the
// EDRAM words `lo` and (64bpp) `hi`, from the command processor with the
// display device's lock held: a host target of that surface whose rows
// in use and width it covers takes the clear itself; others whose tiles
// it touches load again before the next draw into them. The stencil the
// EDRAM's tiles hold where known (which spares a depth target's load its
// draws of each stencil bit) follows the clear.
void ClearTargets(bool depth, uint32_t base, uint32_t width, uint32_t pitch, uint32_t msaa, uint32_t format,
                  int32_t x0, int32_t y0, int32_t x1, int32_t y1, uint32_t lo, uint32_t hi);
// How many times draws loaded a host target from the EDRAM, for the test
// harness.
uint64_t TargetLoads();
// How many draws of one stencil bit each loads (stores: stores) of depth
// targets made, for the test harness.
uint64_t StencilBitDraws(bool stores);

}  // namespace recomp::xbox360
