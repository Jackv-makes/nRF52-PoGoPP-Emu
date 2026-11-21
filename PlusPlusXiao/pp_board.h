#pragma once
// Board support. Three boards share this interface; the build picks one:
//   default            XIAO nRF52840 carrier (pp_board.cpp), described below
//   -DPP_BOARD_BARE    XIAO nRF52840 with no carrier (pp_board.cpp too), see
//                      "Bare XIAO" below
//   -DPP_BOARD_CLONE   PGP+ CLONE v0.1, nRF52832 on the stock Plus+ pinout
//                      (pp_board_clone.cpp, controls described there)
//   -DPP_BOARD_GOPLUS  GO Plus-shaped board, u-blox ANNA-B402 (nRF52833)
//                      (pp_board_goplus.cpp, controls described there)
//   -DPP_BOARD_DISPLAY147  Seeed XIAO 1.47" IPS touch display, nRF52840 Plus
//                      (pp_board_display147.cpp, controls described there)
//
// XIAO nRF52840 carrier board support: three buttons, one status LED, no
// screen. Settings live in InternalFS.
//
//   SW1 catch  (D6)  click      -> manual catch/spin press (when connected)
//   SW2 btn 1  (D5)  click      -> press speed Normal/Fast (LED: 1 / 2 blinks)
//   SW2 btn 1  (D5)  hold 5 s   -> wipe pairing (pp_app_lifecycle, raw pin)
//   SW3 btn 2  (D7)  click      -> catch mode Off/All/New only (LED: 1 long / 2 / 3 blinks)
//   SW3 btn 2  (D7)  hold 1.5 s -> auto spin on/off (LED: 2 long / 1 long)
//
// LED1 (D4, active high through R1):
//   pairable, not connected   slow single blink
//   connected, not authed     fast blink
//   connected + authenticated off, flashes on events (press, caught, fled,
//                             items, warnings)
//
// Bare XIAO (-DPP_BOARD_BARE): the same button pins with their pull-ups, so a
// wire from D6/D5/D7 to GND still works as that button, and nothing is driven
// on D4. The XIAO's own RGB LED (active low) takes over from LED1 with the
// original Plus+'s colours: white blink until connected and authenticated,
// green = Pokemon nearby (press coming) and press sent, yellow = new
// Pokemon, blue = Pokestop and items, red-green-blue cycle = caught, red =
// fled and warnings, white = boot and setting changed.
// The USB console stands in for the buttons (115200 baud, no Enter needed):
//   p press   c catch mode   s auto spin   f press speed   a advertise fast
//   i status  W wipe pairing (capital W)   ? help
// On USB power the board stays out of System OFF, because only the reset
// button could wake it. Pressing RESET twice about a second apart wipes the
// pairing (a fast double tap is the bootloader's boot drive instead).

#include <Arduino.h>
#include <stdint.h>

#include <bluefruit.h>
#include "pp_types.h"
#include "pp_catch_engine.h"

