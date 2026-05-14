#include <Arduino.h>
#include <TFT_eSPI.h>
#include <Preferences.h>
#include "StarWarsMovieData.h"

#ifndef TFT_BL
#define TFT_BL 38
#endif

#define BUTTON_A_PIN 0
#define BUTTON_B_PIN 14

static const int SCREEN_W = 320;
static const int SCREEN_H = 170;

static const int FRAME_COLUMNS = 67;
static const int FRAME_LINES = 13;
static const int CHAR_W = 4;
static const int CHAR_H = 6;
static const int FRAME_W = FRAME_COLUMNS * CHAR_W;
static const int FRAME_H = FRAME_LINES * CHAR_H;

static const int FRAME_X = (SCREEN_W - FRAME_W) / 2;
static const int FRAME_Y = 78;

static const uint16_t BG_COLOR = TFT_BLACK;
static const char APP_VERSION[] = "v1.0.1";

static const uint16_t DELAY_UNIT_MS = 100;
static const uint16_t MIN_FRAME_DELAY_MS = 15;
static const uint32_t JUMP_TIME_MS = 60000;
static const uint16_t FRAME_SPEEDUP_MS = 50;
static const uint8_t FAST_FORWARD_DIVISOR = 16;
static const uint16_t LONG_PRESS_MS = 700;
static const uint32_t SAVE_POSITION_INTERVAL_MS = 5000;
static const uint16_t FRAME_HISTORY_SIZE = 512;

static const uint16_t TEXT_COLORS[] =
{
  TFT_GREEN,
  TFT_WHITE,
  TFT_YELLOW,
  TFT_CYAN,
  TFT_ORANGE,
  TFT_RED,
  TFT_BLUE,
  TFT_MAGENTA
};

static const uint8_t TEXT_COLOR_COUNT = sizeof(TEXT_COLORS) / sizeof(TEXT_COLORS[0]);

TFT_eSPI tft = TFT_eSPI();
TFT_eSprite sprite = TFT_eSprite(&tft);
Preferences preferences;

static uint32_t moviePos = 0;
static uint32_t currentFrameStartPos = 0;
static uint32_t currentMovieTimeMs = 0;
static uint32_t currentFrameStartTimeMs = 0;
static uint16_t frameHistoryIndex = 0;
static uint16_t frameHistoryCount = 0;
static uint32_t frameHistoryPos[FRAME_HISTORY_SIZE];
static uint32_t frameHistoryTime[FRAME_HISTORY_SIZE];
static uint32_t lastSavedMovieTimeMs = 0;
static char frameLines[FRAME_LINES][FRAME_COLUMNS + 1];
static bool movieStarted = false;
static bool moviePaused = false;
static bool movieFinished = false;
static bool loopMode = false;
static bool waitReleaseAfterLongPress = false;
static bool welcomeLongPressConsumed = false;
static bool buttonBFastMode = false;
static bool blockFastForwardUntilBReleased = false;
static bool blockPauseActionsUntilReleased = false;
static uint8_t currentColorIndex = 0;
static uint16_t currentTextColor = TFT_GREEN;


