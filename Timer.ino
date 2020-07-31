#include <Arduino.h>

#define FIRST_SEGMENT_PIN 4
#define FIRST_MUX_PIN A0
#define DECIMAL_POINT_PIN 15
#define BUTTON_LIGHT_PIN 16
#define BUTTON_SWITCH_PIN 14
#define DISPLAY_NONE 5

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

void loop()
{
  if (digitalRead(BUTTON_SWITCH_PIN) == HIGH)
  {
    unsigned long pressStartTime = millis();

    bool longPress = false;
    while (digitalRead(BUTTON_SWITCH_PIN) == HIGH)
    {
      if (millis() - pressStartTime > 1000)
      {
        longPress = true;
        break;
      }
    }

    secondsToEnd += longPress ? 600 : 60;

    while (digitalRead(BUTTON_SWITCH_PIN) == HIGH)
    {
      delay(1);
    }
  }

  displayMux = (displayMux + 1) % 4;
  selectDisplay(DISPLAY_NONE);

  if (!((millis() / 500) % 2))
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
