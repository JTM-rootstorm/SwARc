# Xenos: behaviour the runtime reimplements

What the GPU and the kernel's display driver (the Vd* exports) do, as far
as the runtime (ARCHITECTURE.md, "The GPU") depends on it, and where each
fact comes from. The kernel's driver code is not available; the evidence
is titles' and xam's code, then Xenia's command processor and video
exports (BSD-3), then AMD's documentation of the R6xx command processor,
whose PM4 packets the Xenos shares. "Console check" marks what only a
console can settle.

## Registers
- The register file is at 0x7FC80000, register r at + 4r: a title writes
  CP_RB_WPTR (0x1C5) there and reads the interrupt status (0x1951), the
  scanline counter (0x194C) and the viewport size (0x1961); it also
  writes the primary surface address (0x1844) directly.
- Read-only values titles read: RB_EDRAM_TIMING 0x08100748, RB_BC_CONTROL
  0x0000200E, RB_HSIO_INTERFACE_ALIGNER 0x00BBBBBB (Xenia's), the
  interrupt status 1 (a vertical blank), the viewport 1280x720. They are
  set at power-on; a title that writes one changes it (Xenia ignores such
  writes). Console check.
- VGT_MAX_VTX_INDX is 0xFFFFFF at power-on (a choice, so that a title
  that never writes it is not clamped to vertex 0; console check).
- Writes the command processor makes (type 0 and 1 packets, constants)
  have the side effects Xenia gives them: SCRATCH_REG0-7 are written back
  to SCRATCH_ADDR + 4n when SCRATCH_UMSK enables them; a write to
  COHER_STATUS_HOST sets its bit 31, and a WAIT_REG_MEM on it clears the
  register (caches here are always coherent). The request lands no
  resolve ("EDRAM and resolves"; a choice: it is for the GPU's own later
  reads of the memory a resolve wrote, which here take the resolve's
  results from the device or land it first; a title that polled the
  register from the CPU and then read the texels without a fence would
  see them late; console check).

## The ring buffer
- VdInitializeRingBuffer(physical address, log2): 2^(log2 + 3) bytes,
  emptied, read pointer 0. VdEnableRingBufferRPtrWriteBack(address,
  block log2): the read pointer (in words) is written there, big-endian,
  after each batch (not every block). Xenia's.
- The title advances CP_RB_WPTR by plain stores, which nothing signals:
  the command processor polls the register (yielding, then every 50 us
  when idle).
- A title writes the ring with pre-incrementing stores: its write
  position is the last word written, and VdSwap's buffer starts one word
  after it.

## Packets
- Types: 0 writes count registers from a base (bit 15: all to one
  register), 1 two registers, 2 is a no-op, 3 an opcode with count
  words. A zero word and 0x0BADF00D are skipped, as Xenia does. A type-3
  packet with bit 0 runs only if BIN_SELECT & BIN_MASK (both
  0xFFFFFFFF at first).
- The GPU accesses memory little-endian, after the swap the address's low
  two bits name: none, bytes in halfwords, bytes in words (what a
  big-endian CPU reads as the value), halfwords in words.
- Handled as Xenia does: NOP, ME_INIT (no microcode runs here),
  INDIRECT_BUFFER(_PFD) (at once), WAIT_REG_MEM, REG_RMW, REG_TO_MEM,
  MEM_WRITE, COND_WRITE, EVENT_WRITE (without an address), EVENT_WRITE_SHD
  (a fence; bit 31 of the initiator writes the counter of vertical blanks
  and swaps instead, console check; addresses above physical memory go to
  the writeback window at 0x7F000000), EVENT_WRITE_EXT (the largest
  extents, since draws are not measured), EVENT_WRITE_ZPD (an occlusion
  query's sample counts at RB_SAMPLE_COUNT_ADDR, eight little-endian
  words: total, z fail, z pass, stencil fail, two each; zero, and at a
  query's end, which D3D marks with 0xFFFFFEED in a z pass or z fail
  word, 1000 samples passed, Xenia's count, since draws are not
  measured), INTERRUPT (the callback with source 1 for each processor in
  the mask), SET_CONSTANT, SET_CONSTANT2,
  SET_SHADER_CONSTANTS, LOAD_ALU_CONSTANT, VIZ_QUERY, the bin masks,
  CONTEXT_UPDATE, WAIT_IB_PFD_COMPLETE, WAIT_FOR_IDLE, INVALIDATE_STATE.
- Shader loads, Xenia's reading: IM_LOAD's first word is the
  microcode's physical address with the shader type (0 vertex, 1 pixel)
  in its low two bits, IM_LOAD_IMMEDIATE's the type alone; the second
  word is the start in instruction memory (bits 16-31; only 0 is
  implemented) and the size in words (bits 0-15); IM_LOAD_IMMEDIATE's
  words follow in the packet. The last loaded shader of each type is
  the one draws run (below).
- Draws: DRAW_INDX starts with a visibility query word, then, as
  DRAW_INDX_2 does, VGT_DRAW_INITIATOR: the primitive type (bits 0-5),
  the index source (bits 6-7: 0 indices in memory, 1 in the packet, 2
  counted from 0), 32-bit indices (bit 11) and the count (bits 16-31).
  Indices in memory come with two more words: their physical address
  and VGT_DMA_SIZE (their number in bits 0-23, the endian swap in bits
  30-31). Xenia's reading. Indices in the packet (which Xenia does not
  implement) follow the initiator, 32-bit ones a word each, 16-bit ones
  two a word, the low half first, unswapped: the R6xx's
  DRAW_INDEX_IMMD packing, console check.