static const uint8_t Font3x5[][5] PROGMEM = {
  {0,0,0,0,0}, {2,2,2,0,2}, {5,5,0,0,0}, {5,7,5,7,5}, {3,6,3,1,6}, {5,1,2,4,5}, {2,5,2,5,3}, {2,2,0,0,0},
  {1,2,2,2,1}, {4,2,2,2,4}, {0,5,2,5,0}, {0,2,7,2,0}, {0,0,0,2,4}, {0,0,7,0,0}, {0,0,0,0,2}, {1,1,2,4,4},
  {7,5,5,5,7}, {2,6,2,2,7}, {7,1,7,4,7}, {7,1,7,1,7}, {5,5,7,1,1}, {7,4,7,1,7}, {7,4,7,5,7}, {7,1,2,4,4},
  {7,5,7,5,7}, {7,5,7,1,7}, {0,2,0,2,0}, {0,2,0,2,4}, {1,2,4,2,1}, {0,7,0,7,0}, {4,2,1,2,4}, {7,1,3,0,2},
  {7,5,7,4,7}, {7,5,7,5,5}, {6,5,6,5,6}, {7,4,4,4,7}, {6,5,5,5,6}, {7,4,6,4,7}, {7,4,6,4,4},
  {7,4,5,5,7}, {5,5,7,5,5}, {7,2,2,2,7}, {1,1,1,5,7}, {5,5,6,5,5}, {4,4,4,4,7}, {5,7,7,5,5}, {5,7,7,7,5},
  {7,5,5,5,7}, {7,5,7,4,4}, {7,5,5,7,1}, {7,5,7,6,5}, {7,4,7,1,7}, {7,2,2,2,2}, {5,5,5,5,7}, {5,5,5,5,2},
  {5,5,7,7,5}, {5,5,2,5,5}, {5,5,2,2,2}, {7,1,2,4,7}, {3,2,2,2,3}, {4,4,2,1,1}, {6,2,2,2,6}, {2,5,0,0,0}, {0,0,0,0,7},
  {4,2,0,0,0}, {0,7,1,7,7}, {4,6,5,5,6}, {0,7,4,4,7}, {1,3,5,5,3}, {0,7,5,6,7}, {3,4,6,4,4},
  {0,7,5,7,1}, {4,6,5,5,5}, {2,0,2,2,2}, {1,0,1,5,7}, {4,5,6,5,5}, {2,2,2,2,2}, {0,7,7,5,5}, {0,6,5,5,5},
  {0,7,5,5,7}, {0,7,5,7,4}, {0,7,5,7,1}, {0,7,4,4,4}, {0,7,4,7,1}, {2,7,2,2,3}, {0,5,5,5,7}, {0,5,5,5,2},
  {0,5,5,7,7}, {0,5,2,2,5}, {0,5,5,7,1}, {0,7,1,2,7}, {1,2,6,2,1}, {2,2,2,2,2}, {4,2,3,2,4}, {0,0,3,6,0}
};

static bool isButtonPressed()
{
  return digitalRead(BUTTON_A_PIN) == LOW || digitalRead(BUTTON_B_PIN) == LOW;
}

static void waitForButtonsReleased()
{
  while (isButtonPressed())
  {
    delay(10);
  }
}

static bool isButtonALongPressed()
{
  if (digitalRead(BUTTON_A_PIN) != LOW)
  {
    return false;
  }

  uint32_t startMs = millis();

  while (digitalRead(BUTTON_A_PIN) == LOW)
  {
    if ((millis() - startMs) >= LONG_PRESS_MS)
    {
      return true;
    }

    delay(10);
  }

  return false;
}

static bool isButtonBLongPressed()
{
  if (digitalRead(BUTTON_B_PIN) != LOW)
  {
    return false;
  }

  uint32_t startMs = millis();

  while (digitalRead(BUTTON_B_PIN) == LOW)
  {
    if ((millis() - startMs) >= LONG_PRESS_MS)
    {
      return true;
    }

    delay(10);
  }

  return false;
}

static void saveColorIndex()
{
  preferences.putUChar("color", currentColorIndex);
}

static void loadColorIndex()
{
  preferences.begin("sw-ascii", false);
  currentColorIndex = preferences.getUChar("color", 0);

  if (currentColorIndex >= TEXT_COLOR_COUNT)
  {
    currentColorIndex = 0;
  }

  currentTextColor = TEXT_COLORS[currentColorIndex];
}


static void saveMoviePosition()
{
  preferences.putUInt("moviePos", moviePos);
  preferences.putUInt("movieTime", currentMovieTimeMs);
}

static void resetStoredMoviePosition()
{
  preferences.putUInt("moviePos", 0);
  preferences.putUInt("movieTime", 0);
}

static void loadMoviePosition()
{
  moviePos = preferences.getUInt("moviePos", 0);
  currentMovieTimeMs = preferences.getUInt("movieTime", 0);

  if (moviePos >= STAR_WARS_MOVIE_SIZE)
  {
    moviePos = 0;
    currentMovieTimeMs = 0;
  }

  currentFrameStartPos = moviePos;
  currentFrameStartTimeMs = currentMovieTimeMs;
  clearFrameHistory();
}

static void cycleTextColor()
{
  currentColorIndex++;

  if (currentColorIndex >= TEXT_COLOR_COUNT)
  {
    currentColorIndex = 0;
  }

  currentTextColor = TEXT_COLORS[currentColorIndex];
  saveColorIndex();
}


