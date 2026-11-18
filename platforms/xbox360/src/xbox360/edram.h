#pragma once
// The Xenos's EDRAM and its resolves (docs/xenos.md, "EDRAM and
// resolves"): the 10 MiB the render back end draws into, kept on the host
// GPU as a buffer in the console's layout, so a target that one format
// wrote reads back as another the way it does there. A resolve copies a
// rectangle of a colour or depth target to a tiled texture in memory and
// clears the target, as compute work on the device.

#include <cstdint>

#include "runtime/gfx/device.h"

namespace recomp::xbox360 {

// 2048 tiles of 80x16 32-bit samples.
constexpr uint32_t kEdramTiles = 2048, kEdramTileWords = 80 * 16,
                   kEdramBytes = kEdramTiles * kEdramTileWords * 4;

// At power-on, after StartDisplay: the EDRAM on the display's device.
void StartEdram();

// A draw packet with RB_MODECONTROL in copy mode: the resolve the copy
// registers describe, from the command processor. Its results reach
// memory later, by LandResolves at the latest.
void Resolve();

// From the command processor, before anything the guest can see of the
// GPU's progress (memory it writes or reads, interrupts, the read
// pointer, a swap): the results of the resolves before it, in memory.
void LandResolves();
// Before the GPU's own reads of memory on the CPU (packets, indices,
// vertices, microcode, constants, textures): the resolves' results in
// memory if one still on the device wrote into physical [lo, hi) (all of
// them: they land in order). LandResolvesLocked with the display device's
// lock held.
bool ResolvePending(uint32_t lo, uint32_t hi);
void LandResolves(uint32_t lo, uint32_t hi);
void LandResolvesLocked(uint32_t lo, uint32_t hi);
// How many times resolves were landed (all, and by LandResolves(lo, hi)),
// for the test harness.
uint64_t ResolveLandings();
uint64_t ResolveOverlapLandings();

// The EDRAM's buffer on the display's device (its lock held), and the
// GLSL for its layout and formats: EdramWord, a sample's word; Unpack and
// PackTarget, a colour target's words; the depth formats' conversions.
gfx::BufferId EdramBuffer();
extern const char kEdramGlsl[];
// A vertex stage drawing one triangle over the whole target.
extern const char kFullTargetVertex[];

// The EDRAM's float and depth formats' values, as kEdramGlsl's
// unpackHalf2x16, F7e3, Unorm24 and F20e4 give them: a half, 7e3 (bits
// 0-9), 24-bit unorm and 20e4 (bits 0-23).
float Half(uint16_t h);
float F7e3(uint32_t f);
float Unorm24(uint32_t d);
float F20e4(uint32_t f);

// Byte offset of block (x, y) in a 2D tiled surface `pitch` blocks wide
// (a multiple of 32), blocks of 1 << bpb_log2 bytes; negative coordinates
// give the offsets before the surface that resolves use.
int32_t Tiled2D(int32_t x, int32_t y, uint32_t pitch, uint32_t bpb_log2);

}  // namespace recomp::xbox360