- A draw while RB_MODECONTROL's EDRAM mode is 6 (copy) is a resolve
  (below); in modes 4 (colour and depth) and 5 (depth only) it draws
  ("Drawing"); in mode 0 it draws nothing (Xenia skips it); other modes
  are fatal.
- Any other opcode is fatal and names it.

## EDRAM and resolves
What follows is Xenia's (its render target cache and resolve shaders);
none of it has been checked on a console yet.
- The EDRAM is 2048 tiles of 80x16 32-bit samples (10 MiB); tile
  numbers wrap at 2048. A target's base (RB_COLOR_INFO, RB_COLOR1-3_INFO,
  RB_DEPTH_INFO) is a tile; its rows of tiles are RB_SURFACE_INFO's pitch
  in samples over 80, twice that for 64-bit formats, whose tiles hold
  40x16 samples (Xenia says the real 64-bit layout is not known). With
  2x MSAA a pixel's samples 0 and 1 are one above the other; with 4x,
  samples 2 and 3 are right of 0 and 1. A depth tile's two 40-sample
  halves are swapped. The runtime keeps the EDRAM in this layout in a
  buffer on the host GPU, so a target written in one format reads in
  another as it would there.
- A resolve's rectangle is the three points of vertex fetch 0 (32-bit
  floats, its endian swap): the corners plus half a pixel when
  PA_SU_VTX_CNTL's pixel centres are D3D's, in 16.8 fixed point rounded
  to nearest, then (v + 127) >> 8; moved by the window offset when
  PA_SU_SC_MODE_CNTL enables it; clipped to the window scissor (moved
  by the offset unless its TL bit 31 says not to) within the screen
  scissor; widened to whole blocks of 8 pixels and clipped to the
  surface's pitch.
- RB_COPY_CONTROL: the source (colour target 0-3, or 4 and up for depth),
  the sample (0-3, or 4-6: samples 0-1, 2-3 or 0-3 averaged; narrowed to
  what the surface has, depth never averaged), clears of the colour
  (source 0-3 only) and depth targets, and the copy command (0 raw, 1
  convert; 2 and 3 are fatal).
- The copy goes first: each pixel of the rectangle to the texel with the
  same coordinates of a 2D tiled texture at RB_COPY_DEST_BASE (which D3D
  biases so that this holds, even before the texture), its pitch
  RB_COPY_DEST_PITCH's aligned to 32, in RB_COPY_DEST_INFO's format and
  endian swap. Where the target's format and the texture's hold the same
  bits (8888 as 8_8_8_8, 2_10_10_10, the float formats as themselves) and
  one sample is copied without an exponent bias, the words go as they
  are, with copy_dest_swap trading red and blue in place; otherwise the
  samples are unpacked (7e3 floats, 16_16 fixed point over [-32, 32] with
  missing channels 0), averaged, scaled by 2^exp_bias, red and blue
  swapped, and packed (unorm rounded to nearest, 16_16 as unorm).
  Depth goes as its words. 3D destinations, the gamma format's
  conversion and formats outside these are fatal, not implemented yet.
- Then the clears, of every sample in the rectangle: the colour target
  to RB_COLOR_CLEAR as the EDRAM holds it (red in the low byte of 8888;
  a 64-bit target's low word, RB_COLOR_CLEAR_LO the high one, an order
  Xenia is not sure of), the depth target to RB_DEPTH_CLEAR.
- Draws go into host render targets ("Render targets"): a resolve first
  stores those drawn into to the EDRAM, colour and depth (with
  stencil), so it reads what was drawn; after its clears the targets
  load again.
- The tiled address of texel (x, y) (Tiled2D in edram.cpp): macro tiles
  of 32x32 texels in rows of pitch / 32, micro tiles of 8x2, with bank
  ((y >> 4) & 1) and pipe (((x >> 3) & 3) ^ ((y >> 3) & 1) << 1) bits
  interleaved into the offset.