#ifdef PP_BOARD_CLONE
// PGP+ CLONE v0.1: Arduino pin N = P0.N (firmware-nrf52/clone/variant), so
// the Arduino and the raw numbers are the same. Two buttons only: BTN2 is an
// alias of BTN1, which keeps the three-entry wake list in pp_app_lifecycle.
static constexpr uint8_t PP_BOARD_PIN_LED = 20;    // P0.20 LED_R (RGB LED1 red)
static constexpr uint8_t PP_BOARD_PIN_BTN1 = 22;   // P0.22 BTN_TOP  (SW2)
static constexpr uint8_t PP_BOARD_PIN_CATCH = 25;  // P0.25 BTN_MAIN (SW1)
static constexpr uint8_t PP_BOARD_PIN_BTN2 = PP_BOARD_PIN_BTN1;
static constexpr uint32_t PP_BOARD_PIN_BTN1_RAW = 22;
static constexpr uint32_t PP_BOARD_PIN_CATCH_RAW = 25;
static constexpr uint32_t PP_BOARD_PIN_BTN2_RAW = PP_BOARD_PIN_BTN1_RAW;
// Radio transmit power in dBm: +4 is the most the nRF52832 does (S132
// accepts -40..0, +3, +4; a higher value is refused and leaves 0). The chip
// antenna's match is untuned, so the board sends at full power.
static constexpr int8_t PP_BOARD_TX_POWER_DBM = 4;
// The clone board has no inductor on DCC, so REG1 stays on its LDO.
static constexpr bool PP_BOARD_DCDC_REG1 = false;
#elif defined(PP_BOARD_GOPLUS)
// GO Plus-shaped board (hardware/goplus/board v0.8): Arduino pin N is the
// raw nrf_gpio pin (goplus/variant). One button, so BTN1 and BTN2 are both
// the catch button: holding it 5 s wipes the pairing (pp_app_lifecycle) and
// it is the button the shared System OFF code arms for wake-up.
static constexpr uint8_t PP_BOARD_PIN_LED = LED_RED;
static constexpr uint8_t PP_BOARD_PIN_CATCH = 3;   // P0.03 CATCH
static constexpr uint8_t PP_BOARD_PIN_BTN1 = PP_BOARD_PIN_CATCH;
static constexpr uint8_t PP_BOARD_PIN_BTN2 = PP_BOARD_PIN_CATCH;
static constexpr uint32_t PP_BOARD_PIN_CATCH_RAW = 3;
static constexpr uint32_t PP_BOARD_PIN_BTN1_RAW = PP_BOARD_PIN_CATCH_RAW;
static constexpr uint32_t PP_BOARD_PIN_BTN2_RAW = PP_BOARD_PIN_CATCH_RAW;
// The nRF52833 sends at up to +8 dBm, the module's rated maximum.
static constexpr int8_t PP_BOARD_TX_POWER_DBM = 8;
// The ANNA-B402 has the DC-DC regulator's parts inside the module (data
// sheet: "An integrated DC/DC converter").
static constexpr bool PP_BOARD_DCDC_REG1 = true;
// What the slide switch means. Lever up (MODE low) and lever down (MODE
// high). Up = auto catch and spin with no vibration, down = manual with
// vibration. Change these six lines to give the positions other jobs.
static constexpr uint8_t PP_GOPLUS_LEVER_UP_CATCH = 1;      // PP_CATCH_ALL
static constexpr bool PP_GOPLUS_LEVER_UP_SPIN = true;
static constexpr bool PP_GOPLUS_LEVER_UP_VIBRATE = false;
static constexpr uint8_t PP_GOPLUS_LEVER_DOWN_CATCH = 0;    // PP_CATCH_OFF
static constexpr bool PP_GOPLUS_LEVER_DOWN_SPIN = false;
static constexpr bool PP_GOPLUS_LEVER_DOWN_VIBRATE = true;
#elif defined(PP_BOARD_DISPLAY147)
// Seeed XIAO 1.47" display (XIAO nRF52840 Plus variant; its D15..D19 are
// Arduino pins 34..36 and their raw pins come from the variant's map).
// Button A (USR1) is the catch button and, held 5 s, wipes the pairing;
// button B (USR2) is the mode button. B sits on P0.10, an NFC pin, so the
// build sets CONFIG_NFCT_PINS_AS_GPIOS (build.sh display147).
static constexpr uint8_t PP_BOARD_PIN_LED = LED_RED;   // unused, kept off
static constexpr uint8_t PP_BOARD_PIN_CATCH = D19;     // P1.07 BTN_A
static constexpr uint8_t PP_BOARD_PIN_BTN1 = PP_BOARD_PIN_CATCH;
static constexpr uint8_t PP_BOARD_PIN_BTN2 = D15;      // P0.10 BTN_B
static constexpr uint32_t PP_BOARD_PIN_CATCH_RAW = 39; // P1.07
static constexpr uint32_t PP_BOARD_PIN_BTN1_RAW = PP_BOARD_PIN_CATCH_RAW;
static constexpr uint32_t PP_BOARD_PIN_BTN2_RAW = 10;  // P0.10
static constexpr int8_t PP_BOARD_TX_POWER_DBM = 8;
// Whether the Plus module has the REG1 DC-DC inductor is not confirmed, and
// switching to DC-DC without it stops the chip, so this build keeps the LDO.
static constexpr bool PP_BOARD_DCDC_REG1 = false;
#else
// Arduino pin numbers (Seeed XIAO nRF52840 variant).
static constexpr uint8_t PP_BOARD_PIN_LED = 4;     // D4 = P0.04
static constexpr uint8_t PP_BOARD_PIN_BTN1 = 5;    // D5 = P0.05  (SW2)
static constexpr uint8_t PP_BOARD_PIN_CATCH = 6;   // D6 = P1.11  (SW1)
static constexpr uint8_t PP_BOARD_PIN_BTN2 = 7;    // D7 = P1.12  (SW3)
// Raw nRF pin numbers for the nrf_gpio_* calls in pp_app_lifecycle.
static constexpr uint32_t PP_BOARD_PIN_BTN1_RAW = 5;    // P0.05
static constexpr uint32_t PP_BOARD_PIN_CATCH_RAW = 43;  // P1.11
static constexpr uint32_t PP_BOARD_PIN_BTN2_RAW = 44;   // P1.12
// Radio transmit power in dBm. The Bluefruit default of 0 gave a short range
// on the XIAO's chip antenna; +8 is the most the nRF52840 does.
static constexpr int8_t PP_BOARD_TX_POWER_DBM = 8;
// The XIAO nRF52840 (Sense too) has the REG1 DC-DC parts fitted (10 uH on
// DCC, Seeed schematic v1.1), so the build switches the regulator from LDO
// to DC-DC after the SoftDevice is up. Lower radio current; gain unmeasured.
static constexpr bool PP_BOARD_DCDC_REG1 = true;
#endif

