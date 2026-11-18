// Screen layout for the XIAO 1.47" touch display build. See pp_ui147.h.
#if defined(PP_BOARD_DISPLAY147) || defined(PP_UI_HOST_TEST)
#include "pp_ui147.h"

#include <string.h>

namespace {

// ---------------- font ----------------
//
// 5 x 7 pixels in a 6 x 8 cell, ASCII 32..126. One byte per column, bit 0 at
// the top.
static const uint8_t kFont[95][5] = {
  { 0x00, 0x00, 0x00, 0x00, 0x00 }, { 0x00, 0x00, 0x5F, 0x00, 0x00 },  // space !
  { 0x00, 0x07, 0x00, 0x07, 0x00 }, { 0x14, 0x7F, 0x14, 0x7F, 0x14 },  // " #
  { 0x24, 0x2A, 0x7F, 0x2A, 0x12 }, { 0x23, 0x13, 0x08, 0x64, 0x62 },  // $ %
  { 0x36, 0x49, 0x55, 0x22, 0x50 }, { 0x00, 0x05, 0x03, 0x00, 0x00 },  // & '
  { 0x00, 0x1C, 0x22, 0x41, 0x00 }, { 0x00, 0x41, 0x22, 0x1C, 0x00 },  // ( )
  { 0x14, 0x08, 0x3E, 0x08, 0x14 }, { 0x08, 0x08, 0x3E, 0x08, 0x08 },  // * +
  { 0x00, 0x50, 0x30, 0x00, 0x00 }, { 0x08, 0x08, 0x08, 0x08, 0x08 },  // , -
  { 0x00, 0x60, 0x60, 0x00, 0x00 }, { 0x20, 0x10, 0x08, 0x04, 0x02 },  // . /
  { 0x3E, 0x51, 0x49, 0x45, 0x3E }, { 0x00, 0x42, 0x7F, 0x40, 0x00 },  // 0 1
  { 0x42, 0x61, 0x51, 0x49, 0x46 }, { 0x21, 0x41, 0x45, 0x4B, 0x31 },  // 2 3
  { 0x18, 0x14, 0x12, 0x7F, 0x10 }, { 0x27, 0x45, 0x45, 0x45, 0x39 },  // 4 5
  { 0x3C, 0x4A, 0x49, 0x49, 0x30 }, { 0x01, 0x71, 0x09, 0x05, 0x03 },  // 6 7
  { 0x36, 0x49, 0x49, 0x49, 0x36 }, { 0x06, 0x49, 0x49, 0x29, 0x1E },  // 8 9
  { 0x00, 0x36, 0x36, 0x00, 0x00 }, { 0x00, 0x56, 0x36, 0x00, 0x00 },  // : ;
  { 0x08, 0x14, 0x22, 0x41, 0x00 }, { 0x14, 0x14, 0x14, 0x14, 0x14 },  // < =
  { 0x00, 0x41, 0x22, 0x14, 0x08 }, { 0x02, 0x01, 0x51, 0x09, 0x06 },  // > ?
  { 0x32, 0x49, 0x79, 0x41, 0x3E }, { 0x7E, 0x11, 0x11, 0x11, 0x7E },  // @ A
  { 0x7F, 0x49, 0x49, 0x49, 0x36 }, { 0x3E, 0x41, 0x41, 0x41, 0x22 },  // B C
  { 0x7F, 0x41, 0x41, 0x22, 0x1C }, { 0x7F, 0x49, 0x49, 0x49, 0x41 },  // D E
  { 0x7F, 0x09, 0x09, 0x09, 0x01 }, { 0x3E, 0x41, 0x49, 0x49, 0x7A },  // F G
  { 0x7F, 0x08, 0x08, 0x08, 0x7F }, { 0x00, 0x41, 0x7F, 0x41, 0x00 },  // H I
  { 0x20, 0x40, 0x41, 0x3F, 0x01 }, { 0x7F, 0x08, 0x14, 0x22, 0x41 },  // J K
  { 0x7F, 0x40, 0x40, 0x40, 0x40 }, { 0x7F, 0x02, 0x0C, 0x02, 0x7F },  // L M
  { 0x7F, 0x04, 0x08, 0x10, 0x7F }, { 0x3E, 0x41, 0x41, 0x41, 0x3E },  // N O
  { 0x7F, 0x09, 0x09, 0x09, 0x06 }, { 0x3E, 0x41, 0x51, 0x21, 0x5E },  // P Q
  { 0x7F, 0x09, 0x19, 0x29, 0x46 }, { 0x46, 0x49, 0x49, 0x49, 0x31 },  // R S
  { 0x01, 0x01, 0x7F, 0x01, 0x01 }, { 0x3F, 0x40, 0x40, 0x40, 0x3F },  // T U
  { 0x1F, 0x20, 0x40, 0x20, 0x1F }, { 0x3F, 0x40, 0x38, 0x40, 0x3F },  // V W
  { 0x63, 0x14, 0x08, 0x14, 0x63 }, { 0x07, 0x08, 0x70, 0x08, 0x07 },  // X Y
  { 0x61, 0x51, 0x49, 0x45, 0x43 }, { 0x00, 0x7F, 0x41, 0x41, 0x00 },  // Z [
  { 0x02, 0x04, 0x08, 0x10, 0x20 }, { 0x00, 0x41, 0x41, 0x7F, 0x00 },  // backslash ]
  { 0x04, 0x02, 0x01, 0x02, 0x04 }, { 0x40, 0x40, 0x40, 0x40, 0x40 },  // ^ _
  { 0x00, 0x01, 0x02, 0x04, 0x00 }, { 0x20, 0x54, 0x54, 0x54, 0x78 },  // ` a
  { 0x7F, 0x48, 0x44, 0x44, 0x38 }, { 0x38, 0x44, 0x44, 0x44, 0x20 },  // b c
  { 0x38, 0x44, 0x44, 0x48, 0x7F }, { 0x38, 0x54, 0x54, 0x54, 0x18 },  // d e
  { 0x08, 0x7E, 0x09, 0x01, 0x02 }, { 0x0C, 0x52, 0x52, 0x52, 0x3E },  // f g
  { 0x7F, 0x08, 0x04, 0x04, 0x78 }, { 0x00, 0x44, 0x7D, 0x40, 0x00 },  // h i
  { 0x20, 0x40, 0x44, 0x3D, 0x00 }, { 0x7F, 0x10, 0x28, 0x44, 0x00 },  // j k
  { 0x00, 0x41, 0x7F, 0x40, 0x00 }, { 0x7C, 0x04, 0x18, 0x04, 0x78 },  // l m
  { 0x7C, 0x08, 0x04, 0x04, 0x78 }, { 0x38, 0x44, 0x44, 0x44, 0x38 },  // n o
  { 0x7C, 0x14, 0x14, 0x14, 0x08 }, { 0x08, 0x14, 0x14, 0x18, 0x7C },  // p q
  { 0x7C, 0x08, 0x04, 0x04, 0x08 }, { 0x48, 0x54, 0x54, 0x54, 0x20 },  // r s
  { 0x04, 0x3F, 0x44, 0x40, 0x20 }, { 0x3C, 0x40, 0x40, 0x20, 0x7C },  // t u
  { 0x1C, 0x20, 0x40, 0x20, 0x1C }, { 0x3C, 0x40, 0x30, 0x40, 0x3C },  // v w
  { 0x44, 0x28, 0x10, 0x28, 0x44 }, { 0x0C, 0x50, 0x50, 0x50, 0x3C },  // x y
  { 0x44, 0x64, 0x54, 0x4C, 0x44 }, { 0x00, 0x08, 0x36, 0x41, 0x00 },  // z {
  { 0x00, 0x00, 0x7F, 0x00, 0x00 }, { 0x00, 0x41, 0x36, 0x08, 0x00 },  // | }
  { 0x02, 0x01, 0x02, 0x04, 0x02 },                                    // ~
};

static const uint8_t* glyph(char c)
{
  uint8_t i = static_cast<uint8_t>(c);
  if (i < 32 || i > 126) i = '?';
  return kFont[i - 32];
}

static constexpr uint8_t kMaxScale = 4;

// Transparent text: only the glyph pixels are drawn, one fillRect per run.
static void drawTextOver(PPCanvas& canvas, int16_t x, int16_t y, const char* s, uint8_t scale,
                         uint16_t fg)
{
  for (; *s; ++s, x += 6 * scale) {
    const uint8_t* g = glyph(*s);
    for (uint8_t row = 0; row < 7; ++row) {
      int8_t runStart = -1;
      for (uint8_t col = 0; col <= 5; ++col) {
        bool on = col < 5 && (g[col] >> row) & 1;
        if (on && runStart < 0) runStart = col;
        if (!on && runStart >= 0) {
          canvas.fillRect(x + runStart * scale, y + row * scale, (col - runStart) * scale, scale, fg);
          runStart = -1;
        }
      }
    }
  }
}

// Largest scale (up to maxScale) at which s fits in width.
static uint8_t fitScale(const char* s, int16_t width, uint8_t maxScale)
{
  for (uint8_t sc = maxScale; sc > 1; --sc) {
    if (ppUiTextWidth(s, sc) <= width) return sc;
  }
  return 1;
}

static void drawCentred(PPCanvas& canvas, int16_t y, const char* s, uint8_t scale, uint16_t fg,
                        uint16_t bg)
{
  int16_t w = ppUiTextWidth(s, scale);
  ppUiDrawText(canvas, (PP_UI_W - w) / 2, y, s, scale, fg, bg);
}

// ---------------- layout ----------------

static constexpr int16_t kStatusY = 0, kStatusH = 24;
static constexpr int16_t kBannerY = 26, kBannerH = 80;
static constexpr int16_t kModeY = 110, kModeH = 96;
static constexpr int16_t kCountY = 214, kCountRowH = 26;
static constexpr int16_t kPad = 6;

struct BannerLook {
  const char* title;
  const char* sub;
  uint16_t bg;
  uint16_t fg;
};

static BannerLook bannerLook(const PPUiState& st)
{
  switch (st.banner) {
    case PP_UI_BANNER_POKEMON: return { "POKEMON", "nearby", PP_UI_GREEN, PP_UI_BLACK };
    case PP_UI_BANNER_NEW_POKEMON: return { "NEW!", "new Pokemon", PP_UI_YELLOW, PP_UI_BLACK };
    case PP_UI_BANNER_POKESTOP: return { "STOP", "Pokestop", PP_UI_BLUE, PP_UI_WHITE };
    case PP_UI_BANNER_CAUGHT: return { "CAUGHT!", "got it", PP_UI_BLACK, PP_UI_WHITE };
    case PP_UI_BANNER_FLED: return { "FLED", "it ran away", PP_UI_RED, PP_UI_WHITE };
    case PP_UI_BANNER_ITEMS: return { "ITEMS", "stop spun", PP_UI_BLUE, PP_UI_WHITE };
    case PP_UI_BANNER_BAG_FULL: return { "BAG FULL", "no items", PP_UI_ORANGE, PP_UI_BLACK };
    case PP_UI_BANNER_BOX_FULL: return { "BOX FULL", "no catching", PP_UI_ORANGE, PP_UI_BLACK };
    case PP_UI_BANNER_NO_BALLS: return { "NO BALLS", "out of balls", PP_UI_ORANGE, PP_UI_BLACK };
    case PP_UI_BANNER_PRESSED: return { "PRESS", "button sent", PP_UI_WHITE, PP_UI_BLACK };
    case PP_UI_BANNER_NOT_CONNECTED: return { "NO LINK", "not connected", PP_UI_RED, PP_UI_WHITE };
    case PP_UI_BANNER_ADVERTISING: return { "SEARCH", "fast, 2 min", PP_UI_YELLOW, PP_UI_BLACK };
    default: break;
  }
  switch (st.link) {
    case PP_UI_LINK_HIDDEN: return { "HIDDEN", "adv. is off", PP_UI_CARD, PP_UI_GREY };
    case PP_UI_LINK_SEARCHING: return { "SEARCH", "open the app", PP_UI_CARD, PP_UI_YELLOW };
    case PP_UI_LINK_PAIRING: return { "LINKING", "please wait", PP_UI_CARD, PP_UI_ORANGE };
    default: return { "READY", "waiting", PP_UI_CARD, PP_UI_GREEN };
  }
}

static void drawStatus(PPCanvas& canvas, const PPUiState& st)
{
  canvas.fillRect(0, kStatusY, PP_UI_W, kStatusH, PP_UI_BLACK);
  const char* link = "LINKED";
  uint16_t linkColour = PP_UI_GREEN;
  switch (st.link) {
    case PP_UI_LINK_HIDDEN: link = "HIDDEN"; linkColour = PP_UI_GREY; break;
    case PP_UI_LINK_SEARCHING: link = "SEARCHING"; linkColour = PP_UI_YELLOW; break;
    case PP_UI_LINK_PAIRING: link = "PAIRING"; linkColour = PP_UI_ORANGE; break;
    default: break;
  }
  ppUiDrawText(canvas, kPad, kStatusY + 4, link, 2, linkColour, PP_UI_BLACK);

  char power[5];
  uint16_t powerColour = PP_UI_WHITE;
  if (st.power == PP_UI_POWER_CHARGING) {
    strcpy(power, "CHG");
    powerColour = PP_UI_GREEN;
  } else if (st.power == PP_UI_POWER_USB) {
    strcpy(power, "USB");
    powerColour = PP_UI_GREY;
  } else {
    uint8_t pct = st.batteryPct > 100 ? 100 : st.batteryPct;
    uint8_t n = 0;
    if (pct >= 100) power[n++] = '1';
    if (pct >= 10) power[n++] = static_cast<char>('0' + (pct / 10) % 10);
    power[n++] = static_cast<char>('0' + pct % 10);
    power[n++] = '%';
    power[n] = 0;
    if (pct < 20) powerColour = PP_UI_RED;
  }
  int16_t w = ppUiTextWidth(power, 2);
  ppUiDrawText(canvas, PP_UI_W - kPad - w, kStatusY + 4, power, 2, powerColour, PP_UI_BLACK);
}

static void drawBanner(PPCanvas& canvas, const PPUiState& st)
{
  BannerLook look = bannerLook(st);
  canvas.fillRect(0, kBannerY, PP_UI_W, kBannerH, look.bg);
  if (st.banner == PP_UI_BANNER_CAUGHT) {
    // The original's caught pattern runs red, green, blue: three bars.
    int16_t third = PP_UI_W / 3;
    canvas.fillRect(0, kBannerY, third, 8, PP_UI_RED);
    canvas.fillRect(third, kBannerY, PP_UI_W - 2 * third, 8, PP_UI_GREEN);
    canvas.fillRect(PP_UI_W - third, kBannerY, third, 8, PP_UI_BLUE);
  }
  uint8_t sc = fitScale(look.title, PP_UI_W - 4, kMaxScale);
  int16_t titleY = kBannerY + 14 + (4 - sc) * 4;
  int16_t w = ppUiTextWidth(look.title, sc);
  drawTextOver(canvas, (PP_UI_W - w) / 2, titleY, look.title, sc, look.fg);
  w = ppUiTextWidth(look.sub, 2);
  drawTextOver(canvas, (PP_UI_W - w) / 2, kBannerY + 56, look.sub, 2, look.fg);
}

static void drawMode(PPCanvas& canvas, const PPUiState& st)
{
  static const char* const kNames[] = { "AUTO", "CATCH", "SPIN", "MANUAL" };
  static const char* const kSubs[] = { "catch + spin", "catch only", "spin only", "press A" };
  static const uint16_t kColours[] = { PP_UI_GREEN, PP_UI_YELLOW, PP_UI_BLUE, PP_UI_WHITE };
  uint8_t p = ppUiPreset(st.catchMode, st.spin);
  canvas.fillRect(0, kModeY, PP_UI_W, kModeH, PP_UI_CARD);
  ppUiDrawText(canvas, kPad, kModeY + 8, "MODE", 2, PP_UI_GREY, PP_UI_CARD);
  int16_t w = ppUiTextWidth("B", 2);
  ppUiDrawText(canvas, PP_UI_W - kPad - w, kModeY + 8, "B", 2, PP_UI_GREY, PP_UI_CARD);
  drawCentred(canvas, kModeY + 34, kNames[p], 4, kColours[p], PP_UI_CARD);
  drawCentred(canvas, kModeY + 72, kSubs[p], 2, PP_UI_WHITE, PP_UI_CARD);
}

static void drawRow(PPCanvas& canvas, uint8_t row, const char* label, uint16_t colour,
                    const char* value, uint16_t valueColour)
{
  int16_t y = kCountY + row * kCountRowH;
  canvas.fillRect(0, y, PP_UI_W, kCountRowH, PP_UI_BLACK);
  int16_t ty = y + (kCountRowH - 16) / 2;
  ppUiDrawText(canvas, kPad, ty, label, 2, colour, PP_UI_BLACK);
  int16_t w = ppUiTextWidth(value, 2);
  ppUiDrawText(canvas, PP_UI_W - kPad - w, ty, value, 2, valueColour, PP_UI_BLACK);
}

static void drawCounter(PPCanvas& canvas, uint8_t row, const char* label, uint16_t colour, uint16_t value)
{
  char num[6];
  uint8_t n = 0;
  char rev[6];
  do {
    rev[n++] = static_cast<char>('0' + value % 10);
    value /= 10;
  } while (value && n < 5);
  for (uint8_t i = 0; i < n; ++i) num[i] = rev[n - 1 - i];
  num[n] = 0;
  drawRow(canvas, row, label, colour, num, PP_UI_WHITE);
}

static void drawLinked(PPCanvas& canvas, const PPUiState& st)
{
  if (st.linkedSec == PP_UI_NOT_LINKED) {
    drawRow(canvas, 3, "LINKED", PP_UI_WHITE, "--", PP_UI_GREY);
    return;
  }
  char t[9];
  ppUiFormatDuration(st.linkedSec, t);
  drawRow(canvas, 3, "LINKED", PP_UI_WHITE, t, PP_UI_WHITE);
}

static char* putNum(char* p, uint32_t v, uint8_t minDigits)
{
  char rev[10];
  uint8_t n = 0;
  do {
    rev[n++] = static_cast<char>('0' + v % 10);
    v /= 10;
  } while (v && n < sizeof(rev));
  while (n < minDigits) rev[n++] = '0';
  while (n) *p++ = rev[--n];
  return p;
}

}  // namespace