- The resolve runs as compute on the host GPU (the device exists from
  power-on), which reads the destination's range of memory (from the
  first texel's 32x32 block to the end of the last's) and writes all of
  it back, the bytes outside the rectangle as they were. Its texels
  reach memory later, before anything the guest sees of the GPU's
  progress that comes after it: a packet that writes or reads memory
  for the guest (a fence, MEM_WRITE, REG_TO_MEM, WAIT_REG_MEM on memory,
  scratch register write-back, occlusion query counts, the extents of
  EVENT_WRITE_EXT), an interrupt,
  a swap, the read pointer; and before the GPU's own reads of memory a
  resolve still on the device wrote: an indirect buffer's packets,
  IM_LOAD's microcode, LOAD_ALU_CONSTANT's constants, a draw's indices
  and vertices, a texture decoded or hashed on the CPU (then all of
  them land, in order). A resolve whose range overlaps one still on the
  device waits for that one's texels first. On the console the texels
  are written as the copy runs; a title that reads them without a fence
  or writes into the range while the resolve runs would see the
  difference (a choice, so that the command processor does not wait for
  the host GPU after each resolve).
- Textures the cache holds in the range take the copy's texels on the
  device where they can, decoded as their format decodes the copy's
  words; the others decode again before they are next sampled
  ("Textures").
- A clear that covers all the rows a host target uses, and the
  surface's width, of the target of the surface it clears (base, pitch,
  MSAA, format) clears that host target to what a load of the cleared
  words gives, and leaves it valid; other targets whose tiles the
  cleared rows touch load again; the others are not touched
  ("Render targets").

## Shaders
The microcode's encoding and its instructions' meaning are Xenia's (its
gpu/ucode.h, which it takes from the XNA assembler, AMD's R400
sequencer documents and freedreno); nothing here has been checked on a
console. shader.cpp translates a shader to a GLSL function over globals
the first time a draw runs it (by its words; equal microcode is
translated once).
- Control flow: instructions of 48 bits, two in three words, from the
  start up to the first instruction an exec names. Execs run up to six
  ALU or fetch instructions (three words each, a fetch where the exec's
  sequence bits say), with a bool constant or the predicate as their
  condition; an exec_end ends the shader. Loops (the loop constant's
  count, start and step, a byte each), conditional jumps, calls and
  returns become a program counter over the control flow instructions;
  a shader without them runs its execs in order.
- ALU instructions: a vector and a scalar operation in one, each with
  its own write mask, on registers or float constants (relative to a0
  or the loop index where the instruction says), with Direct3D 9's
  multiply (zero times anything is +0), the predicate and the address
  register a0 as state. An export writes both results to one export
  register: a vertex shader's 0-15 are the interpolators, 62 the
  position, 63 the point size; a pixel shader's 0-3 the colour targets,
  61 depth. Memory export is fatal, not implemented yet.
- Vertex fetches read a vertex fetch constant's buffer at word (index *
  stride + offset), its words swapped as the constant says, in the
  formats 8_8_8_8, 2_10_10_10, 10_11_11, 11_11_10, 16_16(_16_16)
  (integer and float), 32(_32(_32_32)) (integer and float) and
  32_32_32_FLOAT, signed or not, normalized or not, with the
  instruction's exponent adjust; a mini fetch takes the full fetch's
  before it address. Other formats are fatal.
- Texture fetches sample a texture fetch constant's texture with the
  instruction's filters, offsets, LOD bias and gradients; a sample goes
  through the constant's swizzle, signs (biased and gamma ones applied
  after filtering) and exponent bias ("Textures"). A fetch of dimension 2
  samples a 3D texture or a stacked 2D one, which a draw tells apart from
  the constant: the shader calls functions the renderer declares for the
  one it binds (shader.h).
- A fetch that reads by r0.x (the vertex index) in a shader that never
  writes r0 records its buffer's stride and how many words of a vertex
  it reads, so a draw gives the GPU only the range of the buffer its
  vertices use; fetches outside that range read 0, as fetches outside
  the constant's size do (a choice: the console reads memory there).

