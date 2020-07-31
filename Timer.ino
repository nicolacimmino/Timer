#include <Arduino.h>

#define FIRST_SEGMENT_PIN 4
#define FIRST_MUX_PIN A0
#define DECIMAL_POINT_PIN 15
#define BUTTON_LIGHT_PIN 16
#define BUTTON_SWITCH_PIN 14
#define DISPLAY_NONE 5
#define TIMER_START_DELAY_MS 5000

unsigned long pressStartTime = 0;
unsigned long lastPressTime = millis();
unsigned long lastTimerUpdate = millis();

bool longPress = false;
bool buttonReleased = true;
bool timerRunning = false;

void setup()
{
  for (byte ix = 0; ix < 7; ix++)
  {
    pinMode(ix + FIRST_SEGMENT_PIN, OUTPUT);
    digitalWrite(ix + FIRST_SEGMENT_PIN, HIGH);
  }

  pinMode(DECIMAL_POINT_PIN, OUTPUT);
  digitalWrite(DECIMAL_POINT_PIN, HIGH);

  pinMode(BUTTON_LIGHT_PIN, OUTPUT);
  digitalWrite(BUTTON_LIGHT_PIN, LOW);

  pinMode(BUTTON_SWITCH_PIN, INPUT_PULLUP);

  for (byte ix = 0; ix < 4; ix++)
  {
    pinMode(ix + FIRST_MUX_PIN, OUTPUT);
    digitalWrite(ix + FIRST_MUX_PIN, LOW);
  }
}

byte displayMux = 0;

void selectDisplay(byte display)
{
  for (byte ix = 0; ix < 4; ix++)
  {
    digitalWrite(ix + FIRST_MUX_PIN, ix == display);
  }
}

void showDigit(byte digit)
{
  byte digits[] = {63, 6, 91, 79, 102, 109, 125, 7, 127, 111};

  for (byte ix = 0; ix < 7; ix++)
  {
    digitalWrite(ix + FIRST_SEGMENT_PIN, !(digits[digit] & (1 << ix)));
  }
}

void showTime(uint16_t totalSeconds)
{
  byte seconds = totalSeconds % 60;
  byte minutes = (totalSeconds / 60) % 60;

  if (displayMux == 3)
  {
    showDigit(minutes / 10);
  }
  else if (displayMux == 2)
  {
    showDigit(minutes % 10);
  }
  else if (displayMux == 1)
  {
    showDigit(seconds / 10);
  }
  else if (displayMux == 0)
  {
    showDigit(seconds % 10);
  }
}

uint16_t secondsToEnd = 0;

void refreshDisplay()
{
  displayMux = (displayMux + 1) % 4;
  selectDisplay(DISPLAY_NONE);

  if (!timerRunning || !((millis() / 500) % 2))
  {
    digitalWrite(DECIMAL_POINT_PIN, displayMux != 1 && displayMux != 2);
    digitalWrite(BUTTON_LIGHT_PIN, HIGH);
  }
  else
  {
    digitalWrite(DECIMAL_POINT_PIN, HIGH);
    digitalWrite(BUTTON_LIGHT_PIN, LOW);
  }

  showTime(secondsToEnd);

  selectDisplay(displayMux);

  delay(5);
}

void loop()
{
  if (digitalRead(BUTTON_SWITCH_PIN) == HIGH && !longPress)
  {
    lastPressTime = millis();

    buttonReleased = false;

    if (pressStartTime == 0)
    {
      pressStartTime = millis();
    }

    if (millis() - pressStartTime > 1000)
    {
      longPress = true;
      secondsToEnd += 600;
    }
  }
  else if (pressStartTime != 0 && buttonReleased)
  {
    secondsToEnd += longPress ? 0 : 60;
    pressStartTime = 0;
    longPress = false;
  }
  else
  {
    if (digitalRead(BUTTON_SWITCH_PIN) == LOW)
    {
      buttonReleased = true;
    }
  }

  if (!timerRunning && millis() - lastPressTime > TIMER_START_DELAY_MS && secondsToEnd > 0)
  {
    timerRunning = true;
    lastTimerUpdate = millis() - 1001;
  }

  if (timerRunning && (millis() - lastTimerUpdate > 1000))
  {
    secondsToEnd -= 1;
    lastTimerUpdate = lastTimerUpdate + 1000;

    if (secondsToEnd == 0)
    {
      timerRunning = false;
    }
  }

  refreshDisplay();
}
