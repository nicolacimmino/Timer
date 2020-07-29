#include <Arduino.h>

#define FIRST_SEGMENT_PIN 4
#define FIRST_MUX_PIN A0
#define DECIMAL_POINT_PIN 15
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

void showTime(byte minutes, byte seconds)
{
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

void loop()
{
  displayMux = (displayMux + 1) % 4;
  selectDisplay(DISPLAY_NONE);

  if (!((millis() / 500) % 2))
  {
    digitalWrite(DECIMAL_POINT_PIN, displayMux != 1 && displayMux != 2);
  }
  else
  {
    digitalWrite(DECIMAL_POINT_PIN, HIGH);
  }

  //showDigit(((millis() / 1000) % 10));
  byte seconds = (millis() / 1000) % 60;
  byte minutes = (60 * (millis() / 1000)) % 60;

  showTime(minutes, seconds);

  selectDisplay(displayMux);

  delay(5);
}