## Drawing
What a draw does on the host GPU (render.cpp). The vertex position's
path, the rectangle list's fourth vertex, the render state's encodings,
the formats' conversions and the draw packets are Xenia's (BSD-3:
draw_util.cc, xenos.cc, its shader translators, pipeline caches, render
target caches and primitive processor); none of it has been checked on
a console yet.
- The program: the vertex and pixel shaders' functions with a main
  each. The vertex stage runs the vertex shader with r0.x the guest
  vertex index and everything else zeroed; the pixel stage gets the 16
  interpolators in r0-r15 and writes the colours the shader exports
  (and its depth, when it exports that). Each stage reads its float
  constants (the register file's 0x4000 + 4i, a pixel shader's from
  constant 256) from a buffer of their words, and from its uniform block
  the bool (0x4900) and loop (0x4908) constants, the vertex buffers'
  swaps and ranges (fetch constant n is 0x4800 + 2n: type 3 and the
  physical address, the endian swap and the size in words) and the
  system values below. The constants are not in the uniform block: the
  host device's Vulkan backend binds 4 KiB of one, which 256 constants
  fill, and reads past that as zeros. In EDRAM mode 5 (depth
  only) the pixel shader does not run, as Xenia draws that mode. A
  multipass vertex shader (SQ_PROGRAM_CNTL's vs_export_mode 7) and a
  surface of pitch 0 draw nothing (Xenia rasterizes nothing then).
- SQ_PROGRAM_CNTL: vs_export_mode 0, 2, 4 and 5 (position and
  interpolators, with point size or vertex kill) draw; the edge flag
  modes (3, 6) and mode 1 are fatal, as are sequencer-generated indices
  (gen_index_vtx, bit 31): not implemented (nor in Xenia). vs_num_reg,
  ps_num_reg and ps_export_mode are not used (shaders here have all 64
  registers; the pixel shader's exports say whether it writes depth);
  Xenia does not use them either. Console check.
- The guest index: the index buffer's (16-bit with no swap or bytes
  swapped in halfwords, 32-bit with any of the four swaps), the
  packet's, or counted from 0, its low 24 bits; plus VGT_INDX_OFFSET in
  24 bits, clamped to VGT_MIN_VTX_INDX-VGT_MAX_VTX_INDX (0x2101, 0x2100),
  as Xenia's Direct3D 12 path does (its Vulkan one does not clamp); the
  maximum is 0xFFFFFF at power-on ("Registers").
- The position: W is 1/W unless PA_CL_VTE_CNTL's vtx_w0_fmt (bit 10)
  says it is W; XY and Z are multiplied by W where vtx_xy_fmt (bit 8)
  and vtx_z_fmt (bit 9) say they were divided by it; then the viewport
  scale and offset (PA_CL_VPORT_*, each where its PA_CL_VTE_CNTL bit
  enables it, else 1 and 0), the window offset (PA_SC_WINDOW_OFFSET,
  where PA_SU_SC_MODE_CNTL bit 16 enables it) and half a pixel for
  Direct3D's pixel centres (PA_SU_VTX_CNTL bit 0 clear) take X and Y to
  window pixels, which map onto the whole host target. This is Xenia's
  mapping for draws without clipping; draws with clipping go the same
  way in X and Y (a choice: the guest viewport is not a host viewport,
  so host clipping is to the target; console check).
- Z, as Xenia's GetHostViewportInfo takes it: with PA_CL_CLIP_CNTL's
  clip_disable (bit 16) the viewport's Z scale and offset in a 0-1 host
  depth range, not clipped (host depth clipping off); with clipping,
  the clip-space Z, 0..W, or -W..W where dx_clip_space_def (bit 19) is
  clear (made 0..W by halving and adding W/2), clipped, then the host
  viewport's depth range is the viewport's (offset to offset + scale;
  for -W..W, offset - scale to offset + scale) within 0-1, all of 0-1
  when the pixel shader writes depth, and a reversed range forward with
  Z flipped.
- Primitive types (xenos.h's numbers): point lists (1), line lists
  (2), line strips (3), triangle lists (4), triangle fans (5), triangle
  strips (6), rectangle lists (8), line loops (12) and quad lists (13);
  others are fatal, not implemented yet. The host draws lists: lines,
  strips, loops, fans and quads become a host list of the guest's
  indices on the CPU, in the orders Xenia's primitive processor makes
  (a strip's odd triangles as v1 v3 v2, the host's own strip order; a
  fan's triangles as (v[i-1], v[i], v0); a quad as (0 1 2), (0 2 3); a
  loop's last vertex back to its first).
- Strips and fans in memory or the packet restart where an index is
  VGT_MULTI_PRIM_IB_RESET_INDX (0x2103) and PA_SU_SC_MODE_CNTL's
  multi_prim_ib_ena (bit 21) enables it: compared with all of a 16-bit
  index (and only when the reset index fits 16 bits), the low 24 bits of
  a 32-bit one; lists ignore it (Xenia's rules). A line loop with it is
  fatal, not implemented yet.
- Rectangle lists draw each rectangle as two triangles: its three
  vertices (c, a, b, with c the vertex opposite the longest edge in
  XY/W) and b, a, v3, the fourth completing the parallelogram, v3 =
  a + b - c, in the position and every interpolator; the two are wound
  alike.
- Point lists draw each point as a sprite of two triangles around the
  vertex (corners top left, bottom left, top right, bottom right; Xenia's
  order): the diameter in window pixels (samples under MSAA) is the
  vertex shader's point size (export 63's X) where it writes one,
  clamped as a signed integer between PA_SU_POINT_MINMAX's (0x2281)
  half sizes doubled (min bits 0-15, max 16-31), else PA_SU_POINT_SIZE's
  (0x2280) half width (bits 16-31) and height (0-15) doubled, all 12.4
  fixed point; Xenia's. Neither points nor lines are culled.
- Vertex kill: a vertex shader's export 63 Z with bits other than the
  sign kills its vertex; a primitive is dropped when any of its vertices
  is killed (PA_CL_CLIP_CNTL's vtx_kill_or, bit 21), else when all are
  (Xenia's test and rule). Lists whose vertex shader kills draw each
  host vertex by running its primitive's guest vertices; a dropped
  primitive's vertices are put outside the clip volume (Xenia makes the
  position NaN or uses a cull distance).
- The scissor is the window scissor, moved by the window offset unless
  its TL bit 31 says not to, within the screen scissor, as resolves take
  it. RB_COLOR_MASK has four bits a colour target.
- Culling: PA_SU_SC_MODE_CNTL's cull_front and cull_back (bits 0 and
  1; both draw nothing), the front face clockwise where bit 2 is set,
  else counter-clockwise, windings as the window shows them (Y down).
  Polygon modes other than filled are fatal, not implemented yet.
- Blending, per colour target (RB_BLENDCONTROL0-3 at 0x2201, 0x2209-
  0x220B: colour source factor bits 0-4, operation 5-7, destination
  8-12, alpha the same from bit 16; factors 0 zero, 1 one, 4-11 source
  and destination colour and alpha and their complements, 12-15 the
  constant colour and alpha and their complements, 16 source alpha
  saturated; operations add, subtract, min, max, reverse subtract), off
  where it is one, add, zero for both. The constant is RB_BLEND_RED-
  ALPHA (0x2105-0x2108). The host has no constant-alpha factor: where a
  colour factor is the constant's alpha, the constant is made all alpha
  (both constant colour and constant alpha in colour factors is fatal).
- The alpha test (RB_COLORCONTROL bits 0-2 the function, bit 3 on;
  RB_ALPHA_REF) on colour 0's alpha as the shader exports it, before
  the exponent bias: the function's bits less, equal, greater, not
  equal true for NaN, always passing; Xenia's. Alpha to mask (bit 4) is
  fatal, not implemented yet.
- Pixel parameters (SQ_PROGRAM_CNTL's param_gen, bit 18; into the
  register SQ_CONTEXT_MISC's param_gen_pos, bits 8-15, names, when it is
  an interpolator, instead of that interpolator), as Xenia's SPIR-V
  translator builds them: |X| and |Y| the pixel's position in the
  target, whole pixels (MSAA samples divided down); X negative on a
  polygon's back face, Y negative for a point; for a point Z and W its
  sprite coordinates, 0-1 left to right and top to bottom, clamped;
  otherwise Z -0 for a line, else 0, and W 0.
- Interpolation: SQ_INTERPOLATOR_CNTL's flat shading (param_shade) and
  sampling pattern, PA_SU_SC_MODE_CNTL's provoking_vtx_last (bit 19) and
  persp_corr_dis (bit 20) are not modelled, as Xenia does not model
  them: every interpolator is perspective-correct, at sample centres
  (console check).
- Depth and stencil (RB_DEPTHCONTROL 0x2200: stencil bit 0, Z test bit
  1, Z write bit 2, Z function bits 4-6, back faces' own stencil state
  bit 7, the front's function, fail, pass and Z-fail operations from
  bit 8, three bits each, the back's from bit 20; RB_STENCILREFMASK
  0x210D: reference bits 0-7, read mask 8-15, write mask 16-23). The
  compare functions are never, less, equal, less or equal, greater, not
  equal, greater or equal, always; the operations keep, zero, replace,
  increment and decrement clamped, invert, increment and decrement
  wrapped. The depth target is bound where the Z or the stencil test is
  on. Back faces with their own reference or masks
  (RB_STENCILREFMASK_BF) are fatal, not implemented yet (the host has
  one of each).
- Polygon offset: the front's (PA_SU_POLY_OFFSET_FRONT_SCALE and
  _OFFSET, 0x2380-0x2381, where PA_SU_SC_MODE_CNTL bit 11 enables it and
  front faces are not culled), else the back's (0x2382-0x2383, bit 12),
  as Xenia picks one for both faces; lines and points the front's where
  bit 13 enables it; the offset in units of 1/(2^24 -
  1) for D24S8 and 2^-24 for D24FS8, the slope scale in 1/16 pixel
  (Xenia's conversion to a 32-bit float depth buffer, without its
  rounding of the scale to an order of magnitude).
- Each colour output is scaled by 2^RB_COLOR_INFO's exponent bias (bits
  20-25, signed), and a further 1/32 for the fixed-point formats (their
  host targets hold a 32nd), as Xenia does; to an 8_8_8_8 gamma target
  it is converted to the Xenos's piecewise-linear gamma (Xenia's
  LinearToPWLGamma, with its truncation), so blending happens on gamma
  values there, as in Xenia.