static uint8_t glyphIndex(char c)
{
  if (c < 32 || c > 126)
  {
    return 0;
  }
  return (uint8_t)(c - 32);
}

static void drawTinyChar(int x, int y, char c, uint16_t color)
{
  uint8_t idx = glyphIndex(c);

  for (int row = 0; row < 5; row++)
  {
    uint8_t bits = pgm_read_byte(&Font3x5[idx][row]);

    for (int col = 0; col < 3; col++)
    {
      if (bits & (1 << (2 - col)))
      {
        sprite.drawPixel(x + col, y + row, color);
      }
    }
  }
}

static void drawTinyText(int x, int y, const char *text, uint16_t color)
{
  int cx = x;
  while (*text)
  {
    drawTinyChar(cx, y, *text, color);
    cx += CHAR_W;
    text++;
  }
}

static void drawCenteredTinyText(int y, const char *text, uint16_t color)
{
  int len = strlen(text);
  int x = (SCREEN_W - (len * CHAR_W)) / 2;
  drawTinyText(x, y, text, color);
}

static void drawAsciiFrame(const char lines[FRAME_LINES][FRAME_COLUMNS + 1], int x, int y)
{
  for (int row = 0; row < FRAME_LINES; row++)
  {
    for (int col = 0; col < FRAME_COLUMNS; col++)
    {
      char c = lines[row][col];
      if (c != ' ')
      {
        drawTinyChar(x + (col * CHAR_W), y + (row * CHAR_H), c, currentTextColor);
      }
    }
  }
}



static void drawIntroScreen()
{
  sprite.fillSprite(BG_COLOR);

  sprite.setTextColor(currentTextColor, BG_COLOR);
  sprite.setTextFont(1);
  sprite.setTextSize(1);
  sprite.setTextDatum(MC_DATUM);

  sprite.drawString("STAR WARS ASCIIMATION", SCREEN_W / 2, 32);

  sprite.drawString("This is a custom version", SCREEN_W / 2, 62);
  sprite.drawString("generated by TitaNets", SCREEN_W / 2, 76);
  sprite.drawString("for LilyGo T-Display S3", SCREEN_W / 2, 90);

  sprite.drawString("Press any button to continue", SCREEN_W / 2, 128);
  sprite.drawString(APP_VERSION, SCREEN_W / 2, 144);

  sprite.pushSprite(0, 0);
}


static void drawPauseScreen()
{
  sprite.fillSprite(BG_COLOR);

  sprite.setTextColor(currentTextColor, BG_COLOR);
  sprite.setTextFont(1);
  sprite.setTextSize(2);
  sprite.setTextDatum(MC_DATUM);

  sprite.drawString("PAUSED", SCREEN_W / 2, 40);

  // Progress bar
  const int barX = 30;
  const int barY = 90;
  const int barW = SCREEN_W - 60;
  const int barH = 14;

  sprite.drawRect(barX, barY, barW, barH, currentTextColor);

  float progress = 0.0f;

  if (STAR_WARS_MOVIE_SIZE > 0)
  {
    progress = (float)moviePos / (float)STAR_WARS_MOVIE_SIZE;
  }

  if (progress < 0.0f)
  {
    progress = 0.0f;
  }

  if (progress > 1.0f)
  {
    progress = 1.0f;
  }

  int fillW = (int)((barW - 2) * progress);

  sprite.fillRect(barX + 1, barY + 1, fillW, barH - 2, currentTextColor);

  int percent = (int)(progress * 100.0f);

  sprite.setTextSize(1);
  sprite.drawString(String(percent) + "%", SCREEN_W / 2, 120);

  sprite.pushSprite(0, 0);
}

static bool movieAvailable()
{
  return moviePos < STAR_WARS_MOVIE_SIZE;
}

static char readMovieChar()
{
  if (!movieAvailable())
  {
    return '\0';
  }

  char c = (char)pgm_read_byte(&StarWarsMovieData[moviePos]);
  moviePos++;
  return c;
}

static String readCleanLine()
{
  String line;
  line.reserve(80);

  while (movieAvailable())
  {
    char c = readMovieChar();

    if (c == '\n')
    {
      break;
    }

    if (c != '\r')
    {
      line += c;
    }
  }

  return line;
}

