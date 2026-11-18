// The runtime test title (title/), started as a game project starts its
// title: lifted, then run on the runtime with the kernel's HLE module.
//   xbox360_title <title.xex> <its disc's directory> <a data directory>
//
// Its controllers are two virtual SDL gamepads, the only ones it sees (SDL
// ignores the host's). The first, player 1's, holds A, d-pad up, the
// guide button and the left stick up. The title asks for changes by
// rumbling it: low-frequency speed 0x1111 unplugs the second pad, 0x2222
// plugs it in again, 0x3333 presses B on the first and pulls its left
// trigger (SDL holds a trigger's value from before the pad is opened
// back, so it is not set before). A speed with bit 15 set marks a point
// of its GPU checks: from the last mark (0x8000 only sets one), resolves
// landed n times in all (bits 10-14), n by draws' and packets' reads of
// memory a resolve wrote (5-9) and draws loaded n host targets (0-4);
// where the high-frequency speed has bit 15 set too, loads drew n
// stencil bits (its bits 0-7) and stores n (8-14).

#include <SDL3/SDL.h>

#include <condition_variable>
#include <cstdio>
#include <deque>
#include <mutex>
#include <thread>

#include "xbox360/edram.h"
#include "xbox360/process.h"
#include "xbox360/render.h"

namespace {

constexpr uint16_t kVendor = 0xF00D, kProduct = 0x0360;

std::mutex g_lock;
std::condition_variable g_cv;
std::deque<uint16_t> g_requests;
uint16_t g_last = 0;

// The GPU counters at the last mark.
uint64_t g_landings, g_overlap_landings, g_loads, g_load_bits, g_store_bits;

void Mark(uint16_t m, uint16_t bits) {
  using namespace recomp::xbox360;
  uint64_t a = ResolveLandings() - g_landings, b = ResolveOverlapLandings() - g_overlap_landings,
           c = TargetLoads() - g_loads, d = StencilBitDraws(false) - g_load_bits,
           e = StencilBitDraws(true) - g_store_bits;
  if (m != 0x8000 && (a != (m >> 10 & 31u) || b != (m >> 5 & 31u) || c != (m & 31u)))
    std::fprintf(stderr, "FAIL: GPU mark %04x: %llu landings, %llu by reads, %llu target loads\n", m,
                 (unsigned long long)a, (unsigned long long)b, (unsigned long long)c);
  if (bits & 0x8000 && (d != (bits & 255u) || e != (bits >> 8 & 127u)))
    std::fprintf(stderr, "FAIL: GPU mark %04x %04x: %llu stencil bits loaded, %llu stored\n", m, bits,
                 (unsigned long long)d, (unsigned long long)e);
  g_landings += a;
  g_overlap_landings += b;
  g_loads += c;
  g_load_bits += d;
  g_store_bits += e;
}

// Called by SDL inside the title's XamInputSetState: a mark is checked at
// once, other requests are carried out on the harness's own thread.
bool Rumble(void*, uint16_t low, uint16_t high) {
  std::lock_guard<std::mutex> g(g_lock);
  if (low & 0x8000) {
    if (low != g_last) Mark(low, high);
    g_last = low;
    return true;
  }
  if (low != g_last && low) g_requests.push_back(low);
  g_last = low;
  g_cv.notify_one();
  return true;
}

SDL_JoystickID Attach(bool first) {
  SDL_VirtualJoystickDesc d;
  SDL_INIT_INTERFACE(&d);
  d.type = SDL_JOYSTICK_TYPE_GAMEPAD;
  d.vendor_id = kVendor;
  d.product_id = kProduct;
  d.naxes = SDL_GAMEPAD_AXIS_COUNT;
  d.nbuttons = SDL_GAMEPAD_BUTTON_COUNT;
  d.name = first ? "runtime test pad 1" : "runtime test pad 2";
  if (first) d.Rumble = Rumble;
  SDL_JoystickID id = SDL_AttachVirtualJoystick(&d);
  if (!id) std::fprintf(stderr, "FAIL: virtual gamepad: %s\n", SDL_GetError());
  return id;
}

void Pads() {
  SDL_SetHint(SDL_HINT_NO_SIGNAL_HANDLERS, "1");
  char except[32];
  std::snprintf(except, sizeof except, "0x%04x/0x%04x", kVendor, kProduct);
  SDL_SetHint(SDL_HINT_GAMECONTROLLER_IGNORE_DEVICES_EXCEPT, except);
  if (!SDL_InitSubSystem(SDL_INIT_JOYSTICK)) {
    std::fprintf(stderr, "FAIL: SDL joystick init: %s\n", SDL_GetError());
    return;
  }
  SDL_Joystick* first = SDL_OpenJoystick(Attach(true));
  SDL_JoystickID second = Attach(false);
  SDL_SetJoystickVirtualButton(first, SDL_GAMEPAD_BUTTON_SOUTH, true);
  SDL_SetJoystickVirtualButton(first, SDL_GAMEPAD_BUTTON_DPAD_UP, true);
  SDL_SetJoystickVirtualButton(first, SDL_GAMEPAD_BUTTON_GUIDE, true);
  SDL_SetJoystickVirtualAxis(first, SDL_GAMEPAD_AXIS_LEFTY, -32768);
  std::thread([first, second]() mutable {
    for (;;) {
      uint16_t r;
      {
        std::unique_lock<std::mutex> l(g_lock);
        g_cv.wait(l, [] { return !g_requests.empty(); });
        r = g_requests.front();
        g_requests.pop_front();
      }
      if (r == 0x1111) SDL_DetachVirtualJoystick(second);
      if (r == 0x2222) second = Attach(false);
      if (r == 0x3333) {
        SDL_SetJoystickVirtualButton(first, SDL_GAMEPAD_BUTTON_EAST, true);
        SDL_SetJoystickVirtualAxis(first, SDL_GAMEPAD_AXIS_LEFT_TRIGGER, 32767);
      }
    }
  }).detach();
}

}  // namespace

int main(int argc, char** argv) {
  if (argc != 4) {
    std::fprintf(stderr, "usage: xbox360_title title.xex game-dir data-dir\n");
    return 2;
  }
  Pads();
  recomp::xbox360::RunProcess({"XBOX360_TITLE", argv[1], argv[2], argv[3]});
}