## Textures
What a texture fetch samples (texture.cpp). The fetch constant's fields,
the layout of levels and slices, the formats' meanings and their block
decompression are Xenia's (BSD-3: xenos.h, texture_util.cc,
texture_address.h, its texture caches and load shaders); none of it has
been checked on a console yet.
- The fetch constant: six words at 0x4800 + 6n. Word 0: type bits 0-1
  (2 a texture), the components' signs 2-9, clamps X, Y, Z 10-18, pitch
  in texels / 32 22-30, tiled 31. Word 1: format 0-5, endian swap 6-7,
  stacked 10, base address's page 12-31. Word 2: the sizes less one (1D
  24 bits; 2D and cube 13 and 13, stacked layers 26-31; 3D 11, 11 and
  10). Word 3: swizzle 1-12, exponent bias 13-18 (signed), filters mag
  19-20, min 21-22, mip 23-24, anisotropy 25-27. Word 4: volume mag and
  min filters bits 0-1, mip_min_level 2-5, mip_max_level 6-9, LOD bias
  12-21 (signed, 1/32). Word 5: dimension 9-10 (1D, 2D or stacked, 3D,
  cube), packed mips 11, mip address's page 12-31. num_format and the
  gradient exponent adjusts are not used (nor by Xenia).
- Levels, as Xenia's GetSubresourcesFromFetchConstant: with a mip
  address and mip_max_level above 0, levels mip_min_level to
  mip_max_level (clamped to the size), level 0 at the base address and
  the others at the mip address; without a base address the first level
  is 1; otherwise level 0 alone.