static bool readDelayLine(uint16_t &delayTicks)
{
  while (movieAvailable())
  {
    String line = readCleanLine();
    line.trim();

    if (line.length() == 0)
    {
      continue;
    }

    delayTicks = (uint16_t)line.toInt();
    return true;
  }

  return false;
}

static bool readFrame()
{
  for (int row = 0; row < FRAME_LINES; row++)
  {
    if (!movieAvailable())
    {
      return false;
    }

    String line = readCleanLine();

    for (int col = 0; col < FRAME_COLUMNS; col++)
    {
      frameLines[row][col] = (col < line.length()) ? line[col] : ' ';
    }

    frameLines[row][FRAME_COLUMNS] = '\0';
  }

  return true;
}

static void drawMovieFrame()
{
  sprite.fillSprite(BG_COLOR);
  drawAsciiFrame(frameLines, FRAME_X, (SCREEN_H - FRAME_H) / 2);
  sprite.pushSprite(0, 0);
}


static void clearFrameHistory()
{
  frameHistoryIndex = 0;
  frameHistoryCount = 0;
}

static void addFrameHistory(uint32_t framePos, uint32_t frameTimeMs)
{
  if (frameHistoryCount > 0)
  {
    uint16_t lastIndex = (frameHistoryIndex + FRAME_HISTORY_SIZE - 1) % FRAME_HISTORY_SIZE;

    if (frameHistoryPos[lastIndex] == framePos)
    {
      frameHistoryTime[lastIndex] = frameTimeMs;
      return;
    }
  }

  frameHistoryPos[frameHistoryIndex] = framePos;
  frameHistoryTime[frameHistoryIndex] = frameTimeMs;

  frameHistoryIndex = (frameHistoryIndex + 1) % FRAME_HISTORY_SIZE;

  if (frameHistoryCount < FRAME_HISTORY_SIZE)
  {
    frameHistoryCount++;
  }
}

static bool restoreFromHistory(uint32_t targetTimeMs)
{
  if (frameHistoryCount == 0)
  {
    return false;
  }

  bool found = false;
  uint32_t bestTime = 0;
  uint32_t bestPos = 0;

  for (uint16_t i = 0; i < frameHistoryCount; i++)
  {
    if (frameHistoryTime[i] <= targetTimeMs && (!found || frameHistoryTime[i] > bestTime))
    {
      bestTime = frameHistoryTime[i];
      bestPos = frameHistoryPos[i];
      found = true;
    }
  }

  if (!found)
  {
    return false;
  }

  moviePos = bestPos;
  currentFrameStartPos = bestPos;
  currentMovieTimeMs = bestTime;
  currentFrameStartTimeMs = bestTime;
  return true;
}

static bool skipNextFrame(uint32_t &frameDurationMs)
{
  uint32_t frameStartPos = moviePos;
  uint16_t delayTicks = 0;
  currentFrameStartPos = moviePos;
  currentFrameStartTimeMs = currentMovieTimeMs;

  if (!readDelayLine(delayTicks) || !readFrame())
  {
    return false;
  }

  frameDurationMs = frameDelayMs(delayTicks);
  currentFrameStartPos = frameStartPos;
  currentFrameStartTimeMs = currentMovieTimeMs;

  addFrameHistory(frameStartPos, currentMovieTimeMs);

  currentMovieTimeMs += frameDurationMs;
  return true;
}

static bool seekForwardFromCurrent(uint32_t jumpMs)
{
  uint32_t targetTimeMs = currentMovieTimeMs + jumpMs;

  while (movieAvailable() && currentMovieTimeMs < targetTimeMs)
  {
    uint32_t frameDurationMs = 0;

    if (!skipNextFrame(frameDurationMs))
    {
      movieFinished = true;
      return false;
    }
  }

  return true;
}

static bool seekBackwardFromHistory(uint32_t jumpMs)
{
  uint32_t targetTimeMs = 0;

  if (currentMovieTimeMs > jumpMs)
  {
    targetTimeMs = currentMovieTimeMs - jumpMs;
  }

  if (restoreFromHistory(targetTimeMs))
  {
    return true;
  }

  restartMovie();
  return true;
}

