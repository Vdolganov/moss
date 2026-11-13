#ifndef MOSS_LAB_UI_H
#define MOSS_LAB_UI_H
#include "lab_core.h"
#include "lab_sample.h"
#include <stdio.h>
#include <ctype.h>
namespace lab {
constexpr uint32_t BG = 0x191a1c, PANEL = 0x222326, LINE = 0x414348, TEXT = 0xe8e8e6,
                   DIM = 0xa9abb0;
static const uint32_t slots[4] = {0xa7ba91, 0x91aebb, 0xaca0ba, 0xc69b83};
// Compact original 5x7 diagnostic glyphs. Native instrument typography remains a separate task.
inline const char *glyph(char c) {
  switch (c) {
  case 'A':
    return "01110100011000111111100011000110001";
  case 'B':
    return "11110100011000111110100011000111110";
  case 'C':
    return "01111100001000010000100001000001111";
  case 'D':
    return "11110100011000110001100011000111110";
  case 'E':
    return "11111100001000011110100001000011111";
  case 'F':
    return "11111100001000011110100001000010000";
  case 'G':
    return "01111100001000010111100011000101111";
  case 'H':
    return "10001100011000111111100011000110001";
  case 'I':
    return "11111001000010000100001000010011111";
  case 'J':
    return "00111000100001000010100101001001100";
  case 'K':
    return "10001100101010011000101001001010001";
  case 'L':
    return "10000100001000010000100001000011111";
  case 'M':
    return "10001110111010110101100011000110001";
  case 'N':
    return "10001110011010110011100011000110001";
  case 'O':
    return "01110100011000110001100011000101110";
  case 'P':
    return "11110100011000111110100001000010000";
  case 'Q':
    return "01110100011000110001101011001001101";
  case 'R':
    return "11110100011000111110101001001010001";
  case 'S':
    return "01111100001000001110000010000111110";
  case 'T':
    return "11111001000010000100001000010000100";
  case 'U':
    return "10001100011000110001100011000101110";
  case 'V':
    return "10001100011000110001100010101000100";
  case 'W':
    return "10001100011000110101101011010101010";
  case 'X':
    return "10001100010101000100010101000110001";
  case 'Y':
    return "10001100010101000100001000010000100";
  case 'Z':
    return "11111000010001000100010001000011111";
  case '0':
    return "01110100011001110101110011000101110";
  case '1':
    return "00100011000010000100001000010001110";
  case '2':
    return "01110100010000100010001000100011111";
  case '3':
    return "11110000010000101110000010000111110";
  case '4':
    return "00010001100101010010111110001000010";
  case '5':
    return "11111100001000011110000010000111110";
  case '6':
    return "01110100001000011110100011000101110";
  case '7':
    return "11111000010001000100010000100001000";
  case '8':
    return "01110100011000101110100011000101110";
  case '9':
    return "01110100011000101111000010000101110";
  case ':':
    return "00000001000010000000001000010000000";
  case '.':
    return "00000000000000000000000000011000110";
  case '/':
    return "00001000100001000100010000100010000";
  case '-':
    return "00000000000000011111000000000000000";
  case '+':
    return "00000001000010011111001000010000000";
  case '[':
    return "01110010000100001000010000100001110";
  case ']':
    return "01110000100001000010000100001001110";
  case '>':
    return "10000010000010000010001000100010000";
  case '<':
    return "00001000100010001000001000001000001";
  case '=':
    return "00000000001111100000111110000000000";
  case '%':
    return "11001110100001000100010001011010011";
  case '_':
    return "00000000000000000000000000000011111";
  case '#':
    return "01010010101111101010111110101001010";
  case '?':
    return "01110100010000100010001000000000100";
  default:
    return "00000000000000000000000000000000000";
  }
}
inline void text(Canvas &c, int x, int y, const char *s, uint32_t color = TEXT, int scale = 2,
                 int maxChars = 80) {
  for (int n = 0; s[n] && n < maxChars; n++) {
    const char *g = glyph(char(toupper((unsigned char)s[n])));
    for (int row = 0; row < 7; row++)
      for (int col = 0; col < 5; col++)
        if (g[row * 5 + col] == '1')
          c.rect(x + n * 6 * scale + col * scale, y + row * scale, scale, scale, color);
  }
}
struct View {
  unsigned liveSeconds = 0, rawCount = 0;
  int page = 0;
  Buttons buttons;
  HoldStats holds;
  int values[4] = {50, 50, 50, 50};
  char inputBackend[64] = "NOT OPEN";
  char lastEvent[64] = "PRESS EACH BUTTON AND RELEASE";
  int rate = 44100, buffer = 1024, mode = 0, hz = 440;
  bool audioRunning = false, audioOpened = false;
  int obtainedRate = 0, obtainedFrames = 0;
  char audioDriver[32] = "NOT OPEN";
  char audioStatus[80] = "START: PLAY A QUIET TEN SECOND TONE";
  char verdict[32] = "NOT LISTENED";
  unsigned callbacks = 0, elapsedMs = 0;
  uint64_t callbackMaxUs = 0, late = 0;
  bool motion = false;
  unsigned tick = 0;
  double fps = 0;
  int bpp = 0;
  char videoDriver[32] = "NOT OPEN";
  long totalKb = -1, availableKb = -1, rssKb = -1;
  char machine[40] = "UNKNOWN";
  char kernel[80] = "UNKNOWN";
  char writeStatus[64] = "A: RUN 256 KIB WRITE / READ TEST";
  char reportStatus[64] = "SELECT: SAVE REPORT";
  int sampleIndex = 0, samplePitch = 0, sampleStart = 0, sampleEnd = 100, sampleMode = 0;
  bool sampleLoaded = false;
  unsigned sampleRate = 0, sampleChannels = 0, sampleFrames = 0, sampleBytes = 0;
  unsigned samplePosition = 0, sampleLoadMs = 0;
  int samplePeaks[64] = {};
  bool preview = false;
  bool missingInput = false;
};
inline void field(Canvas &c, const View &v, int col, const char *label, const char *value,
                  const char *unit = "", const char *limit = "") {
  int x = 18 + col * 153;
  char b[32];
  bool held = (v.buttons.held & bit(A + col)) != 0;
  snprintf(b, sizeof(b), "%s %s%s%s", names[A + col], label, *limit ? " " : "", limit);
  if (held)
    c.box(x, 346, 142, 89, slots[col]);
  c.rect(x, 346, 142, held ? 3 : 1, held ? slots[col] : LINE);
  text(c, x + 4, 357, b, slots[col], 2, 11);
  text(c, x + 4, 385, value, TEXT, 3, 7);
  if (held)
    snprintf(b, sizeof(b), "HOLD %.1fS", v.holds.liveMs[A + col] / 1000.0);
  text(c, x + 4, 416, held ? b : unit, held ? slots[col] : DIM, 2, 11);
}
inline void render(Canvas &c, const View &v) {
  char b[128];
  c.clear(BG);
  text(c, 18, 14, "MOSS LAB", TEXT, 2);
  text(c, 390, 14, v.preview ? "PC PREVIEW" : "0.1.3 / RG35XX", DIM, 2);
  c.rect(0, 41, W, 1, LINE);
  const char *tabs[] = {"INPUT", "AUDIO", "DISPLAY", "SYSTEM", "SAMPLE"};
  text(c, 18, 59, "[", DIM);
  for (int i = 0; i < 5; i++) {
    text(c, 54 + i * 112, 59, tabs[i], i == v.page ? TEXT : DIM);
    if (i == v.page)
      c.rect(50 + i * 112, 81, 96, 2, TEXT);
  }
  text(c, 610, 59, "]", DIM);
  c.rect(0, 84, W, 1, LINE);
  if (v.page == 0) {
    snprintf(b, sizeof(b), "BUTTONS %02d/15   CHORDS %02u/16   L1+R1 %s", v.buttons.completed(),
             popcount(v.buttons.chords), v.buttons.shoulders ? "OK" : "--");
    text(c, 18, 102, b, DIM, 2, 50);
    for (int i = 0; i < ButtonCount; i++) {
      int x = 18 + (i % 5) * 122, y = 133 + (i / 5) * 46;
      bool down = (v.buttons.held & bit(i)) != 0, done = (v.buttons.released & bit(i)) != 0;
      c.rect(x, y, 112, 36, down ? TEXT : PANEL);
      text(c, x + 8, y + 11, names[i], down ? BG : TEXT);
      if (done)
        c.rect(x + 101, y + 27, 5, 2, down ? BG : DIM);
    }
    if (v.buttons.held) {
      strcpy(b, "HELD");
      unsigned longest = 0;
      for (int button = 0; button < ButtonCount; button++)
        if (v.buttons.held & bit(button)) {
          size_t used = strlen(b);
          snprintf(b + used, sizeof(b) - used, " %s", names[button]);
          if (v.holds.liveMs[button] > longest)
            longest = v.holds.liveMs[button];
        }
      size_t used = strlen(b);
      snprintf(b + used, sizeof(b) - used, " / %.1fS", longest / 1000.0);
      text(c, 18, 286, b, TEXT, 2, 50);
    } else
      text(c, 18, 286, v.lastEvent, DIM, 2, 50);
    text(c, 18, 313, "HOLD A/B/X/Y + D-PAD TO CHANGE A VALUE", DIM, 2, 50);
    for (int i = 0; i < 4; i++) {
      snprintf(b, sizeof(b), "%03d", v.values[i]);
      field(c, v, i, "VALUE", b, "0-100",
            v.values[i] == 100 ? "MAX"
            : v.values[i] == 0 ? "MIN"
                               : "");
    }
  } else if (v.page == 1) {
    text(c, 18, 104, v.audioStatus, TEXT, 2, 50);
    snprintf(b, sizeof(b), "DRIVER %s / %s", v.audioDriver, v.audioRunning ? "PLAYING" : "STOPPED");
    text(c, 18, 139, b, DIM, 2, 50);
    snprintf(b, sizeof(b), "ACTUAL %d HZ / %d FRAMES / S16 STEREO", v.obtainedRate,
             v.obtainedFrames);
    text(c, 18, 167, b, DIM, 2, 50);
    snprintf(b, sizeof(b), "CALLBACKS %u / MAX %llu US", v.callbacks,
             (unsigned long long)v.callbackMaxUs);
    text(c, 18, 195, b, TEXT, 2, 50);
    snprintf(b, sizeof(b), "LATE INTERVALS %llu / NOT AN XRUN COUNT", (unsigned long long)v.late);
    text(c, 18, 223, b, DIM, 2, 50);
    snprintf(b, sizeof(b), "HEARING: %s", v.verdict);
    text(c, 18, 259, b, TEXT, 2, 50);
    text(c, 18, 287, "A HEARD / B SILENT / X CLICKS / Y RESET", DIM, 2, 50);
    c.rect(18, 323, 604, 3, LINE);
    c.rect(18, 323, int(604 * (v.elapsedMs > 10000 ? 10000 : v.elapsedMs) / 10000), 3, TEXT);
    snprintf(b, sizeof(b), "%.1f", v.rate / 1000.0);
    field(c, v, 0, "RATE", b, "KHZ", v.rate == 48000 ? "MAX" : "MIN");
    snprintf(b, sizeof(b), "%d", v.buffer);
    field(c, v, 1, "BUF", b, "FRAMES", v.buffer == 2048 ? "MAX" : v.buffer == 256 ? "MIN" : "");
    const char *modes[] = {"BOTH", "LEFT", "RIGHT", "ALT"};
    field(c, v, 2, "OUT", modes[v.mode], "CHANNEL", v.mode == 3 ? "MAX" : v.mode == 0 ? "MIN" : "");
    snprintf(b, sizeof(b), "%d", v.hz);
    field(c, v, 3, "TONE", b, "HZ / -26DB", v.hz == 880 ? "MAX" : v.hz == 220 ? "MIN" : "");
  } else if (v.page == 2) {
    snprintf(b, sizeof(b), "640 X 480 / %d BPP / %s", v.bpp, v.videoDriver);
    text(c, 18, 104, b, DIM, 2, 50);
    c.box(18, 133, 604, 190, TEXT);
    for (int x = 38; x < 602; x += 40)
      c.rect(x, 151, 1, 154, LINE);
    for (int y = 151; y < 306; y += 30)
      c.rect(38, y, 564, 1, LINE);
    int x = v.motion ? 38 + int(v.tick % 540) : 300;
    c.rect(x, 155, 20, 146, TEXT);
    text(c, 45, 339, "START: MOTION / CHECK BORDER AND TEARING", DIM, 2, 47);
    for (int i = 0; i < 4; i++) {
      c.rect(18 + i * 153, 374, 142, 20, slots[i]);
    }
    snprintf(b, sizeof(b), "PRESENT RATE %.1f FPS / TARGET 30", v.fps);
    text(c, 18, 414, b, DIM, 2, 50);
  } else if (v.page == 3) {
    snprintf(b, sizeof(b), "CPU %s / KERNEL %s", v.machine, v.kernel);
    text(c, 18, 104, b, DIM, 2, 50);
    text(c, 18, 139, "TARGET GARLICOS 1.3.9 / VERIFY ON DEVICE", TEXT, 2, 50);
    snprintf(b, sizeof(b), "RAM TOTAL %ld KIB / AVAILABLE %ld KIB", v.totalKb, v.availableKb);
    text(c, 18, 181, b, TEXT, 2, 50);
    snprintf(b, sizeof(b), "PROCESS RSS %ld KIB", v.rssKb);
    text(c, 18, 211, b, DIM, 2, 50);
    text(c, 18, 251, v.writeStatus, TEXT, 2, 50);
    text(c, 18, 286, "A: TEST SD / SELECT: SAVE CURRENT REPORT", DIM, 2, 50);
    text(c, 18, 325, "AVAILABLE -1 = KERNEL DOES NOT EXPOSE IT", DIM, 2, 50);
    text(c, 18, 358, "L2/R2 TABS / HOLD MENU 2 SEC TO EXIT", DIM, 2, 50);
    text(c, 18, 392, "REPORTS STAY INSIDE MOSS-LAB/REPORTS", DIM, 2, 50);
  }
  if (v.page == 4) {
    snprintf(b, sizeof(b), "L1 < %s > R1 / %d HZ / %d FR", sampleNames[v.sampleIndex], v.rate,
             v.buffer);
    text(c, 18, 104, b, TEXT, 2, 50);
    if (v.sampleLoaded) {
      snprintf(b, sizeof(b), "%u HZ / %u CH / %u KIB / LOAD %u MS", v.sampleRate, v.sampleChannels,
               (v.sampleBytes + 1023) / 1024, v.sampleLoadMs);
      text(c, 18, 133, b, DIM, 2, 50);
    } else
      text(c, 18, 133, "START: LOAD WAV FROM SD / PCM LIMIT 256 KIB", DIM, 2, 50);
    int left = 18 + 604 * v.sampleStart / 100, right = 18 + 604 * v.sampleEnd / 100;
    c.rect(left, 166, right - left, 88, PANEL);
    c.rect(18, 210, 604, 1, LINE);
    for (int i = 0; i < 64; i++) {
      int height = v.sampleLoaded ? v.samplePeaks[i] * 40 / 8192 : 0;
      if (height > 40)
        height = 40;
      c.rect(18 + i * 604 / 64, 210 - height, 5, height * 2 + 1, DIM);
    }
    c.rect(left, 166, 1, 88, TEXT);
    c.rect(right - 1, 166, 1, 88, TEXT);
    if (v.sampleLoaded && v.sampleFrames && v.audioRunning)
      c.rect(18 + int(uint64_t(v.samplePosition) * 603 / v.sampleFrames), 166, 2, 88, TEXT);
    snprintf(b, sizeof(b), "CALLBACKS %u / MAX %llu US / LATE %llu", v.callbacks,
             (unsigned long long)v.callbackMaxUs, (unsigned long long)v.late);
    text(c, 18, 270, b, DIM, 2, 50);
    text(c, 18, 296, v.audioStatus, TEXT, 2, 50);
    snprintf(b, sizeof(b), "TAP A HEARD / B SILENT / X CLICKS: %s", v.verdict);
    text(c, 18, 322, b, DIM, 2, 50);
    snprintf(b, sizeof(b), "%+d", v.samplePitch);
    field(c, v, 0, "PITCH", b, "SEMITONES");
    snprintf(b, sizeof(b), "%d", v.sampleStart);
    field(c, v, 1, "START", b, "PERCENT");
    snprintf(b, sizeof(b), "%d", v.sampleEnd);
    field(c, v, 2, "END", b, "PERCENT");
    const char *modes[] = {"ONCE", "LOOP", "THREE"};
    field(c, v, 3, "MODE", modes[v.sampleMode], "1/3 VOICES");
  }
  c.rect(0, 445, W, 1, LINE);
  snprintf(b, sizeof(b), "LIVE %us / RAW %u / %s", v.liveSeconds, v.rawCount,
           v.missingInput ? "WAIT INPUT / EXIT 60S" : "L2 R2 TABS / MENU HOLD EXIT");
  text(c, 18, 461,
       v.page == 0   ? b
       : v.page == 4 ? "START PLAY / HOLD FACE + D-PAD EDIT"
       : v.page == 1 ? "START TONE / SELECT SAVE / MENU HOLD EXIT"
                     : v.reportStatus,
       DIM, 2, 50);
  c.box(0, 0, W, H, LINE);
}
} // namespace lab
#endif