uint8_t ppUiPreset(uint8_t catchMode, bool spin)
{
  bool catching = catchMode != 0;
  if (catching && spin) return PP_UI_PRESET_AUTO;
  if (catching) return PP_UI_PRESET_CATCH;
  if (spin) return PP_UI_PRESET_SPIN;
  return PP_UI_PRESET_MANUAL;
}

void ppUiFormatDuration(uint32_t sec, char out[9])
{
  uint32_t h = sec / 3600, m = (sec / 60) % 60, s = sec % 60;
  char* p = out;
  if (h >= 10) {
    if (h > 99) { h = 99; m = 59; }
    p = putNum(p, h, 1);
    *p++ = 'h';
    *p++ = ' ';
    p = putNum(p, m, 2);
    *p++ = 'm';
  } else if (h) {
    p = putNum(p, h, 1);
    *p++ = ':';
    p = putNum(p, m, 2);
    *p++ = ':';
    p = putNum(p, s, 2);
  } else {
    p = putNum(p, m, 1);
    *p++ = ':';
    p = putNum(p, s, 2);
  }
  *p = 0;
}

int16_t ppUiTextWidth(const char* s, uint8_t scale)
{
  int16_t n = static_cast<int16_t>(strlen(s));
  return n ? static_cast<int16_t>((n * 6 - 1) * scale) : 0;
}

