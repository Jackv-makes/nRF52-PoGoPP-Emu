#include "pp_catch_engine.h"

#include <Arduino.h>  // random() seeded from the SoftDevice RNG at boot

namespace {

// Written by the NimBLE host task (LED write callback), read by the loop task.
static volatile bool gPressPending = false;
static volatile uint32_t gPressAt = 0;
static bool gReleasePending = false;
static uint32_t gReleaseAt = 0;
static constexpr uint32_t kReleaseAfterMs = 500;

// The last prompt (Pokemon or Pokestop) and when, so a storage-full flash that
// follows can be split into box-full (after a Pokemon) or bag-full (after a
// Pokestop). Window = mallek's PP_FULL_CONTEXT_MS.
static PPLedEvent gLastPrompt = PP_LED_NONE;
static uint32_t gLastPromptAt = 0;
static constexpr uint32_t kContextMs = 20000;

// Exact 3-byte LED-entry families, ported from mallek/nRF52-go-plus-plus-
// backup- (code/nrf_emulator/pgpemu_xiao/pp_feedback_logic.cpp, AGPL). mallek
// identified these from a real field log labelled against the in-game Journal,
// which fixes the warnings pgpemu's colour-count guesses got wrong (notably
// bag-full vs box-full, and an all-white pattern misfiring as bag-full). Each
// entry is {duration (50 ms units), green<<4|red, flags<<4|blue}.
struct LedEntry { uint8_t a, b, c; };
static const LedEntry kShake        = { 0x03, 0x88, 0xF8 };  // opens every encounter result
static const LedEntry kRainbowStart = { 0x03, 0x08, 0xF0 };  // caught: colour cycle after the shake
static const LedEntry kRedBlink     = { 0x01, 0x0F, 0xF0 };  // fled: red blink after the shake
static const LedEntry kCycleStart   = { 0x03, 0xF0, 0x80 };  // Pokestop items
static const LedEntry kStopNearby   = { 0x04, 0x00, 0xFF };  // Pokestop in range
static const LedEntry kEncounter    = { 0x02, 0xCC, 0xF0 };  // Pokemon encounter
static const LedEntry kEncounterAlt = { 0x10, 0xF0, 0xF0 };  // also an encounter
static const LedEntry kStorageFull  = { 0x18, 0x0F, 0xF0 };  // one long flash: box or bag full
static const LedEntry kStopPassed   = { 0x10, 0xFF, 0xFF };  // Pokestop left range without a spin

static bool entryIs(const uint8_t* data, int index, const LedEntry& e)
{
  const uint8_t* p = data + 4 + 3 * index;
  return p[0] == e.a && p[1] == e.b && p[2] == e.c;
}

static bool containsEntry(const uint8_t* data, int count, const LedEntry& e)
{
  for (int i = 0; i < count; ++i) {
    if (entryIs(data, i, e)) return true;
  }
  return false;
}

static void firePress(const CatchEngineRefs& refs)
{
  // Per u/EeveesGalore's GO Plus notes: the button is sampled every 50 ms,
  // byte 0 holds samples 0-1 (bit 1 = sample 0), byte 1 holds samples 2-9
  // (bit 0 = sample 9). Randomise where the press sits and how long it is,
  // keeping it at least 200 ms, exactly as pgpemu does.
  int pressStart = random() % 6;
  int pressLast = pressStart + 4 + random() % (10 - pressStart - 4);
  uint16_t pattern = 0;
  for (int i = 0; i < 10; ++i) {
    pattern <<= 1;
    if (i >= pressStart && i <= pressLast) {
      pattern |= 1;
    }
  }
  pattern &= 0x03FF;
  uint8_t b0 = static_cast<uint8_t>((pattern >> 8) & 0x03);
  uint8_t b1 = static_cast<uint8_t>(pattern & 0xFF);
  Serial.print("Catch engine: press bitmap ");
  Serial.print(b0, HEX);
  Serial.print(" ");
  Serial.print(b1, HEX);
  Serial.print(" duration_ms=");
  Serial.println((pressLast - pressStart + 1) * 50);
  refs.sendButtonState(b0, b1);
  gReleasePending = true;
  gReleaseAt = millis() + kReleaseAfterMs;
}

}  // namespace

