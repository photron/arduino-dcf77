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

#include "DCF77.h"

DCF77::DCF77(int signalPin) {
  pinMode(signalPin, INPUT);

  _signalPin = signalPin;
  _samplingInterval = 10;

  _lastSampleValue = readSignalPin();
  _lastSampleTime = _lastFlankTime = millis();
  _lastError = None;
  _hasNewMinute = false;
  _hasNewSecond = false;
  _hasNewData = false;

  _year = -1;
  _month = -1;
  _day = -1;
  _hour = -1;
  _minute = -1;
  _second = -1;
}

byte DCF77::readSignalPin() const {
  // the DCF77 module provides the signal inverted
  return digitalRead(_signalPin) == LOW ? HIGH : LOW;
}

boolean DCF77::checkPulseDuration(byte value, unsigned long duration, boolean maximumOnly) const  {
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

boolean DCF77::checkEvenParity(int from, int to) const  { // assumes bits array is fully populated
  int sum = 0;

  for (int i = from; i < to + 1; ++i) {
    sum += _bits[i];
  }

  return sum % 2 == 0;
}

boolean DCF77::checkBits() const  { // assumes bits array is fully populated
  if (_bits[0] != 0) { // bit 0 is always 0
    return false;
  }

  return checkEvenParity(21, 28) && checkEvenParity(29, 35) && checkEvenParity(36, 58);
}

void DCF77::sampleSignal() {
  unsigned long currentTime = millis();

  _lastError = None;
  _hasNewMinute = false;
  _hasNewSecond = false;
  _hasNewData = false;

  if (_lastSampleTime > currentTime) { // timer overflowed, just start over
    _lastSampleValue = readSignalPin();
    _lastSampleTime = currentTime;
    _lastFlankTime = currentTime;
    _second = -1;

    return;
  }

  if (currentTime < _lastSampleTime + _samplingInterval) { // don't sample yet
    return;
  }

  byte currentValue = readSignalPin();

  if (_lastSampleValue == currentValue) { // signal didn't change
    _lastSampleTime = currentTime;

    if (!checkPulseDuration(currentValue, currentTime - _lastFlankTime, true)) {
      // signal has invalid pulse duration, just start over
      _second = -1;
      _lastError = InvalidPulseDuration;
    }

    return;
  }

  unsigned long pulseDuration = currentTime - _lastFlankTime;
  byte pulseValue = _lastSampleValue;

  _lastSampleValue = currentValue;
  _lastSampleTime = currentTime;
  _lastFlankTime = currentTime;

  if (!checkPulseDuration(pulseValue, pulseDuration, false)) {
    // signal has invalid pulse duration, just start over
    _second = -1;
    _lastError = InvalidPulseDuration;

    return;
  }

  if (pulseValue == HIGH) { // HIGH -> LOW: new second starts
    _hasNewSecond = true;

    if (_second >= 0) { // found a minute mark before, increase second counter
      ++_second;
    }

    if (pulseDuration > 1750) { // missing second start, new minute starts
      if (_second != -1 && _second != 59) { // new minute starts unexpected, just start over
        _second = -1;
        _lastError = UnexpectedMinuteStart;

        return;
      }

      if (_second == 59) { // got a complete minute of bits
        if (!checkBits()) {
          _lastError = ChecksumMismatch;
        }
        else {
          _year = 2000 + _bits[57] * 80 + _bits[56] * 40 + _bits[55] * 20 + _bits[54] * 10 + _bits[53] * 8 + _bits[52] * 4 + _bits[51] * 2 + _bits[50] * 1;
          _month = _bits[49] * 10 + _bits[48] * 8 + _bits[47] * 4 + _bits[46] * 2 + _bits[45] * 1;
          _day = _bits[41] * 20 + _bits[40] * 10 + _bits[39] * 8 + _bits[38] * 4 + _bits[37] * 2 + _bits[36] * 1;
          _hour = _bits[34] * 20 + _bits[33] * 10 + _bits[32] * 8 + _bits[31] * 4 + _bits[30] * 2 + _bits[29] * 1;
          _minute = _bits[27] * 40 + _bits[26] * 20 + _bits[25] * 10 + _bits[24] * 8 + _bits[23] * 4 + _bits[22] * 2 + _bits[21] * 1;
          _hasNewData = true;
        }
      }

      _hasNewMinute = true;
      _second = 0;
    }
  }
  else { // LOW -> HIGH: bit ends
    if (_second < 0) { // didn't see minute start yet
      return;
    }

    if (_second > 58) { // too many bits, missed minute start? just start over
      _second = -1;
      _lastError = TooManyBits;

      return;
    }

    if (pulseDuration < 150) { // 50 <= pulseDuration < 150 --> bit is 0
      _bits[_second] = 0;
    }
    else { // 150 <= pulseDuration < 250 --> bit is 1
      _bits[_second] = 1;
    }
  }
}

DCF77::Error DCF77::getLastError() const {
  return _lastError;
}

boolean DCF77::hasNewMinute() const {
  return _hasNewMinute;
}

boolean DCF77::hasNewSecond() const {
  return _hasNewSecond;
}

boolean DCF77::hasNewData() const {
  return _hasNewData;
}

int DCF77::getYear() const {
  return _year;
}

int DCF77::getMonth() const {
  return _month;
}

int DCF77::getDay() const {
  return _day;
}

int DCF77::getHour() const {
  return _hour;
}

int DCF77::getMinute() const {
  return _minute;
}

int DCF77::getSecond() const {
  return _second;
}
