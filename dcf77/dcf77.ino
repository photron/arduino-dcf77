/*
 * A simple DCF77 decoder for Arduino
 *
 * Copyright (c) 2013-2014 Matthias Bolte
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

const int signalPin = 2;
const int ledPin = 13;
const int samplingInterval = 10; // milliseconds

byte lastSampleValue;
unsigned long lastSampleTime;
unsigned long lastFlankTime;
int bitIndex = -1; // -1 means waiting for the next minute start
byte bits[59]; // being lazy, using bytes as bits
int newError = -1; // -1 means no error
boolean newMinute = false;
boolean newSecond = false;
boolean newData = false;
byte ledState = LOW;

byte readSignalPin() {
  // the DCF77 module provides the signal inverted
  return digitalRead(signalPin) == LOW ? HIGH : LOW;
}

boolean checkPulseDuration(byte value, unsigned long duration, boolean maximumOnly) {
  if (value == LOW) { // LOW pulse
    // should be 100ms or 200ms long
    return (maximumOnly || duration > 50) && duration < 250;
  }
  else { // HIGH pulse
    // should be 800ms, 900ms or 1800ms or 1900ms long
    return ((maximumOnly || duration > 750) && duration < 950) ||
           ((maximumOnly || duration > 1750) && duration < 1950);
  }
}

void sampleSignal() {
  byte currentValue = readSignalPin();
  unsigned long currentTime = millis();

  newError = -1;
  newMinute = false;
  newSecond = false;
  newData = false;

  if (lastSampleTime > currentTime) { // timer overflowed, just start over
    lastSampleValue = currentValue;
    lastSampleTime = currentTime;
    lastFlankTime = currentTime;
    bitIndex = -1;

    return;
  }

  if (lastSampleValue == currentValue) { // signal didn't change
    lastSampleTime = currentTime;

    if (!checkPulseDuration(currentValue, currentTime - lastFlankTime, true)) {
      // signal has invalid pulse duration, just start over
      bitIndex = -1;
      newError = 0;
    }

    return;
  }

  unsigned long pulseDuration = currentTime - lastFlankTime;
  byte pulseValue = lastSampleValue;

  lastSampleValue = currentValue;
  lastSampleTime = currentTime;
  lastFlankTime = currentTime;

  if (!checkPulseDuration(pulseValue, pulseDuration, false)) {
    // signal has invalid pulse duration, just start over
    bitIndex = -1;
    newError = 1;

    return;
  }

  if (pulseValue == HIGH) { // HIGH -> LOW: new second starts
    newSecond = true;

    if (pulseDuration > 1750) { // missing second start, new minute starts
      if (bitIndex != -1) { // new minute starts unexpected, just start over
        bitIndex = -1;
        newError = 2;

        return;
      }

      newMinute = true;
      bitIndex = 0;
    }
  }
  else { // LOW -> HIGH: bit ends
    if (bitIndex < 0) { // didn't see minute start yet
      return;
    }

    if (bitIndex > 58) { // too many bits, missed minute start? just start over
      bitIndex = -1;
      newError = 3;

      return;
    }

    if (pulseDuration < 150) { // 50 <= pulseDuration < 150 --> bit is 0
      bits[bitIndex] = 0;
    }
    else { // 150 <= pulseDuration < 250 --> bit is 1
      bits[bitIndex] = 1;
    }

    ++bitIndex;

    if (bitIndex == 59) { // got a complete minute of bits
      bitIndex = -1;
      newData = true;
    }
  }
}

boolean checkEvenParity(int from, int to) { // assumes bits array is fully populated
  int sum = 0;

  for (int i = from; i < to + 1; ++i) {
    sum += bits[i];
  }

  return sum % 2 == 0;
}

boolean checkBits() { // assumes bits array is fully populated
  if (bits[0] != 0) { // bit 0 is always 0
    return false;
  }

  return checkEvenParity(21, 28) && checkEvenParity(29, 35) && checkEvenParity(36, 58);
}

void setup() {
  Serial.begin(115200);
  Serial.println("setup");

  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, HIGH);

  lastSampleValue = readSignalPin();
  lastSampleTime = lastFlankTime = millis();
}

void loop() {
  delay(samplingInterval); // FIXME: need to account for the actual time it takes to do the sampling

  sampleSignal();

  if (newError != -1) {
    Serial.print("error: ");
    Serial.println(newError);

    digitalWrite(ledPin, HIGH);

    return;
  }

  if (newMinute) {
    Serial.println("minute");
  }

  if (newSecond) {
    Serial.print("second: ");
    Serial.println(bitIndex);

    if (ledState == LOW) {
      ledState = HIGH;
    }
    else {
      ledState = LOW;
    }

    digitalWrite(ledPin, ledState);
  }

  if (newData) {
    Serial.print("bits: ");

    for (int i = 0; i < 59; ++i) {
      Serial.print(bits[i]);
      Serial.print(" ");
    }

    Serial.println("");

    if (!checkBits()) {
      Serial.print("error: invalid bits");

      return;
    }

    int day = bits[41] * 20 + bits[40] * 10 + bits[39] * 8 + bits[38] * 4 + bits[37] * 2 + bits[36] * 1;
    int month = bits[49] * 10 + bits[48] * 8 + bits[47] * 4 + bits[46] * 2 + bits[45] * 1;
    int year = bits[57] * 80 + bits[56] * 40 + bits[55] * 20 + bits[54] * 10 + bits[53] * 8 + bits[52] * 4 + bits[51] * 2 + bits[50] * 1;
    int hour = bits[34] * 20 + bits[33] * 10 + bits[32] * 8 + bits[31] * 4 + bits[30] * 2 + bits[29] * 1;
    int minute = bits[27] * 40 + bits[26] * 20 + bits[25] * 10 + bits[24] * 8 + bits[23] * 4 + bits[22] * 2 + bits[21] * 1;

    Serial.print("time: ");

    Serial.print(day);
    Serial.print(".");

    if (month < 10) {
      Serial.print(0);
    }

    Serial.print(month);
    Serial.print(".");

    Serial.print("20");
    Serial.print(year);
    Serial.print(" ");

    Serial.print(hour);
    Serial.print(":");

    if (minute < 10) {
      Serial.print(0);
    }

    Serial.print(minute);
    Serial.println("");
  }
}
