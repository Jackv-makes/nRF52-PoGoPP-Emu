// LCD, backlight and touch of the Seeed XIAO 1.47" display. See pp_lcd147.h.
#ifdef PP_BOARD_DISPLAY147
#include "pp_lcd147.h"

#include <SPI.h>
#include <Wire.h>

namespace {

// Arduino pin numbers on the XIAO nRF52840 Plus variant.
static constexpr uint8_t kPinCs = D2;        // P0.28
static constexpr uint8_t kPinDc = D3;        // P0.29
static constexpr uint8_t kPinRst = D17;      // P1.03, LCD and touch reset
static constexpr uint8_t kPinBl = D18;       // P1.05, backlight, active high
static constexpr uint8_t kPinSdCs = D6;      // P1.11, microSD on the same SPI bus
static constexpr uint8_t kPinTouchInt = D7;  // P1.12, low while touched

static constexpr int16_t kColOffset = 34;    // 172 visible of 240 columns
// Seeed runs this panel at 25 MHz; SPIM3 picks 16 MHz below 32.
static const SPISettings kSpi(16000000, MSBFIRST, SPI_MODE0);

static constexpr uint8_t kTouchAddr = 0x63;
static constexpr uint8_t kTouchReg = 0x01;
static constexpr uint8_t kTouchLen = 14;
static constexpr uint32_t kTouchPollMs = 40;

// HwPWM3 is left alone by analogWrite until the other three are taken; the
// token marks it as ours.
static constexpr uint32_t kPwmToken = 0x4C434431UL;  // "LCD1"

// The controller's commands.
enum : uint8_t {
  SWRESET = 0x01, SLPIN = 0x10, SLPOUT = 0x11, NORON = 0x13, INVOFF = 0x20, INVON = 0x21,
  DISPOFF = 0x28, DISPON = 0x29, CASET = 0x2A, RASET = 0x2B, RAMWR = 0x2C,
  MADCTL = 0x36, COLMOD = 0x3A,
};

// Pixel staging: RGB565 goes out high byte first.
static constexpr size_t kChunkPx = 256;
static uint16_t gChunk[kChunkPx];

static inline uint16_t swap16(uint16_t v) { return static_cast<uint16_t>((v << 8) | (v >> 8)); }

}  // namespace

void PPLcd147::select()
{
  SPI.beginTransaction(kSpi);
  digitalWrite(kPinCs, LOW);
}

void PPLcd147::deselect()
{
  digitalWrite(kPinCs, HIGH);
  SPI.endTransaction();
}

void PPLcd147::command(uint8_t cmd)
{
  digitalWrite(kPinDc, LOW);
  SPI.transfer(cmd);
  digitalWrite(kPinDc, HIGH);
}

void PPLcd147::data(const uint8_t* bytes, size_t n)
{
  // EasyDMA reads from RAM only, so constant tables go through a copy.
  uint8_t buf[16];
  while (n) {
    size_t k = n > sizeof(buf) ? sizeof(buf) : n;
    memcpy(buf, bytes, k);
    SPI.transfer(buf, nullptr, k);
    bytes += k;
    n -= k;
  }
}

void PPLcd147::command(uint8_t cmd, const uint8_t* bytes, size_t n)
{
  command(cmd);
  data(bytes, n);
}

void PPLcd147::begin()
{
  pinMode(kPinSdCs, OUTPUT);
  digitalWrite(kPinSdCs, HIGH);   // the card stays off the bus
  pinMode(kPinCs, OUTPUT);
  digitalWrite(kPinCs, HIGH);
  pinMode(kPinDc, OUTPUT);
  digitalWrite(kPinDc, HIGH);
  pinMode(kPinBl, OUTPUT);
  digitalWrite(kPinBl, LOW);
  pinMode(kPinTouchInt, INPUT_PULLUP);

  // One reset for the panel and the touch chip.
  pinMode(kPinRst, OUTPUT);
  digitalWrite(kPinRst, HIGH);
  delay(5);
  digitalWrite(kPinRst, LOW);
  delay(200);
  digitalWrite(kPinRst, HIGH);
  delay(300);

  SPI.begin();
  Wire.begin();
  Wire.setClock(400000);

  initPanel();
  _touchDown = false;
}