// RGB status LED. Defined for the builds whose status LED has three colours;
// PP_BOARD_HAS_RGB turns the colour map in pp_board.cpp on. A single-LED
// carrier build leaves all of this undefined and drives PP_BOARD_PIN_LED.
#if defined(PP_BOARD_BARE)
  #define PP_BOARD_HAS_RGB 1
  // The XIAO's own on-module RGB LED, common anode / active low.
  static constexpr uint8_t PP_RGB_PIN_R = LED_RED;
  static constexpr uint8_t PP_RGB_PIN_G = LED_GREEN;
  static constexpr uint8_t PP_RGB_PIN_B = LED_BLUE;
  static constexpr int PP_RGB_ON = LOW;
  static constexpr int PP_RGB_OFF = HIGH;
#elif defined(PP_BOARD_CARRIER_RGB)
  #define PP_BOARD_HAS_RGB 1
  // Carrier v0.4 front LED: Kingbright AAA3528 PLCC-4, common anode (anode on
  // 3V3, each cathode pulled low to light). The board routes red D3, green
  // D10, blue D2 (hardware/carrier, rgb_led.py). Change these three lines if
  // the board's lanes change.
  static constexpr uint8_t PP_RGB_PIN_R = 3;    // D3  = P0.29
  static constexpr uint8_t PP_RGB_PIN_G = 10;   // D10 = P1.15
  static constexpr uint8_t PP_RGB_PIN_B = 2;    // D2  = P0.28
  static constexpr int PP_RGB_ON = LOW;
  static constexpr int PP_RGB_OFF = HIGH;
#endif

enum PPCatchMode : uint8_t {
  PP_CATCH_OFF = 0,
  PP_CATCH_ALL = 1,
  PP_CATCH_NEW_ONLY = 2,
};

struct BoardRefs {
  uint16_t* activeConnHandle;
  PairingStage* pairingStage;
  bool* persistentReconnectReady;
  const char* buildTag;
  const uint8_t* donorAddrLe;
  void (*manualButtonPress)();
  void (*startAdvertising)();
  void (*stopAdvertising)();
  void (*resetPairing)(const char* reason);
  const char* (*pairingStageLabel)(PairingStage stage);
  void (*armDisconnectedSleepTimer)(const char* reason);
};

void ppBoardInit();
// Called once the USB console is up: prints the restored settings and starts
// the boot blink (ppBoardInit runs before Serial.begin).
void ppBoardLogSettings();
// Called right before System OFF: LED off so it cannot stay latched on.
void ppBoardPrepareSystemOff();
void ppBoardTick(const BoardRefs& refs);
// Start-up cannot continue (the Bluetooth stack did not start): reports it on
// the console and the LED. Does not return.
void ppBoardFatal(const char* what);

// Settings read by the catch engine and the advertising wrapper.
bool ppBoardAutoSpin();
uint8_t ppBoardCatchMode();

// Press speed: delay between the app's prompt and the emulated button press.
// Normal = 1.0-2.5 s (pgpemu's human-like default), Fast = 0.3-0.8 s.
enum PPPressSpeed : uint8_t { PP_SPEED_NORMAL = 0, PP_SPEED_FAST = 1 };
uint8_t ppBoardPressSpeed();
bool ppBoardAdvertiseEnabled();
// Whether holding the catch button wipes the pairing. A weak default in
// pp_app_lifecycle.cpp says yes; the display build says no while locked.
bool ppBoardResetHoldAllowed();

// Events from the catch engine: counters and LED flashes.
void ppBoardOnEvent(PPLedEvent evt, const char* label);
void ppBoardSetNote(const char* text);
// Link dropped: remembers the BLE reason.
void ppBoardOnDisconnect(uint8_t reason);
