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
// The EDRAM was written (a resolve's clears): the host targets take what
// it holds before the next draw into them.
void InvalidateTargets();

}  // namespace recomp::xbox360