const char* ppLedEventLabel(PPLedEvent evt)
{
  switch (evt) {
    case PP_LED_OFF: return "LEDs off";
    case PP_LED_POKEMON: return "Pokemon nearby";
    case PP_LED_NEW_POKEMON: return "New Pokemon!";
    case PP_LED_POKESTOP: return "Pokestop";
    case PP_LED_CAUGHT: return "Caught!";
    case PP_LED_FLED: return "Fled";
    case PP_LED_ITEMS: return "Got items";
    case PP_LED_BAG_FULL: return "Bag full";
    case PP_LED_BOX_FULL: return "Box full";
    case PP_LED_BALLS_EMPTY: return "No balls / out of range";
    case PP_LED_UNKNOWN: return "Unknown pattern";
    case PP_LED_SPIN_FAILED: return "Spin didn't go through";
    default: return "-";
  }
}

void ppCatchReset()
{
  gPressPending = false;
  gReleasePending = false;
  gLastPrompt = PP_LED_NONE;
}

bool ppCatchRequestPress(const CatchEngineRefs& refs, uint32_t delayMs, const char* reason)
{
  if (*refs.pairingStage != PAIR_STAGE_AUTHENTICATED || *refs.activeConnHandle == BLE_CONN_HANDLE_INVALID) {
    Serial.print("Catch engine: press ignored (not authenticated) via ");
    Serial.println(reason);
    return false;
  }
  if (gPressPending) {
    Serial.print("Catch engine: press already pending, dropped via ");
    Serial.println(reason);
    return false;
  }
  gPressAt = millis() + delayMs;  // deadline first, then publish the flag
  gPressPending = true;
  Serial.print("Catch engine: press queued in ");
  Serial.print(delayMs);
  Serial.print(" ms via ");
  Serial.println(reason);
  return true;
}

void ppCatchTick(const CatchEngineRefs& refs)
{
  uint32_t now = millis();
  if (gPressPending && static_cast<int32_t>(now - gPressAt) >= 0) {
    gPressPending = false;
    if (*refs.pairingStage == PAIR_STAGE_AUTHENTICATED && *refs.activeConnHandle != BLE_CONN_HANDLE_INVALID) {
      firePress(refs);
    }
  }
  if (gReleasePending && static_cast<int32_t>(now - gReleaseAt) >= 0) {
    gReleasePending = false;
    if (*refs.activeConnHandle != BLE_CONN_HANDLE_INVALID) {
      refs.sendButtonState(0x00, 0x00);
    }
  }
}