void PPLcd147::initPanel()
{
  // Seeed's start-up values for this panel (see pp_lcd147.h).
  static const uint8_t kMadctlInit[] = { 0x08 };
  static const uint8_t kB6[] = { 0x0A, 0x82 };
  static const uint8_t kRamctrl[] = { 0x00, 0xE0 };
  static const uint8_t kColmod[] = { 0x55 };           // 16-bit pixels
  static const uint8_t kPorch[] = { 0x0C, 0x0C, 0x00, 0x33, 0x33 };
  static const uint8_t kGate[] = { 0x35 };
  static const uint8_t kVcom[] = { 0x28 };
  static const uint8_t kLcm[] = { 0x0C };
  static const uint8_t kVdvVrhEn[] = { 0x01, 0xFF };
  static const uint8_t kVrh[] = { 0x10 };
  static const uint8_t kVdv[] = { 0x20 };
  static const uint8_t kFrameRate[] = { 0x0F };
  static const uint8_t kPower[] = { 0xA4, 0xA1 };
  static const uint8_t kGammaPos[] = { 0xD0, 0x00, 0x02, 0x07, 0x0A, 0x28, 0x32,
                                       0x44, 0x42, 0x06, 0x0E, 0x12, 0x14, 0x17 };
  static const uint8_t kGammaNeg[] = { 0xD0, 0x00, 0x02, 0x07, 0x0A, 0x28, 0x31,
                                       0x54, 0x47, 0x0E, 0x1C, 0x17, 0x1B, 0x1E };
  static const uint8_t kMadctl[] = { 0x48 };           // column mirror, BGR

  select();
  command(SLPOUT);
  deselect();
  delay(120);
  select();
  command(NORON);
  command(MADCTL, kMadctlInit, sizeof(kMadctlInit));
  command(0xB6, kB6, sizeof(kB6));
  command(0xB0, kRamctrl, sizeof(kRamctrl));
  command(COLMOD, kColmod, sizeof(kColmod));
  deselect();
  delay(10);
  select();
  command(0xB2, kPorch, sizeof(kPorch));
  command(0xB7, kGate, sizeof(kGate));
  command(0xBB, kVcom, sizeof(kVcom));
  command(0xC0, kLcm, sizeof(kLcm));
  command(0xC2, kVdvVrhEn, sizeof(kVdvVrhEn));
  command(0xC3, kVrh, sizeof(kVrh));
  command(0xC4, kVdv, sizeof(kVdv));
  command(0xC6, kFrameRate, sizeof(kFrameRate));
  command(0xD0, kPower, sizeof(kPower));
  command(0xE0, kGammaPos, sizeof(kGammaPos));
  command(0xE1, kGammaNeg, sizeof(kGammaNeg));
  command(INVOFF);                                     // this glass: normal colours
  command(MADCTL, kMadctl, sizeof(kMadctl));
  deselect();
  fillRect(0, 0, PP_UI_W, PP_UI_H, PP_UI_BLACK);
  select();
  command(DISPON);
  deselect();
  delay(20);
  _asleep = false;
}

void PPLcd147::reinit()
{
  // A software reset puts the controller back in its power-on state (sleep
  // in, display off); it then needs 120 ms before Sleep Out.
  select();
  command(SWRESET);
  deselect();
  delay(120);
  initPanel();
}

void PPLcd147::window(int16_t x, int16_t y, int16_t w, int16_t h)
{
  uint16_t x0 = static_cast<uint16_t>(x + kColOffset);
  uint16_t x1 = static_cast<uint16_t>(x + w - 1 + kColOffset);
  uint16_t y0 = static_cast<uint16_t>(y);
  uint16_t y1 = static_cast<uint16_t>(y + h - 1);
  uint8_t cols[4] = { static_cast<uint8_t>(x0 >> 8), static_cast<uint8_t>(x0),
                      static_cast<uint8_t>(x1 >> 8), static_cast<uint8_t>(x1) };
  uint8_t rows[4] = { static_cast<uint8_t>(y0 >> 8), static_cast<uint8_t>(y0),
                      static_cast<uint8_t>(y1 >> 8), static_cast<uint8_t>(y1) };
  command(CASET, cols, 4);
  command(RASET, rows, 4);
  command(RAMWR);
}

