#pragma once
// Screen layout for the XIAO 1.47" touch display build (-DPP_BOARD_DISPLAY147).
// Plain C++ with no Arduino calls: the board file draws it on the LCD, and
// display147/test compiles it on the Mac (-DPP_UI_HOST_TEST) to render the
// screens into pictures.
//
// Portrait, 172 x 320, top to bottom:
//   status bar   link state, battery or USB
//   banner       the last event (Pokemon, caught, ...) or the idle state
//   mode card    the B-button preset: AUTO / CATCH / SPIN / MANUAL
//   counters     caught, fled, spins since boot, and how long the phone has
//                been connected

#include <stdint.h>

static constexpr int16_t PP_UI_W = 172;
static constexpr int16_t PP_UI_H = 320;

// RGB565.
static constexpr uint16_t PP_UI_BLACK = 0x0000;
static constexpr uint16_t PP_UI_WHITE = 0xFFFF;
static constexpr uint16_t PP_UI_GREY = 0x8410;
static constexpr uint16_t PP_UI_CARD = 0x2124;
static constexpr uint16_t PP_UI_GREEN = 0x2E68;
static constexpr uint16_t PP_UI_YELLOW = 0xFEA0;
static constexpr uint16_t PP_UI_BLUE = 0x2B9F;
static constexpr uint16_t PP_UI_RED = 0xE8E4;
static constexpr uint16_t PP_UI_ORANGE = 0xFC60;

// Where pixels go. The board file implements it on the LCD, the host test on
// a framebuffer.
class PPCanvas {
public:
  virtual ~PPCanvas() {}
  virtual void fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t colour) = 0;
  // w * h pixels, row by row.
  virtual void blit(int16_t x, int16_t y, int16_t w, int16_t h, const uint16_t* px) = 0;
};

enum PPUiLink : uint8_t {
  PP_UI_LINK_HIDDEN = 0,    // advertising turned off
  PP_UI_LINK_SEARCHING,     // advertising, nobody connected
  PP_UI_LINK_PAIRING,       // connected, not authenticated yet
  PP_UI_LINK_READY,         // connected and authenticated
};

enum PPUiPower : uint8_t {
  PP_UI_POWER_BATTERY = 0,
  PP_UI_POWER_USB,
  PP_UI_POWER_CHARGING,
};

enum PPUiBanner : uint8_t {
  PP_UI_BANNER_IDLE = 0,    // shows the link state
  PP_UI_BANNER_POKEMON,
  PP_UI_BANNER_NEW_POKEMON,
  PP_UI_BANNER_POKESTOP,
  PP_UI_BANNER_CAUGHT,
  PP_UI_BANNER_FLED,
  PP_UI_BANNER_ITEMS,
  PP_UI_BANNER_BAG_FULL,
  PP_UI_BANNER_BOX_FULL,
  PP_UI_BANNER_NO_BALLS,
  PP_UI_BANNER_NO_SPIN,
  PP_UI_BANNER_PRESSED,
  PP_UI_BANNER_NOT_CONNECTED,
  PP_UI_BANNER_ADVERTISING,
  PP_UI_BANNER_COUNT
};

struct PPUiState {
  uint8_t link;        // PPUiLink
  uint8_t catchMode;   // PPCatchMode: 0 off, otherwise catching
  bool spin;
  uint16_t caught;
  uint16_t fled;
  uint16_t spins;
  uint32_t linkedSec;  // seconds this connection has lasted; PP_UI_NOT_LINKED if none
  uint8_t power;       // PPUiPower
  uint8_t batteryPct;  // 0..100; shown on battery power only
  uint8_t banner;      // PPUiBanner
};

static constexpr uint32_t PP_UI_NOT_LINKED = 0xFFFFFFFFUL;

// The B-button presets, derived from catch mode and auto spin.
enum PPUiPreset : uint8_t {
  PP_UI_PRESET_AUTO = 0,    // catch + spin
  PP_UI_PRESET_CATCH,       // catch only
  PP_UI_PRESET_SPIN,        // spin only
  PP_UI_PRESET_MANUAL,      // neither: every press is yours
};
uint8_t ppUiPreset(uint8_t catchMode, bool spin);

// Draws the screen. prev = what is on the glass now, or nullptr to draw all
// of it; only the regions whose fields changed are drawn again.
void ppUiDraw(PPCanvas& canvas, const PPUiState& now, const PPUiState* prev);

// A full-screen message (start-up failure).
void ppUiDrawFatal(PPCanvas& canvas, const char* what);

// What a tap at (x, y) hits.
enum PPUiZone : uint8_t {
  PP_UI_ZONE_NONE = 0,
  PP_UI_ZONE_BANNER,
  PP_UI_ZONE_MODE,
};
uint8_t ppUiHitTest(int16_t x, int16_t y);

// A connection time as at most 7 characters plus the terminator: "0:42",
// "12:05", "1:02:03" under 10 hours, then "12h 34m", capped at "99h 59m".
void ppUiFormatDuration(uint32_t sec, char out[9]);

// Text, also used by the host test. scale 1 = 6 x 8 pixel cells.
int16_t ppUiTextWidth(const char* s, uint8_t scale);
void ppUiDrawText(PPCanvas& canvas, int16_t x, int16_t y, const char* s, uint8_t scale,
                  uint16_t fg, uint16_t bg);