- Layout: a level's rows of blocks aligned to 32 blocks (the base's
  pitch the constant's, the mips' their power-of-two sizes'), a linear
  mip's rows to 256 bytes; slices of 32-aligned rows (3D: times depth
  aligned to 4), each 4 KiB-aligned; mips one after the other from the
  mip address. A packed tail starts at the level whose smaller side is
  16 texels (from level 0 when it is that small already) and keeps all
  smaller levels inside that level's storage at Xenia's offsets. Tiled
  2D textures address as resolves write them ("EDRAM and resolves"),
  tiled 3D ones by Xenia's Tiled3D. The texture's 32-bit words are
  endian swapped as the constant says.
- Formats (xenos.h's names) decoded on the CPU into one of five host
  classes: 8-bit unorm RGBA (8, 1_5_5_5, 5_6_5, 6_5_5, 8_8_8_8 and its
  _A and _AS_16_16_16_16 forms, 8_A, 8_8, 4_4_4_4, the YUV pairs, DXT1,
  DXT2_3, DXT4_5, DXN, DXT3A, DXT5A, CTX1, DXT3A_AS_1_1_1_1 and the
  _AS_16_16_16_16 DXTs), 16-bit unorm (2_10_10_10, 10_11_11, 11_11_10
  and their _AS_16_16_16_16 forms, 16, 16_16, 16_16_16_16), 16-bit
  snorm (an integer format with a signed component), half floats
  (16_EXPAND and _FLOAT formats) and floats (24_8 as 24-bit unorm,
  24_8_FLOAT as 20e4, the 32-bit float formats); 10- and 2-bit
  components go to 16-bit unorm rounded to nearest in integers. Missing
  components repeat the last (RRRR, RGGG, RGBB), as Xenia's host
  swizzles do; 8_8_8_8_A is taken as 8_8_8_8 (a choice). The other
  formats (1-bit, 8_B, the EDRAM, 32-bit integer, AS_8, MPEG and
  interlaced ones), tiled formats of 3-word blocks, signed compressed and
  YUV textures and tiled or packed 1D textures are fatal, not
  implemented yet.
- Signs, per data component: 1 signed (v / (2^(n-1) - 1), at least -1;
  the host image then 16-bit snorm), 2 biased (2v - 1) and 3 gamma
  (Xenia's PWLGammaToLinear), both applied after filtering, as Xenia
  does on hosts without such formats; then the swizzle (0-3 a data
  component, 4 zero, 5 one) and 2^exponent bias.
- Kinds: 2D textures as 2D images, stacked ones as 2D arrays, 3D as 3D,
  cubes as six layers. A fetch of dimension 2 of a stacked texture
  takes layer floor(z * layers), or with the magnifying volume filter
  linear (used for minifying too: no gradients here; Xenia picks by
  them) mixes the two layers around z * layers - 0.5. A constant that
  is not a texture, has no memory or is of another kind than the fetch
  samples binds a null texture of the fetch's kind, a texel of zeros
  (Xenia's).
- The sampler: the instruction's filters, where it does not say to use
  the constant's (3 for a filter, 7 for anisotropy); 0 point, 1 linear,
  mip filter 2 the base map (max LOD mip_min_level + 0.25, as Xenia);
  the minimum LOD mip_min_level; the constant's LOD bias; anisotropy
  2^(n-1) for 1-5. Clamps per axis the dimension has: repeat, mirrored
  repeat, clamp to edge; the mirror clamps as mirrored repeat and clamp
  to halfway as clamp to edge (Xenia's fallbacks for hosts without
  them; mirror clamp to border too, a choice); clamp to border is fatal
  (the host device has no border colours).
- Vertex shaders sample textures too, with their own bindings.
- The cache, by the constant's words (less the fields the sampler and
  xe_sample take): a texture decodes when first sampled, after the
  resolves still on the device that wrote its memory land. Decoding and
  hashing run on the CPU (big levels and ranges on several threads).
- A resolve into a texture the cache holds updates it on the device,
  from the EDRAM before the resolve's clears, where its level 0 is the
  copy's destination: a 2D tiled texture of one level (none packed into
  a tail), at the copy's base or that of one of its 32x32 blocks with
  the copy's rectangle within its rows, the copy's pitch and endian
  swap, unsigned (the float formats ignore signs), in a format whose
  texels are the copy's words as they are (8_8_8_8 and its _A and
  _AS_16_16_16_16 forms, 2_10_10_10 and its _AS_16_16_16_16 form,
  16_16, 16_16_16_16, the half float pairs and quadruples as _FLOAT or
  _EXPAND, 32_FLOAT, 32_32_FLOAT, 24_8 and 24_8_FLOAT of a depth
  resolve). Each texel is what decoding the copy's words on the CPU
  gives. A texture there that does not match decodes again.
