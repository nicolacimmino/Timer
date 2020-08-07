#include <Arduino.h>
#include <avr/sleep.h>

#define FIRST_SEGMENT_PIN 4
#define FIRST_MUX_PIN A0
#define DECIMAL_POINT_PIN 15
#define GREEN_BUTTON_LIGHT_PIN 16
#define RED_BUTTON_LIGHT_PIN 3
#define BUTTON_SWITCH_PIN 14
#define DISPLAY_NONE 5
#define TIMER_START_DELAY_MS 5000

unsigned long pressStartTime = 0;
unsigned long lastPressTime = millis();
unsigned long lastTimerUpdate = millis();
unsigned long showCompletionUntil = 0;
uint16_t secondsToEnd = 0;
long measuredVcc = 0;

bool longPress = false;
bool buttonReleased = false;
bool timerRunning = false;
byte displayMux = 0;
bool lowBattery = false;
bool testMode = false;

void setup()
{
  for (byte ix = 0; ix < 7; ix++)
  {
    pinMode(ix + FIRST_SEGMENT_PIN, OUTPUT);
    digitalWrite(ix + FIRST_SEGMENT_PIN, HIGH);
  }

  pinMode(DECIMAL_POINT_PIN, OUTPUT);
  digitalWrite(DECIMAL_POINT_PIN, HIGH);

  pinMode(GREEN_BUTTON_LIGHT_PIN, OUTPUT);
  digitalWrite(GREEN_BUTTON_LIGHT_PIN, LOW);

  pinMode(RED_BUTTON_LIGHT_PIN, OUTPUT);
  digitalWrite(RED_BUTTON_LIGHT_PIN, LOW);

  pinMode(BUTTON_SWITCH_PIN, INPUT_PULLUP);

  for (byte ix = 0; ix < 4; ix++)
  {
    pinMode(ix + FIRST_MUX_PIN, OUTPUT);
    digitalWrite(ix + FIRST_MUX_PIN, LOW);
  }

  // We were powered up with the button pressed, enter test mode.
  if (digitalRead(BUTTON_SWITCH_PIN))
  {
    testMode = true;
  }
}

void criticalShutdown()
{
  for (byte ix = 0; ix < 7; ix++)
  {
    pinMode(ix + FIRST_SEGMENT_PIN, INPUT);
  }

  pinMode(DECIMAL_POINT_PIN, INPUT);

  pinMode(GREEN_BUTTON_LIGHT_PIN, INPUT);

  pinMode(RED_BUTTON_LIGHT_PIN, INPUT);

  pinMode(BUTTON_SWITCH_PIN, INPUT);

  for (byte ix = 0; ix < 4; ix++)
  {
    pinMode(ix + FIRST_MUX_PIN, INPUT);
  }

  set_sleep_mode(SLEEP_MODE_PWR_DOWN);
  sleep_enable();
  sleep_mode();
  sleep_disable();
}
void checkBattery()
{
  digitalWrite(RED_BUTTON_LIGHT_PIN, lowBattery && (millis() / 500) % 2);

  static unsigned long lastBatteryCheck = 0;

  if (lastBatteryCheck != 0 && millis() - lastBatteryCheck < 3000)
  {
    return;
  }

  lastBatteryCheck = millis();

  // See this article for an in-depth explanation.
  // https://provideyourown.com/2012/secret-arduino-voltmeter-measure-battery-voltage/
  // tl;dr: we switch the ADC to measure the internal 1.1v reference using Vcc as reference, the rest is simple math.

  ADMUX = _BV(REFS0) | _BV(MUX4) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);

  delay(2);
  ADCSRA |= _BV(ADSC);
  while (bit_is_set(ADCSRA, ADSC))
    ;

  measuredVcc = 1125300L / (ADCL | (ADCH << 8));

  analogReference(DEFAULT);

  lowBattery = measuredVcc < 2800;

  if (measuredVcc < 2700)
  {
    criticalShutdown();
  }
}

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

  if (testMode)
  {
    uint8_t batteryPercentage = max((measuredVcc - 2700) / 7, 0);

    minutes = batteryPercentage / 100;
    seconds = batteryPercentage % 100;
  }

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

void showTimeRemaining()
{
  if (!testMode)
  {
    if (!timerRunning || !((millis() / 500) % 2))
    {
      digitalWrite(DECIMAL_POINT_PIN, displayMux != 1 && displayMux != 2);
      digitalWrite(GREEN_BUTTON_LIGHT_PIN, HIGH);
    }
    else
    {
      digitalWrite(DECIMAL_POINT_PIN, HIGH);
      digitalWrite(GREEN_BUTTON_LIGHT_PIN, LOW);
    }
  }
  else
  {
    digitalWrite(DECIMAL_POINT_PIN, displayMux != 3);
  }

  showTime(secondsToEnd);
}

void refreshDisplay()
{
  displayMux = (displayMux + 1) % 4;
  selectDisplay(DISPLAY_NONE);

  showTimeRemaining();

  if (showCompletionUntil > millis())
  {
    if ((millis() / 200) % 2)
    {
      selectDisplay(displayMux);
      digitalWrite(GREEN_BUTTON_LIGHT_PIN, HIGH);
    }
    else
    {
      digitalWrite(GREEN_BUTTON_LIGHT_PIN, LOW);
    }
  }
  else
  {
    selectDisplay(displayMux);
  }

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

  secondsToEnd = secondsToEnd % 3600;

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
      showCompletionUntil = millis() + 3000;
    }
  }

  refreshDisplay();

  checkBattery();
}
