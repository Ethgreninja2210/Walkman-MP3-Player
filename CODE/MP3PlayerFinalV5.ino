#include <Arduino.h>
#include <SoftwareSerial.h>
#include <DFRobotDFPlayerMini.h>
#include <U8g2lib.h>
#include <Wire.h>
#include <EEPROM.h>
#include "OneButton.h"

// Icon fonts: U8g2's built-in, verified "Open Iconic" icon sets.
// (A custom font was tried earlier but its binary data gets corrupted by any
// text-based copy/paste, so these official library fonts are used instead —
// no extra files needed, just U8g2 library version ≥2.22.)
//   u8g2_font_open_iconic_play_2x_t -> play(69) pause(68) skip-back(71) skip-fwd(72)

// ── Pin definitions ──────────────────────────────────────────────────────────
const uint8_t leftButtonPin      = 2;   // Previous track (direct skip)
const uint8_t selectionButtonPin = 5;   // Play / Pause
const uint8_t rightButtonPin     = 8;   // Next track (direct skip)
const uint8_t eqButtonPin        = 9;   // Cycle EQ preset
const uint8_t potPin             = A0;  // Potentiometer for volume

// ── State variables ──────────────────────────────────────────────────────────
volatile bool updateScreen = true;

uint8_t filecounts;
uint8_t volume = 20;
uint8_t file   = 1;
uint8_t eq     = 0;

// Potentiometer smoothing
int     lastPotValue   = -1;
uint8_t lastVolumeSent  = 255;   // force a send on first read

boolean playing = false;

OneButton PreviousBTN(leftButtonPin,      true);
OneButton PlayBTN    (selectionButtonPin, true);
OneButton NextBTN    (rightButtonPin,     true);
OneButton EqBTN      (eqButtonPin,        true);

// SoftwareSerial(RX, TX) — D12 carries data FROM the DFPlayer's TX pin (so D12
// is the Nano's RX), and D13 carries data TO the DFPlayer's RX pin (so D13 is
// the Nano's TX) — matches the wiring diagram.
SoftwareSerial customSoftwareSerial(12, 13);
DFRobotDFPlayerMini myDFPlayer;

U8G2_SH1106_128X64_NONAME_1_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE);

// ── Boot animation ────────────────────────────────────────────────────────────
const uint8_t  STARTUP_MAX_BLOCKS = 40;
const uint8_t  STARTUP_BLOCK_SIZE = 10;
const uint16_t STARTUP_FRAME_MS   = 40;

uint8_t blockX[STARTUP_MAX_BLOCKS];
uint8_t blockY[STARTUP_MAX_BLOCKS];

void bootAnimation() {
  uint8_t blocksSoFar = 0;

  for (uint8_t frame = 0; frame <= STARTUP_MAX_BLOCKS; frame++) {
    if (frame < STARTUP_MAX_BLOCKS) {
      blockX[blocksSoFar] = random(0, u8g2.getDisplayWidth()  - STARTUP_BLOCK_SIZE);
      blockY[blocksSoFar] = random(0, u8g2.getDisplayHeight() - STARTUP_BLOCK_SIZE);
      blocksSoFar++;
    }

    u8g2.firstPage();
    do {
      if (frame < STARTUP_MAX_BLOCKS) {
        for (uint8_t i = 0; i < blocksSoFar; i++) {
          u8g2.drawBox(blockX[i], blockY[i], STARTUP_BLOCK_SIZE, STARTUP_BLOCK_SIZE);
        }
      }
    } while (u8g2.nextPage());

    delay(STARTUP_FRAME_MS);
  }
}

// ── Setup ────────────────────────────────────────────────────────────────────
void setup(void) {
  Serial.begin(9600);
  randomSeed(analogRead(A7));   // unused pin, just for boot-animation entropy
  u8g2.begin();
  bootAnimation();

  PreviousBTN.attachClick(previousButtonClicked);
  PlayBTN    .attachClick(playButtonClicked);
  NextBTN    .attachClick(nextButtonClicked);
  EqBTN      .attachClick(eqButtonClicked);

  PreviousBTN.setDebounceMs(50);
  PlayBTN    .setDebounceMs(50);
  NextBTN    .setDebounceMs(50);
  EqBTN      .setDebounceMs(50);

  customSoftwareSerial.begin(9600);
  if (!myDFPlayer.begin(customSoftwareSerial)) {
    Serial.println(F("Please insert the SD card!"));
  }

  // Restore EQ and last-played file from EEPROM (volume is hardware-controlled)
  eq   = EEPROM.read(1);  if (eq   > 5)   eq   = 5;
  file = EEPROM.read(2);  if (file >= 19) file = 1;

  delay(1000);

  // Read potentiometer immediately 
  volume = map(analogRead(potPin), 0, 1023, 0, 30);
  myDFPlayer.volume(volume);
  lastVolumeSent = volume;

  delay(500);
  startInitialPlay();
}

// ── Main loop ────────────────────────────────────────────────────────────────
void loop() {
  PreviousBTN.tick();
  PlayBTN    .tick();
  NextBTN    .tick();
  EqBTN      .tick();

  updateVolume();
  updateDisplay();
  updateDFplayer();
}