void ppUiDrawText(PPCanvas& canvas, int16_t x, int16_t y, const char* s, uint8_t scale,
                  uint16_t fg, uint16_t bg)
{
  if (scale < 1) scale = 1;
  if (scale > kMaxScale) scale = kMaxScale;
  uint16_t cell[6 * kMaxScale * 8 * kMaxScale];
  const int16_t cw = 6 * scale, ch = 8 * scale;
  for (; *s; ++s, x += cw) {
    const uint8_t* g = glyph(*s);
    for (int16_t py = 0; py < ch; ++py) {
      uint8_t row = py / scale;
      for (int16_t px = 0; px < cw; ++px) {
        uint8_t col = px / scale;
        bool on = col < 5 && row < 7 && ((g[col] >> row) & 1);
        cell[py * cw + px] = on ? fg : bg;
      }
    }
    // The last cell drops its spacing column so text ends at its width.
    int16_t w = s[1] ? cw : cw - scale;
    if (w == cw) {
      canvas.blit(x, y, cw, ch, cell);
    } else {
      for (int16_t py = 0; py < ch; ++py) canvas.blit(x, y + py, w, 1, cell + py * cw);
    }
  }
}

void ppUiDraw(PPCanvas& canvas, const PPUiState& st, const PPUiState* prev)
{
  bool all = prev == nullptr;
  if (all) {
    canvas.fillRect(0, 0, PP_UI_W, PP_UI_H, PP_UI_BLACK);
  }
  if (all || prev->link != st.link || prev->power != st.power ||
      (st.power == PP_UI_POWER_BATTERY && prev->batteryPct != st.batteryPct)) {
    drawStatus(canvas, st);
  }
  if (all || prev->banner != st.banner || (st.banner == PP_UI_BANNER_IDLE && prev->link != st.link)) {
    drawBanner(canvas, st);
  }
  if (all || ppUiPreset(prev->catchMode, prev->spin) != ppUiPreset(st.catchMode, st.spin)) {
    drawMode(canvas, st);
  }
  if (all || prev->caught != st.caught) drawCounter(canvas, 0, "CAUGHT", PP_UI_GREEN, st.caught);
  if (all || prev->fled != st.fled) drawCounter(canvas, 1, "FLED", PP_UI_RED, st.fled);
  if (all || prev->spins != st.spins) drawCounter(canvas, 2, "SPINS", PP_UI_BLUE, st.spins);
  if (all || prev->linkedSec != st.linkedSec) drawLinked(canvas, st);
}