- Its memory is checked against a hash (a choice: no tracking of CPU
  writes): once a frame (a swap), at its first use, unless a resolve
  still on the device wrote there, it decodes again when the hash
  changed; a texture a resolve updated on the device is hashed when the
  resolve lands. A CPU write within a frame and undone by its end is
  missed, a texture written by the CPU and sampled in the same frame
  after another sample of it shows the older contents, and a CPU write
  to a texture after a resolve updated it on the device and before the
  resolve lands, outside the bytes the resolve writes back, is missed
  (inside them, memory itself loses it).
- Textures not sampled for 300 frames are dropped.

## Render targets
- Colour targets 0-3 (RB_COLOR_INFO, RB_COLOR1-3_INFO at 0x2001,
  0x2003-0x2005: base tile bits 0-11, format bits 16-19) as far as the
  pixel shader exports colours, from 0 on (a gap is fatal); the depth
  target (RB_DEPTH_INFO 0x2002: base tile, format bit 16, 0 D24S8 and 1
  D24FS8); RB_SURFACE_INFO's pitch and MSAA (bits 16-17: none, 2x, 4x;
  3 is fatal). Targets of one draw with the same base tile are fatal.
- A draw's targets use rows up to its scissor's bottom, each also only
  up to the next bound target's base tile (wrapping): a title lays out a
  draw's targets one after the other whatever its scissor (often far
  larger than the surface), so this is the surface's height there.