static void restartMovie()
{
  moviePos = 0;
  currentFrameStartPos = 0;
  currentMovieTimeMs = 0;
  currentFrameStartTimeMs = 0;
  lastSavedMovieTimeMs = 0;
  movieFinished = false;
  clearFrameHistory();
  resetStoredMoviePosition();
}

static uint32_t frameDelayMs(uint16_t delayTicks)
{
  uint32_t waitMs = (uint32_t)delayTicks * DELAY_UNIT_MS;

  if (waitMs > FRAME_SPEEDUP_MS)
  {
    waitMs -= FRAME_SPEEDUP_MS;
  }

  if (waitMs < MIN_FRAME_DELAY_MS)
  {
    waitMs = MIN_FRAME_DELAY_MS;
  }

  return waitMs;
}

static bool seekMovieToTime(uint32_t targetTimeMs)
{
  if (targetTimeMs >= currentMovieTimeMs)
  {
    return seekForwardFromCurrent(targetTimeMs - currentMovieTimeMs);
  }

  return restoreFromHistory(targetTimeMs);
}

static void jumpBackward()
{
  seekBackwardFromHistory(JUMP_TIME_MS);
}

static void jumpForward()
{
  uint32_t originalPos = moviePos;
  uint32_t originalFramePos = currentFrameStartPos;
  uint32_t originalTime = currentMovieTimeMs;
  uint32_t originalFrameTime = currentFrameStartTimeMs;
  bool originalFinished = movieFinished;

  if (!seekForwardFromCurrent(JUMP_TIME_MS))
  {
    moviePos = originalPos;
    currentFrameStartPos = originalFramePos;
    currentMovieTimeMs = originalTime;
    currentFrameStartTimeMs = originalFrameTime;
    movieFinished = originalFinished;
  }
}

static bool handleMovieButtons()
{
  if (digitalRead(BUTTON_A_PIN) == LOW)
  {
    uint32_t startMs = millis();

    while (digitalRead(BUTTON_A_PIN) == LOW)
    {
      if ((millis() - startMs) >= LONG_PRESS_MS)
      {
        moviePaused = true;
        blockPauseActionsUntilReleased = true;
        drawPauseScreen();
        return true;
      }

      delay(10);
    }

    jumpBackward();
    return true;
  }

  if (digitalRead(BUTTON_B_PIN) == LOW)
  {
    uint32_t startMs = millis();

    while (digitalRead(BUTTON_B_PIN) == LOW)
    {
      if ((millis() - startMs) >= LONG_PRESS_MS)
      {
        buttonBFastMode = true;
        return true;
      }

      delay(10);
    }

    jumpForward();
    return true;
  }

  return false;
}

static bool waitFrame(uint32_t waitMs)
{
  uint32_t startMs = millis();

  while ((millis() - startMs) < waitMs)
  {
    if (digitalRead(BUTTON_A_PIN) == LOW)
    {
      handleMovieButtons();
      return false;
    }

    if (digitalRead(BUTTON_B_PIN) == LOW)
    {
      if (blockFastForwardUntilBReleased)
      {
        delay(5);
        continue;
      }

      if (buttonBFastMode)
      {
        delay(5);
        continue;
      }

      uint32_t pressStartMs = millis();

      while (digitalRead(BUTTON_B_PIN) == LOW)
      {
        if (blockFastForwardUntilBReleased)
        {
          delay(5);
          continue;
        }

        if ((millis() - pressStartMs) >= LONG_PRESS_MS)
        {
          buttonBFastMode = true;
          return true;
        }

        delay(10);
      }

      jumpForward();
      return false;
    }

    delay(10);
  }

  return true;
}

void setup()
{
  pinMode(BUTTON_A_PIN, INPUT_PULLUP);
  pinMode(BUTTON_B_PIN, INPUT_PULLUP);

#ifdef TFT_BL
  pinMode(TFT_BL, OUTPUT);
  digitalWrite(TFT_BL, HIGH);
#endif

  loadColorIndex();
  loadMoviePosition();

  tft.init();
  tft.setRotation(1);
  tft.fillScreen(BG_COLOR);

  sprite.setColorDepth(8);
  sprite.createSprite(SCREEN_W, SCREEN_H);

  drawIntroScreen();
}