// ── Potentiometer volume ─────────────────────────────────────────────────────
// Only sends a new volume command when the pot moves enough to change the
// 0-30 DFPlayer volume level, so the serial line isn't flooded.
void updateVolume() {
  int raw = analogRead(potPin);
  if (abs(raw - lastPotValue) >= 4) {
    lastPotValue = raw;
    uint8_t newVol = (uint8_t) map(raw, 0, 1023, 0, 30);
    if (newVol != lastVolumeSent) {
      volume         = newVol;
      lastVolumeSent = newVol;
      myDFPlayer.volume(volume);
      updateScreen = true;   // refresh the volume bar
    }
  }
}

// ── Button callbacks ─────────────────────────────────────────────────────────
void previousButtonClicked() {
  myDFPlayer.previous();
  file--;
  playing = true;
  EEPROM.write(2, file);
  updateScreen = true;
}

void nextButtonClicked() {
  file++;
  myDFPlayer.next();
  playing = true;
  EEPROM.write(2, file);
  updateScreen = true;
}

void playButtonClicked() {
  if (playing) myDFPlayer.pause();
  else         myDFPlayer.start();
  playing = !playing;
  updateScreen = true;
}

// 4th button cycles through EQ presets (Normal/Pop/Rock/Jazz/Classic/Bass).
void eqButtonClicked() {
  eq = (eq + 1) % 6;
  myDFPlayer.EQ(eq);
  EEPROM.write(1, eq);
  updateScreen = true;
}

// ── DFPlayer helpers ─────────────────────────────────────────────────────────
void startInitialPlay() {
  filecounts = myDFPlayer.readFileCounts();
  myDFPlayer.play(file);
  playing = false;
}

void updateDFplayer() {
  if (myDFPlayer.available()) {
    uint8_t type  = myDFPlayer.readType();
    int     value = myDFPlayer.read();
    if (type == DFPlayerPlayFinished) {
      if (file < filecounts) {
        file++;
        myDFPlayer.play(file);
        EEPROM.write(2, file);
        updateScreen = true;
      }
    }
  }
}

// EQ preset single-letter label (top-right indicator)
char eqLetter(uint8_t e) {
  switch (e) {
    case 0: return 'N';   // Normal
    case 1: return 'P';   // Pop
    case 2: return 'R';   // Rock
    case 3: return 'J';   // Jazz
    case 4: return 'C';   // Classic
    case 5: return 'B';   // Bass
    default: return '?';
  }
}

// ── Display update ────────────────────────────────────────────────────────────
void updateDisplay() {
  if (updateScreen) {
    u8g2.firstPage();
    do {
      player();
      updateScreen = false;
    } while (u8g2.nextPage());
  }
}

// ── Volume bar (left edge, visual only) ──────────────────────────────────────
// potentiometer's current 0-30 mapped level.
void volumeBar() {
  const u8g2_uint_t barX      = 3;
  const u8g2_uint_t barY      = 4;
  const u8g2_uint_t barW      = 10;
  const u8g2_uint_t barH      = 56;     // total track height
  const u8g2_uint_t barBottom = barY + barH;

  // Outline
  u8g2.drawFrame(barX, barY, barW, barH);

  // Fill proportional to volume (0-30)
  u8g2_uint_t fillH = (u8g2_uint_t)((uint32_t)barH * volume / 30);
  if (fillH > barH - 2) fillH = barH - 2;   // keep inside the outline
  if (fillH > 0) {
    u8g2.drawBox(barX + 1, barBottom - 1 - fillH, barW - 2, fillH);
  }

  u8g2.drawVLine(barX + barW + 4, 0, 64);   // divider between bar and main UI
}

// ── EQ indicator (top-right) ─────────────────────────────────────────────────
void eqIndicator() {
  u8g2.setFont(u8g2_font_logisoso16_tf);
  char letter[2] = { eqLetter(eq), '\0' };
  u8g2.setCursor(112, 16);
  u8g2.print(letter);
}

// ── Player screen (single screen, no settings page) ──────────────────────────
void player() {
  volumeBar();
  eqIndicator();

  u8g2_uint_t midOriginX = 70, midOriginY = 44;

  // Track info, centred over the button row
  u8g2.setFontMode(0);
  u8g2.setFont(u8g2_font_glasstown_nbp_tf);
  char trackStr[12];
  snprintf(trackStr, sizeof(trackStr), "%d/%d", file, filecounts);
  uint8_t strWidth = u8g2.getStrWidth(trackStr);
  u8g2.setCursor(midOriginX - (strWidth / 2), 25);
  u8g2.print(trackStr);

  // Previous (skip-backward)
  u8g2.setFont(u8g2_font_open_iconic_play_2x_t);
  u8g2.drawGlyph(midOriginX - 32, midOriginY + 8, 71);

  // Play / Pause — centred between prev and next
  if (playing) u8g2.drawGlyph(midOriginX - 8, midOriginY + 8, 68);   // pause
  else         u8g2.drawGlyph(midOriginX - 8, midOriginY + 8, 69);   // play

  // Next (skip-forward)
  u8g2.drawGlyph(midOriginX + 16, midOriginY + 8, 72);
}
