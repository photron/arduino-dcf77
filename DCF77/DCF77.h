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

#ifndef DCF77_H
#define DCF77_H

#include "Arduino.h"

class DCF77
{
  public:
    enum Error {
      None,
      InvalidPulseDuration,
      UnexpectedMinuteStart,
      TooManyBits,
      ChecksumMismatch
    };

  private:
    int _signalPin;
    int _samplingInterval; // milliseconds

    byte _lastSampleValue;
    unsigned long _lastSampleTime;
    unsigned long _lastFlankTime;
    byte _bits[59]; // being lazy, using bytes as bits
    Error _lastError;
    boolean _hasNewMinute;
    boolean _hasNewSecond;
    boolean _hasNewData;

    int _year;
    int _month;
    int _day;
    int _hour;
    int _minute;
    int _second; // -1 means waiting for the next minute start

    byte readSignalPin() const ;
    boolean checkPulseDuration(byte value, unsigned long duration, boolean maximumOnly) const ;
    boolean checkEvenParity(int from, int to) const ;
    boolean checkBits() const ;

  public:
    DCF77(int signalPin);

    void sampleSignal();

    Error getLastError() const;
    boolean hasNewMinute() const;
    boolean hasNewSecond() const;
    boolean hasNewData() const;

    int getYear() const;
    int getMonth() const;
    int getDay() const;
    int getHour() const;
    int getMinute() const;
    int getSecond() const;
};

#endif