void PPLcd147::fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t colour)
{
  if (x < 0) { w += x; x = 0; }
  if (y < 0) { h += y; y = 0; }
  if (x + w > PP_UI_W) w = PP_UI_W - x;
  if (y + h > PP_UI_H) h = PP_UI_H - y;
  if (w <= 0 || h <= 0 || _asleep) return;
  uint32_t left = static_cast<uint32_t>(w) * static_cast<uint32_t>(h);
  size_t fill = left < kChunkPx ? left : kChunkPx;
  uint16_t c = swap16(colour);
  for (size_t i = 0; i < fill; ++i) gChunk[i] = c;
  select();
  window(x, y, w, h);
  while (left) {
    size_t n = left < kChunkPx ? left : kChunkPx;
    SPI.transfer(gChunk, nullptr, n * 2);
    left -= n;
  }
  deselect();
}

void PPLcd147::blit(int16_t x, int16_t y, int16_t w, int16_t h, const uint16_t* px)
{
  // The layout never draws past the edges; anything that would is dropped
  // rather than clipped.
  if (x < 0 || y < 0 || w <= 0 || h <= 0 || x + w > PP_UI_W || y + h > PP_UI_H || _asleep) return;
  uint32_t left = static_cast<uint32_t>(w) * static_cast<uint32_t>(h);
  select();
  window(x, y, w, h);
  while (left) {
    size_t n = left < kChunkPx ? left : kChunkPx;
    for (size_t i = 0; i < n; ++i) gChunk[i] = swap16(px[i]);
    SPI.transfer(gChunk, nullptr, n * 2);
    px += n;
    left -= n;
  }
  deselect();
}

void PPLcd147::sleep()
{
  if (_asleep) return;
  backlight(0);
  select();
  command(DISPOFF);
  command(SLPIN);
  deselect();
  _sleptAt = millis();
  _asleep = true;
}

void PPLcd147::wake()
{
  if (!_asleep) return;
  // The controller ignores Sleep Out sent less than 120 ms after Sleep In,
  // and stays dark. Wait that out, then start it from scratch rather than
  // trust whatever state it is in.
  uint32_t since = millis() - _sleptAt;
  if (since < 120) delay(120 - since);
  _asleep = false;                     // initPanel's clear must reach the glass
  reinit();
}

void PPLcd147::backlight(uint8_t level)
{
  if (level == 0 || level == 255) {
    if (_blPwm) {
      HwPWM3.removePin(kPinBl);
      HwPWM3.stop();
      HwPWM3.releaseOwnership(kPwmToken);
      _blPwm = false;
    }
    pinMode(kPinBl, OUTPUT);
    digitalWrite(kPinBl, level ? HIGH : LOW);
    return;
  }
  if (!_blPwm) {
    if (!HwPWM3.takeOwnership(kPwmToken)) {
      // Somebody else has the PWM: full brightness rather than none.
      digitalWrite(kPinBl, HIGH);
      return;
    }
    HwPWM3.setResolution(8);
    HwPWM3.addPin(kPinBl);
    HwPWM3.begin();
    _blPwm = true;
  }
  HwPWM3.writePin(kPinBl, level);
}

bool PPLcd147::touchRead(int16_t& x, int16_t& y)
{
  // The interrupt line is a hint only (Seeed's driver polls as well when it
  // stays high), so the chip is also read every kTouchPollMs.
  uint32_t now = millis();
  if (digitalRead(kPinTouchInt) != LOW && !_touchDown && (now - _touchPolledAt) < kTouchPollMs) return false;
  _touchPolledAt = now;
  uint8_t buf[kTouchLen];
  Wire.beginTransmission(kTouchAddr);
  Wire.write(kTouchReg);
  if (Wire.endTransmission() != 0 || Wire.requestFrom(kTouchAddr, kTouchLen) != kTouchLen) {
    _touchDown = false;
    return false;
  }
  for (uint8_t i = 0; i < kTouchLen; ++i) buf[i] = static_cast<uint8_t>(Wire.read());
  if (buf[1] == 0) {
    _touchDown = false;
    return false;
  }
  int16_t rx = static_cast<int16_t>(((buf[2] & 0x0F) << 8) | buf[3]);
  int16_t ry = static_cast<int16_t>(((buf[4] & 0x0F) << 8) | buf[5]);
  if (rx >= PP_UI_W) rx = PP_UI_W - 1;
  if (ry >= PP_UI_H) ry = PP_UI_H - 1;
  x = static_cast<int16_t>(PP_UI_W - 1 - rx);
  y = ry;
  _touchDown = true;
  return true;
}

#endif  // PP_BOARD_DISPLAY147
