#pragma once
// LCD, backlight and touch of the Seeed XIAO 1.47" IPS touch display
// (nRF52840 Plus). Used by pp_board_display147.cpp only.
//
// Panel: 172 x 320 IPS on a JD9853A controller, SPI, portrait. The register
// values are the ones Seeed's own library sends this panel (Seeed_GFX2
// v1.0.0, read 2026-11-18): its ST7789-compatible start-up, then inversion
// off, MADCTL 0x48 (column mirror, BGR) and a 34-column offset into the
// controller's 240-column RAM. The driver code here is ours.
//
// Touch: AXS5106L at I2C 0x63 on D4/D5, interrupt on D7 (low while
// touched). It shares the LCD's reset line, so begin() resets both, holding
// the line low 200 ms and waiting 300 ms after, as the touch chip needs.
// A report is 14 bytes from register 0x01: [1] = points, then X and Y as
// 12-bit values (high nibble + low byte) from [2]..[5]. X is mirrored to
// match the screen.

#include <Arduino.h>
#include "pp_ui147.h"

class PPLcd147 : public PPCanvas {
public:
  // Resets the panel and the touch chip, starts both, clears the glass to
  // black. The backlight stays off.
  void begin();
  void fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t colour) override;
  void blit(int16_t x, int16_t y, int16_t w, int16_t h, const uint16_t* px) override;

  // Panel sleep (backlight off first) and wake. wake() restarts the
  // controller from a software reset and clears it to black: about 270 ms.
  void sleep();
  void wake();
  // Restarts the controller (software reset and the start-up values) and
  // clears it to black, awake or not.
  void reinit();
  bool asleep() const { return _asleep; }

  // 0 = off, 255 = full; anything between runs the PWM.
  void backlight(uint8_t level);

  // Reads the touch chip when its interrupt line says touched, while a
  // finger is down, and every 40 ms otherwise. true while a finger is down,
  // with its position.
  bool touchRead(int16_t& x, int16_t& y);

private:
  void command(uint8_t cmd);
  void data(const uint8_t* bytes, size_t n);
  void command(uint8_t cmd, const uint8_t* bytes, size_t n);
  void window(int16_t x, int16_t y, int16_t w, int16_t h);
  void select();
  void deselect();
  void initPanel();

  bool _asleep = false;
  uint32_t _sleptAt = 0;
  bool _touchDown = false;
  uint32_t _touchPolledAt = 0;
  bool _blPwm = false;
};
