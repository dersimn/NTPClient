/**
 * The MIT License (MIT)
 * Copyright (c) 2015 by Fabrice Weinberg
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

//#include "NTPClient.h"
#define DEBUG_NTPClient 1

NTPClient::NTPClient(UDP& udp) {
  this->_udp            = &udp;
}

NTPClient::NTPClient(UDP& udp, long timeOffset) {
  this->_udp            = &udp;
  this->_timeOffset     = timeOffset;
}

NTPClient::NTPClient(UDP& udp, const char* poolServerName) {
  this->_udp            = &udp;
  this->_poolServerName = poolServerName;
}

NTPClient::NTPClient(UDP& udp, const char* poolServerName, long timeOffset) {
  this->_udp            = &udp;
  this->_timeOffset     = timeOffset;
  this->_poolServerName = poolServerName;
}

NTPClient::NTPClient(UDP& udp, const char* poolServerName, long timeOffset, unsigned long updateInterval) {
  this->_udp            = &udp;
  this->_timeOffset     = timeOffset;
  this->_poolServerName = poolServerName;
  this->_updateInterval = updateInterval;
}

void NTPClient::begin() {
  this->begin(NTP_DEFAULT_LOCAL_PORT);
}

void NTPClient::begin(int port) {
  this->_port = port;
  this->_udp->begin(this->_port);
  this->_udpSetup = true;
}

bool NTPClient::checkResponse() {
  bool updated = false;
  if (_udp->parsePacket() == NTP_PACKET_SIZE) {
    _lastUpdate = millis();
    _udp->read(_packetBuffer, NTP_PACKET_SIZE);

    unsigned long highWord, lowWord;

    // Get the reference timestamp (time server clock was last set)
    highWord = word(_packetBuffer[16], _packetBuffer[17]);
    lowWord = word(_packetBuffer[18], _packetBuffer[19]);
    _referenceSecs = (highWord << 16 | lowWord) - SEVENZYYEARS;
    highWord = word(_packetBuffer[20], _packetBuffer[21]);
    lowWord = word(_packetBuffer[22], _packetBuffer[23]);
    _referenceFraction = highWord << 16 | lowWord;

    // Get the origin timestamp (time we sent this request)
    highWord = word(_packetBuffer[24], _packetBuffer[25]);
    lowWord = word(_packetBuffer[26], _packetBuffer[27]);
    _originSecs = (highWord << 16 | lowWord) - SEVENZYYEARS;
    highWord = word(_packetBuffer[28], _packetBuffer[29]);
    lowWord = word(_packetBuffer[30], _packetBuffer[31]);
    _originFraction = highWord << 16 | lowWord;

    // Get the receive timestamp (time server received this request)
    highWord = word(_packetBuffer[32], _packetBuffer[33]);
    lowWord = word(_packetBuffer[34], _packetBuffer[35]);
    _receiveSecs = (highWord << 16 | lowWord) - SEVENZYYEARS;
    highWord = word(_packetBuffer[36], _packetBuffer[37]);
    lowWord = word(_packetBuffer[38], _packetBuffer[39]);
    _receiveFraction = highWord << 16 | lowWord;

    // t1 = time request was sent (_origin)
    // t2 = time request was received by the server (_receive)
    // t3 = time response was sent (transmit)
    // t4 = time response was received (_lastUpdate)
    // Offset = ((t2 - t1) + (t3 - t4)) / 2
    // Delay  = (t4 - t1) - (t3 - t2)

    // Check if the response is stale (i.e., stray packet from a request that went out before the last one)
    if (_originSecs!=0 && _lastSentSecs!=_originSecs && _lastSentFraction!=_originFraction) {
      #ifdef DEBUG_NTPClient
        Serial.println("STALE RESPONSE: server origin does not match last sent timestamp.");
        Serial.print("lastSent =  "); Serial.print(_lastSentSecs);
        Serial.print("; fraction = "); Serial.println(_lastSentFraction/FRACTIONSPERMILLI);
        Serial.print("origin =    "); Serial.print(_originSecs);
        Serial.print("; fraction = "); Serial.println(_originFraction/FRACTIONSPERMILLI);
      #endif
      // Try again, in case there's another response in the buffer
      checkResponse();
    }

    // Get the transmit timestamp (NTP server time when packet was sent)
    highWord = word(_packetBuffer[40], _packetBuffer[41]);
    lowWord = word(_packetBuffer[42], _packetBuffer[43]);
    // combine the 2 words (4 bytes) into a long integer to form  NTP time (seconds since Jan 1 1900)
    _currentEpoc = (highWord << 16 | lowWord) - SEVENZYYEARS;
    // Now get the fraction of seconds in the next 4 bytes
    highWord = word(_packetBuffer[44], _packetBuffer[45]);
    lowWord = word(_packetBuffer[46], _packetBuffer[47]);
    _currentFraction = highWord << 16 | lowWord;

    #ifdef DEBUG_NTPClient
      Serial.println("NTP Updating... ");
      Serial.print("transmit =  "); Serial.print(_currentEpoc);
      Serial.print("; fraction = "); Serial.println(_currentFraction/FRACTIONSPERMILLI);
    #endif

    // Adjust for network delay
    unsigned long netDelay = (_lastUpdate - _lastRequest) / 2;
        - ((_currentEpoc - _receiveSecs)*1000 + (_currentFraction - _receiveFraction)/FRACTIONSPERMILLI);

    _currentEpoc += netDelay / 1000;
    // Need to account for fraction rollover
    unsigned long newFraction = _currentFraction + (netDelay % 1000) * FRACTIONSPERMILLI;
    if(newFraction < _currentFraction)
      _currentEpoc += 1;  // add one second
    _currentFraction = newFraction;

    // if the user has set a callback function for when the time is updated, call it
    if (_updateCallback) { _updateCallback(this); }

    #ifdef DEBUG_NTPClient
      Serial.print("reference = "); Serial.print(_referenceSecs);
      Serial.print("; fraction = "); Serial.println(_referenceFraction/FRACTIONSPERMILLI);
      Serial.print("origin =    "); Serial.print(_originSecs);
      Serial.print("; fraction = "); Serial.println(_originFraction/FRACTIONSPERMILLI);
      Serial.print("epoc =      "); Serial.print(_currentEpoc);
      Serial.print("; fraction = "); Serial.println(_currentFraction/FRACTIONSPERMILLI);
      Serial.print("netDelay = "); Serial.print(netDelay); Serial.println(" ms");
    #endif

    _lastRequest = 0; // no outstanding request
    updated = true;
  }
  return updated;
}

bool NTPClient::forceUpdate() {
  #ifdef DEBUG_NTPClient
    Serial.println("Update from NTP Server");
  #endif

  this->sendNTPPacket();

  // Wait till data is there or timeout...
  byte timeout = 0;
  bool cb = 0;
  do {
    delay ( 10 );
    cb = this->checkResponse();
    if (timeout > 200) return false; // timeout after 2000 ms
    timeout++;
  } while (cb == false);

  return true;
}

bool NTPClient::update() {
  bool updated = false;
  unsigned long now = millis();

  if ( ((_lastRequest == 0) && (_lastUpdate == 0))                          // Never requested or updated
    || ((_lastRequest == 0) && ((now - _lastUpdate) >= _updateInterval))    // Update after _updateInterval
    || ((_lastRequest != 0) && ((now - _lastRequest) > _retryInterval)) ) { // Update if there was no response to the request

    // setup the UDP client if needed
    if (!this->_udpSetup) {
      this->begin();
    }

    this->sendNTPPacket();
  }

  if (_lastRequest) {
    updated = checkResponse();
  }

  return updated;
}

bool NTPClient::updated() {
  return (_currentEpoc != 0);
}

unsigned long NTPClient::getEpochTimeUTC() const {
    unsigned long epoch = _currentEpoc;
    epoch += (millis() - _lastUpdate + 500) / 1000;
    epoch += (_currentFraction / FRACTIONSPERMILLI + 500) / 1000;
    return epoch;
}

unsigned long long NTPClient::getEpochMillisUTC() {
  unsigned long long epoch = (unsigned long long)_currentEpoc * 1000; // last time returned via server, in millis
  epoch += _currentFraction / FRACTIONSPERMILLI;  // add the fraction from the server
  epoch += millis() - _lastUpdate;                // add the millis that have passed since the last update
  return epoch;
}

unsigned long NTPClient::getEpochTime() const {
  // add user offset to convert UTC to local time
  return _timeOffset + getEpochTimeUTC();
}

unsigned long long NTPClient::getEpochMillis() {
  // add user offset to convert UTC to local time
  return getEpochMillisUTC() + _timeOffset * 1000;
}

int NTPClient::getDay() const {
  return (((this->getEpochTime()  / 86400L) + 4 ) % 7); //0 is Sunday
}
int NTPClient::getHours() const {
  return ((this->getEpochTime()  % 86400L) / 3600);
}
int NTPClient::getMinutes() const {
  return ((this->getEpochTime() % 3600) / 60);
}
int NTPClient::getSeconds() const {
  return (this->getEpochTime() % 60);
}

String NTPClient::getFormattedTime() const {
  unsigned long rawTime = this->getEpochTime();
  unsigned long hours = (rawTime % 86400L) / 3600;
  String hoursStr = hours < 10 ? "0" + String(hours) : String(hours);

  unsigned long minutes = (rawTime % 3600) / 60;
  String minuteStr = minutes < 10 ? "0" + String(minutes) : String(minutes);

  unsigned long seconds = rawTime % 60;
  String secondStr = seconds < 10 ? "0" + String(seconds) : String(seconds);

  return hoursStr + ":" + minuteStr + ":" + secondStr;
}

void NTPClient::end() {
  this->_udp->stop();

  this->_udpSetup = false;
}

void NTPClient::setTimeOffset(int timeOffset) {
  this->_timeOffset     = timeOffset;
}

void NTPClient::setUpdateInterval(unsigned long updateInterval) {
  this->_updateInterval = updateInterval;
}

void NTPClient::setRetryInterval(int retryInterval) {
  _retryInterval = retryInterval;
}

void NTPClient::setUpdateCallback(NTPUpdateCallbackFunction f) {
  _updateCallback = f;
}

void NTPClient::setPoolServerName(const char* poolServerName) {
    this->_poolServerName = poolServerName;
}

void NTPClient::sendNTPPacket() {
  // set all bytes in the buffer to 0
  memset(_packetBuffer, 0, NTP_PACKET_SIZE);
  unsigned long ms_since_last = millis() - _lastUpdate;
  unsigned long tmp;

  // Initialize values needed to form NTP request
  // (see URL above for details on the packets)
  //this->_packetBuffer[0] = 0b11100011;   // LI, Version, Mode
  _packetBuffer[0] = 0b00100011;   // LI, Version, Mode
  //this->_packetBuffer[1] = 0;     // Stratum, or type of clock
  //this->_packetBuffer[2] = 6;     // Polling Interval
  //this->_packetBuffer[3] = 0xEC;  // Peer Clock Precision
  //// 8 bytes of zero for Root Delay & Root Dispersion
  //this->_packetBuffer[12]  = 49;
  //this->_packetBuffer[13]  = 0x4E;
  //this->_packetBuffer[14]  = 49;
  //this->_packetBuffer[15]  = 52;

  // Set the transmit timestamp to the time we are sending this request
  // According to the SNTP protocol, this will be copied into the origin
  // timestamp field by the server. See https://www.ietf.org/rfc/rfc2030.txt
  if(_currentEpoc > 0) {
    _lastSentSecs = _currentEpoc + SEVENZYYEARS + (ms_since_last+500)/1000;
    _packetBuffer[40] = _lastSentSecs >> 24;
    _packetBuffer[41] = _lastSentSecs >> 16;
    _packetBuffer[42] = _lastSentSecs >> 8;
    _packetBuffer[43] = _lastSentSecs;
    #ifdef DEBUG_NTPClient
      Serial.print("sent origin =     "); Serial.print(_lastSentSecs-SEVENZYYEARS);
    #endif
    _lastSentFraction = _currentFraction + ms_since_last * FRACTIONSPERMILLI;
    _packetBuffer[44] = _lastSentFraction >> 24;
    _packetBuffer[45] = _lastSentFraction >> 16;
    _packetBuffer[46] = _lastSentFraction >> 8;
    _packetBuffer[47] = _lastSentFraction;
    #ifdef DEBUG_NTPClient
      Serial.print("; fraction = "); Serial.println(_lastSentFraction/FRACTIONSPERMILLI);
    #endif
  }

  // all NTP fields have been given values, now
  // you can send a packet requesting a timestamp:
  _udp->beginPacket(this->_poolServerName, 123); //NTP requests are to port 123
  _udp->write(this->_packetBuffer, NTP_PACKET_SIZE);
  _udp->endPacket();

  _lastRequest = millis();
}
