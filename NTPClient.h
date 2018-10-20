#pragma once

#include "Arduino.h"

#include <Udp.h>

#define SEVENZYYEARS 2208988800UL
#define FRACTIONSPERMILLI (4294967UL)
#define NTP_PACKET_SIZE 48
#define NTP_DEFAULT_LOCAL_PORT 1337

class NTPClient;

typedef void (*NTPUpdateCallbackFunction)(NTPClient* c);

class NTPClient {
  private:
    UDP*          _udp;
    bool          _udpSetup       = false;

    const char*   _poolServerName = "time.nist.gov"; // Default time server
    int           _port           = NTP_DEFAULT_LOCAL_PORT;
    int           _timeOffset     = 0;

    unsigned int  _updateInterval = 60000;  // In ms
    unsigned int  _retryInterval  = 1000;   // In ms

    unsigned long _currentEpoc    = 0;      // In s
    unsigned long _currentFraction = 0;     // In 1/(2^32) s
    unsigned long _lastUpdate     = 0;      // In ms
    unsigned long _lastRequest    = 0;      // IN ms

    byte          _packetBuffer[NTP_PACKET_SIZE];

    NTPUpdateCallbackFunction _updateCallback = NULL;

    void          sendNTPPacket();
    bool          checkResponse();

  public:
    NTPClient(UDP& udp);
    NTPClient(UDP& udp, int timeOffset);
    NTPClient(UDP& udp, const char* poolServerName);
    NTPClient(UDP& udp, const char* poolServerName, int timeOffset);
    NTPClient(UDP& udp, const char* poolServerName, int timeOffset, int updateInterval);

    /**
     * Starts the underlying UDP client with the default local port
     */
    void begin();

    /**
     * Starts the underlying UDP client with the specified local port
     */
    void begin(int port);

    /**
     * This should be called in the main loop of your application. By default an update from the NTP Server is only
     * made every 60 seconds. This can be configured in the NTPClient constructor.
     *
     * @return true on success, false on failure
     */
    bool update();

    /**
     * Has the time ever been sucessfully updated
     *
     */
    bool updated();

    /**
     * Register a callback function for when the time gets updated
     *
     */
    void setUpdateCallback(NTPUpdateCallbackFunction f);

    /**
     * This will force the update from the NTP Server.
     * This can block for a full second
     * @return true on success, false on failure
     */
    bool forceUpdate();

    int getDay();
    int getHours();
    int getMinutes();
    int getSeconds();

    /**
     * Changes the time offset. Useful for changing timezones dynamically
     */
    void setTimeOffset(int timeOffset);

    /**
     * Set the update interval to another frequency. E.g. useful when the
     * timeOffset should not be set in the constructor
     */
    void setUpdateInterval(int updateInterval);

    /**
     * Set the retry interval to another frequency in ms
     */
    void setRetryInterval(int retryInterval);

    /**
     * @return time formatted like `hh:mm:ss`
     */
    String getFormattedTime();

    /**
     * @return time in seconds since Jan. 1, 1970
     */
    unsigned long getEpochTime();

    /**
     * @return time in milliseconds since Jan. 1, 1970
     */
    unsigned long long getEpochMillis();

    /**
     * Stops the underlying UDP client
     */
    void end();
};