- Each is a host image per base, pitch, MSAA mode and format, in
  samples: MSAA is supersampled (a choice: the samples are then a
  regular grid, not the Xenos's positions), 2x two samples above each
  other, 4x two by two, so a sample is at the EDRAM word resolves read
  it from (sample 0 top left, 1 below it, 2 and 3 to their right). It is
  as wide as the pitch in 80-sample tiles (the pitch in pixels doubled
  across for 4x) and as high as the EDRAM holds rows of 32bpp tiles of
  that pitch (at most 4096), so a surface's targets are one size; X, Y,
  the scissor, the half pixel and the slope of the polygon offset go to
  samples.
- The host formats: 8_8_8_8 and its gamma variant as 8-bit unorm (the
  gamma one holding gamma values, as Xenia does without an sRGB
  format); 2_10_10_10 (and _AS_10_10_10_10) as 10-bit unorm; its 7e3
  float variants and 16_16_16_16_FLOAT as half floats; 16_16 and
  16_16_16_16, fixed point over [-32, 32], as 16-bit snorm holding a
  32nd of the value (Xenia's); 16_16_FLOAT, 32_FLOAT and 32_32_FLOAT as
  themselves. Depth is a 32-bit float depth with 8-bit stencil.
- A target holds what the EDRAM holds in the rows the draws' scissors
  reach: before a draw, other targets drawn into whose tiles overlap a
  bound one's are stored to the EDRAM, and all overlapping ones become
  stale; a stale target (or rows it had not used) loads from the EDRAM.
  A resolve stores every target drawn into before it reads the EDRAM.
  Its clear clears the host target of the surface it clears where it
  covers all of the target's rows in use and the surface's width (the
  columns of the target's last tile past the surface's width too,
  which nothing of the surface uses), to what a load of the cleared
  words gives; other targets whose tiles the cleared rows touch become
  stale.
- Loads and stores convert as resolves read: a colour target's words
  unpacked and packed back (unorm rounded to nearest; 7e3 rounded to
  the nearest even, clamped to [0, 31.875], below 0.25 denormal, as
  Xenia's SPIR-V translator packs it; the fixed-point formats rounded to
  the nearest even); depth over stencil, 24 bits from bit 8: D24S8 unorm
  (stored rounded to the nearest even, loaded as (d + d >> 23) / 2^24),
  D24FS8 the 20e4 float (4 exponent bits biased by 15, 20 mantissa
  bits, [0, 2); stored rounded to the nearest even, loaded at most 1).
  A depth tile's two 40-sample halves are swapped, as resolves read
  them. The host samples depth but not stencil, so stencil moves only
  where it is used or changed. The renderer keeps, for each EDRAM tile,
  the stencil all its words hold where it is known: the EDRAM starts
  zeroed; a store that writes a known stencil, or a resolve's depth
  clear, sets the tiles it writes all of to that value and keeps the
  others it touches only where they held it already; a store that
  writes a stencil read back, a colour store and a colour clear make
  the tiles they touch unknown. A depth load writes the depth, and the
  stencil where the tiles all hold one known value; otherwise the
  target's stencil is not loaded: a target that is valid holds what the
  EDRAM holds but for what draws changed (any other writer of its tiles
  stores it and makes it stale first), so the EDRAM's words hold its
  stencil until a draw writes it, and a draw that writes it tests it.
  A stencil test that can neither fail nor write (compare Always for
  each face in use, and a write mask of 0 or Keep where it passes and
  where the depth fails) is drawn as if off. The first draw with the
  stencil test on loads it, for all the rows in use: the known value,
  or the stencil cleared and then a bit at a time
  (eight passes setting one bit where the EDRAM's is set), leaving the
  depth as it is. Rows added to a target whose stencil is loaded load
  theirs at once. A store writes the depth over the EDRAM words'
  stencil bytes as they are unless a draw wrote the stencil since the
  target last matched the EDRAM; then it writes the known value, or the
  stencil read back the same way, a bit at a time into an 8-bit image,
  by stencil tests. This is the host's bookkeeping only: what the EDRAM
  holds is the same.

## Presentation
- A swap packet shows its front buffer: the texture fetch constant the
  words before it set (0x4800-0x4805), read as Xenia reads VdSwap's
  (BSD-3): pitch (dword 0, bits 22-30, in 32 texels), tiled (bit 31),
  format (dword 1, bits 0-5), endian (bits 6-7), address (bits 12-31),
  width and height less one (dword 2, 13 bits each), the swizzle (dword
  3, bits 1-12, 3 bits per destination: a source component 0-3, 4 zero,
  5 one). The packet's width and height crop it.
- k_8_8_8_8 only (component X in the low byte after the endian swap);
  other formats and swizzle values 6 and 7 are fatal, not implemented
  yet.
- Texels are shown as stored: the display's gamma ramp is not applied
  (Xenia applies the 256-entry ramp to 8_8_8_8). Console check.
- A swap waits until the frame before it has been shown, so each is
  shown once, in order (a choice, as a host swap chain holds the GPU
  back).

## Interrupts and the vertical blank
- VdSetGraphicsInterruptCallback(callback, argument): the callback runs as
  a DPC, callback(source, argument), source 0 for the vertical blank
  (Xenia: on processor 2), 1 for a packet's interrupt. Here it runs on a
  kernel thread at DISPATCH_LEVEL with the KPCR's DPC flag set.
- The vertical blank comes at the video mode's rate; the scanline counter
  moves on by a frame's lines each time, as Xenia moves it. Console check.

## The display driver's exports
- VdSwap(ring words, front buffer fetch, writeback, system command buffer,
  its value, *front buffer, *format, *color space, *width, *height):
  fills the 64 words the title set aside with the front buffer's fetch
  constant (a type-0 write of 0x4800-0x4805, the address made physical),
  a swap packet (type 3, opcode 0x64, which the GPU leaves unused, then
  "SWAP", the front buffer, width, height), and type-2 no-ops, as Xenia
  does. The real kernel's words are not known. Console check.
- VdGetSystemCommandBuffer(buffer, value): the 0x94 bytes zeroed, the
  value 0; a title zeroes the buffer itself when it skips the swap, and
  reads its second word. VdSetSystemCommandBufferGpuIdentifierAddress
  records nothing (there is no system command buffer here).
- VdInitializeEngines returns TRUE; VdShutdownEngines does nothing
  (titles shut down and initialize again). Xenia's.
- VdInitializeScalerCommandBuffer fills its words with no-ops and
  returns their count; VdPersistDisplay returns TRUE and a page of
  physical memory, which the title frees with MmFreePhysicalMemory.
  Xenia's.
- VdQueryVideoFlags: widescreen 1, 1280 wide 2, 1920 wide 4.
  VdGetCurrentDisplayInformation: X_DISPLAY_INFO (0x58 bytes) at the
  video mode's size, unscaled, overscan 320/180. VdGetCurrentDisplayGamma:
  type 2 (BT.709), power 2.2222. Xenia's.
- The HSIO and EDRAM training calls succeed (nothing to train);
  VdCallGraphicsNotificationRoutines has no routines to call.
