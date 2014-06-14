/*
 * A simple DCF77 decoder for Arduino
 *
 * Copyright (c) 2013-2013 Matthias Bolte
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

const int signalPin = 0;
const int sampleInterval = 10; // milliseconds
const int ledPin = 13;

int secondIndex = -1;
byte bits[59]; // being lazy
int lastValue;
unsigned long lowStart = 0;
unsigned long highStart = 0;
int ledState = LOW;

int readSignalPin() {
  return analogRead(signalPin) > 400 ? LOW : HIGH;
}

boolean sampleSignal() {
  boolean result = false;
  int value = readSignalPin();

  if (lastValue == HIGH) {
    if (value == LOW) { // new second starts
      Serial.println("new second starts");
      unsigned long time = millis(); // FIXME: need to handle overflow

      lowStart = time;

      if (highStart > 0) {
        unsigned long highDuration = time - highStart;

        if (highDuration > 1500) { // new minute starts
          Serial.println("new minute starts");
          secondIndex = 0;
        }
      }

      if (ledState == LOW) {
        ledState = HIGH;
      } else {
        ledState = LOW;
      }

      digitalWrite(ledPin, ledState);
    }
  }
  else { // lastValue == LOW
    if (value == HIGH) {
      unsigned long time = millis(); // FIXME: need to handle overflow

      highStart = time;

      if (lowStart > 0 && secondIndex >= 0 && secondIndex <= 58) {
        unsigned long lowDuration = time - lowStart;

        if (lowDuration < 150) { // bit is 0
          Serial.println("bit is 0");
          bits[secondIndex] = 0;
        }
        else { // bit is 1
          Serial.println("bit is 1");
          bits[secondIndex] = 1;
        }

        ++secondIndex;

        if (secondIndex == 59) {
          result = true;
        }
      }
    }
  }

  lastValue = value;

  return result;
}

void setup() {
  Serial.begin(115200);
  Serial.println("setup");

  pinMode(ledPin, OUTPUT);

  lastValue = readSignalPin();
}

void loop() {
  delay(sampleInterval); // FIXME: need to account for the actual time it takes to do the sampling

  if (sampleSignal()) {
    Serial.print("full: ");

    for (int i = 0; i < 59; ++i) {
      Serial.print(bits[i]);
      Serial.print(" ");
    }

    Serial.println("");

    int day = bits[41] * 20 + bits[40] * 10 + bits[39] * 8 + bits[38] * 4 + bits[37] * 2 + bits[36] * 1;
    int month = bits[49] * 10 + bits[48] * 8 + bits[47] * 4 + bits[46] * 2 + bits[45] * 1;
    int year = bits[57] * 80 + bits[56] * 40 + bits[55] * 20 + bits[54] * 10 + bits[53] * 8 + bits[52] * 4 + bits[51] * 2 + bits[50] * 1;
    int hour = bits[34] * 20 + bits[33] * 10 + bits[32] * 8 + bits[31] * 4 + bits[30] * 2 + bits[29] * 1;
    int minute = bits[27] * 40 + bits[26] * 20 + bits[25] * 10 + bits[24] * 8 + bits[23] * 4 + bits[22] * 2 + bits[21] * 1;

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
