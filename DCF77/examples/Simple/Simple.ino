/*
 * Example for the simple DCF77 decoder for Arduino
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

#include <DCF77.h>

/*
 * The LED is turned on/off at the start of each second.
 */
const int ledPin = 13;
byte ledState = LOW;

/*
 * The signal pin of the DCF77 receiver is connected to digital pin 7.
 */
DCF77 dcf77(7);

void setup() {
  Serial.begin(115200);

  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, HIGH);
}

void loop() {
  dcf77.sampleSignal();

  DCF77::Error error = dcf77.getLastError();

  if (error != DCF77::None) {
    Serial.print("error: ");
    Serial.println(error);

    digitalWrite(ledPin, HIGH);

    return;
  }

  if (dcf77.hasNewMinute()) {
    Serial.println("minute");
  }

  if (dcf77.hasNewSecond()) {
    Serial.print("second: ");
    Serial.println(dcf77.getSecond());

    if (ledState == LOW) {
      ledState = HIGH;
    }
    else {
      ledState = LOW;
    }

    digitalWrite(ledPin, ledState);
  }

  if (dcf77.hasNewData()) {
    Serial.print("time: ");

    if (dcf77.getDay() < 10) {
      Serial.print(0);
    }

    Serial.print(dcf77.getDay());
    Serial.print(".");

    if (dcf77.getMonth() < 10) {
      Serial.print(0);
    }

    Serial.print(dcf77.getMonth());
    Serial.print(".");

    Serial.print(dcf77.getYear());
    Serial.print(" ");

    Serial.print(dcf77.getHour());
    Serial.print(":");

    if (dcf77.getMinute() < 10) {
      Serial.print(0);
    }

    Serial.print(dcf77.getMinute());
    Serial.println("");
  }
}
