# NTPClient

This fork adds API calls to return the epoch timestamp in milliseconds and UTC.


Examples for connecting to a NTP server:

```cpp
#include <NTPClient.h>
// change next line to use with another board/shield
#include <ESP8266WiFi.h>
//#include <WiFi.h> // for WiFi shield
//#include <WiFi101.h> // for WiFi 101 shield or MKR1000
#include <WiFiUdp.h>

const char *ssid     = "<SSID>";
const char *password = "<PASSWORD>";

WiFiUDP ntpUDP;

// By default 'pool.ntp.org' is used with 60 seconds update interval and
// no offset
NTPClient timeClient(ntpUDP);

// You can specify the time server pool and the offset, (in seconds)
// additionaly you can specify the update interval (in milliseconds).
// NTPClient timeClient(ntpUDP, "europe.pool.ntp.org", 3600, 60000);

void setup(){
  Serial.begin(115200);
  WiFi.begin(ssid, password);

  while ( WiFi.status() != WL_CONNECTED ) {
    delay ( 500 );
    Serial.print ( "." );
  }

  timeClient.begin();
}

void loop() {
  timeClient.update();

  Serial.println(timeClient.getFormattedTime());

  // Unix UTC epoch in milliseconds:
  print64(timeClient.getEpochTimeMillisUTC())

  delay(1000);
}

// Serial has trouble with 64-bit ints, so we need a custom function
void print64(uint64_t value) {
    if(value >= 10)
      print64(value / 10);
    Serial.print((uint32_t)(value % 10));
}
```