void loop()
{
  if (!movieStarted)
  {
    if (waitReleaseAfterLongPress || blockPauseActionsUntilReleased)
    {
      if (!isButtonPressed())
      {
        waitReleaseAfterLongPress = false;
        welcomeLongPressConsumed = false;
        blockPauseActionsUntilReleased = false;
      }

      delay(10);
      return;
    }

    if (isButtonPressed())
    {
      uint32_t startMs = millis();

      while (isButtonPressed())
      {
        if ((millis() - startMs) >= LONG_PRESS_MS)
        {
          loopMode = true;
          welcomeLongPressConsumed = true;
          buttonBFastMode = false;
          blockFastForwardUntilBReleased = (digitalRead(BUTTON_B_PIN) == LOW);

          if (moviePos >= STAR_WARS_MOVIE_SIZE)
          {
            restartMovie();
          }

          movieStarted = true;
          return;
        }

        delay(5);
      }

      loopMode = false;
      buttonBFastMode = false;
      blockFastForwardUntilBReleased = false;

      if (moviePos >= STAR_WARS_MOVIE_SIZE)
      {
        restartMovie();
      }

      movieStarted = true;
    }

    delay(10);
    return;
  }

  if (waitReleaseAfterLongPress)
  {
    if (!isButtonPressed())
    {
      waitReleaseAfterLongPress = false;
    }
  }

  if (blockFastForwardUntilBReleased)
  {
    if (digitalRead(BUTTON_B_PIN) != LOW)
    {
      blockFastForwardUntilBReleased = false;
    }
  }


  if (moviePaused)
  {
    if (blockPauseActionsUntilReleased)
    {
      if (!isButtonPressed())
      {
        blockPauseActionsUntilReleased = false;
      }

      delay(10);
      return;
    }

    if (digitalRead(BUTTON_A_PIN) == LOW)
    {
      uint32_t startMs = millis();

      while (digitalRead(BUTTON_A_PIN) == LOW)
      {
        if ((millis() - startMs) >= LONG_PRESS_MS)
        {
          restartMovie();
          movieStarted = false;
          moviePaused = false;
          loopMode = false;
          buttonBFastMode = false;
          blockFastForwardUntilBReleased = false;
          blockPauseActionsUntilReleased = true;
          drawIntroScreen();
          return;
        }

        delay(10);
      }

      moviePaused = false;
      return;
    }

    if (digitalRead(BUTTON_B_PIN) == LOW)
    {
      uint32_t startMs = millis();

      while (digitalRead(BUTTON_B_PIN) == LOW)
      {
        if ((millis() - startMs) >= LONG_PRESS_MS)
        {
          cycleTextColor();
          drawPauseScreen();
          blockPauseActionsUntilReleased = true;
          return;
        }

        delay(10);
      }

      moviePaused = false;
      return;
    }

    delay(10);
    return;
  }

  uint16_t delayTicks = 0;
  currentFrameStartPos = moviePos;
  currentFrameStartTimeMs = currentMovieTimeMs;

  if (!readDelayLine(delayTicks) || !readFrame())
  {
    restartMovie();
    moviePaused = false;
    buttonBFastMode = false;

    if (loopMode)
    {
      movieStarted = true;
      return;
    }

    movieStarted = false;
    drawIntroScreen();
    return;
  }

  addFrameHistory(currentFrameStartPos, currentFrameStartTimeMs);

  drawMovieFrame();

  uint32_t normalWaitMs = frameDelayMs(delayTicks);
  uint32_t waitMs = normalWaitMs;

  if (buttonBFastMode && !blockFastForwardUntilBReleased)
  {
    if (digitalRead(BUTTON_B_PIN) == LOW)
    {
      waitMs = normalWaitMs / FAST_FORWARD_DIVISOR;

      if (waitMs < MIN_FRAME_DELAY_MS)
      {
        waitMs = MIN_FRAME_DELAY_MS;
      }
    }
    else
    {
      buttonBFastMode = false;
    }
  }

  if (waitFrame(waitMs))
  {
    currentMovieTimeMs += normalWaitMs;

    if ((currentMovieTimeMs - lastSavedMovieTimeMs) >= SAVE_POSITION_INTERVAL_MS)
    {
      saveMoviePosition();
      lastSavedMovieTimeMs = currentMovieTimeMs;
    }
  }
}