void ppUiDrawFatal(PPCanvas& canvas, const char* what)
{
  canvas.fillRect(0, 0, PP_UI_W, PP_UI_H, PP_UI_BLACK);
  drawCentred(canvas, 40, "ERROR", 4, PP_UI_RED, PP_UI_BLACK);
  // Wrap at 13 characters (scale 2), on spaces where possible.
  char line[14];
  int16_t y = 100;
  const char* p = what;
  while (*p && y < PP_UI_H - 20) {
    size_t n = strlen(p);
    if (n > 13) {
      n = 13;
      for (size_t i = 13; i > 0; --i) {
        if (p[i] == ' ') { n = i; break; }
      }
    }
    memcpy(line, p, n);
    line[n] = 0;
    ppUiDrawText(canvas, kPad, y, line, 2, PP_UI_WHITE, PP_UI_BLACK);
    p += n;
    while (*p == ' ') ++p;
    y += 22;
  }
}

uint8_t ppUiHitTest(int16_t x, int16_t y)
{
  if (x < 0 || x >= PP_UI_W) return PP_UI_ZONE_NONE;
  if (y >= kBannerY && y < kBannerY + kBannerH) return PP_UI_ZONE_BANNER;
  if (y >= kModeY && y < kModeY + kModeH) return PP_UI_ZONE_MODE;
  return PP_UI_ZONE_NONE;
}

#endif  // PP_BOARD_DISPLAY147 || PP_UI_HOST_TEST