PPLedEvent ppCatchHandleLedWrite(const CatchEngineRefs& refs, const uint8_t* buffer, uint16_t len)
{
  if (len < 4 || buffer[0] || buffer[1] || buffer[2]) {
    return PP_LED_NONE;
  }
  // Count is the low 5 bits of byte 3 (priority is the top 3); entries follow,
  // 3 bytes each.
  int count = buffer[3] & 0x1F;
  if (len < 4 + 3 * count) {
    Serial.print("Catch engine: LED write truncated len=");
    Serial.print(len);
    Serial.print(" count=");
    Serial.println(count);
    return PP_LED_NONE;
  }
  if (count == 0) {
    return PP_LED_OFF;
  }

  // Sum entry durations for the press-timing fit, and note an all-dark write.
  int durationUnits = 0;
  bool allOff = true;
  for (int i = 0; i < count; ++i) {
    const uint8_t* p = &buffer[4 + 3 * i];
    durationUnits += p[0];
    if (p[1] || p[2]) allOff = false;
  }

  // Classify by exact entry families (mallek's field-log decode). isResult
  // marks a result sequence (opened by the shake) so an unrecognised result is
  // not mistaken for a pressable prompt.
  uint32_t now = millis();
  PPLedEvent evt = PP_LED_UNKNOWN;
  bool isResult = false;
  if (allOff) {
    evt = PP_LED_OFF;
  } else if (count == 1) {
    if (entryIs(buffer, 0, kStorageFull)) {
      // One long flash = box/bag full; the recent prompt says which.
      bool freshStop = gLastPrompt == PP_LED_POKESTOP && (now - gLastPromptAt) <= kContextMs;
      evt = freshStop ? PP_LED_BAG_FULL : PP_LED_BOX_FULL;
    } else if (entryIs(buffer, 0, kStopPassed)) {
      evt = PP_LED_SPIN_FAILED;  // Pokestop left range without spinning
    } else {
      evt = PP_LED_UNKNOWN;
    }
  } else if (entryIs(buffer, 0, kShake)) {
    isResult = true;
    if (containsEntry(buffer, count, kRainbowStart)) evt = PP_LED_CAUGHT;
    else if (containsEntry(buffer, count, kRedBlink)) evt = PP_LED_FLED;
    else evt = PP_LED_UNKNOWN;
  } else if (entryIs(buffer, 0, kCycleStart)) {
    evt = PP_LED_ITEMS;
  } else if (entryIs(buffer, 0, kStopNearby)) {
    evt = PP_LED_POKESTOP;
  } else if (entryIs(buffer, 0, kEncounter) || entryIs(buffer, 0, kEncounterAlt)) {
    evt = PP_LED_POKEMON;
  } else {
    evt = PP_LED_UNKNOWN;
  }

  Serial.print("Catch engine: entries=");
  Serial.print(count);
  Serial.print(" first=");
  Serial.print(buffer[4], HEX); Serial.print(' ');
  Serial.print(buffer[5], HEX); Serial.print(' ');
  Serial.print(buffer[6], HEX);
  Serial.print(" total_ms=");
  Serial.print(durationUnits * 50);
  Serial.print(" -> ");
  Serial.println(ppLedEventLabel(evt));

  // Remember the prompt for the box/bag split; clear it once a result lands.
  if (evt == PP_LED_POKEMON || evt == PP_LED_POKESTOP) {
    gLastPrompt = evt;
    gLastPromptAt = now;
  } else if (evt == PP_LED_CAUGHT || evt == PP_LED_FLED || evt == PP_LED_ITEMS) {
    gLastPrompt = PP_LED_NONE;
  }

  // Press decisions. Prompts press when their mode is on. An unrecognised
  // MULTI-entry, non-result pattern still presses in auto (like pgpemu), so a
  // catch is not missed on a prompt we have not catalogued; results, single
  // entries and acks never press.
  bool spin = refs.autoSpin();
  uint8_t catchMode = refs.catchMode();
  bool press = false;
  if (evt == PP_LED_POKEMON) press = (catchMode != 0);
  else if (evt == PP_LED_POKESTOP) press = spin;
  else if (evt == PP_LED_UNKNOWN && !isResult && count > 1) press = spin || (catchMode != 0);

  if (evt != PP_LED_OFF && refs.onEvent) {
    refs.onEvent(evt, ppLedEventLabel(evt));
  }

  if (press) {
    int patternMs = durationUnits * 50;
    // Normal keeps pgpemu's human-like 1.0-2.5 s; Fast answers in 0.3-0.8 s.
    // Fast only applies to recognised prompts; unknowns keep pgpemu's pacing.
    bool fast = refs.pressSpeed && refs.pressSpeed() == 1 && evt != PP_LED_UNKNOWN;
    uint32_t delayMs = fast ? (300 + random() % 501) : (1000 + random() % 1501);
    if (static_cast<int>(delayMs) < patternMs) {
      ppCatchRequestPress(refs, delayMs, ppLedEventLabel(evt));
    } else {
      Serial.print("Catch engine: pattern too short for a press (");
      Serial.print(patternMs);
      Serial.println(" ms)");
    }
  }
  return evt;
}
