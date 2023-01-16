#include <ESP8266WiFi.h>

#define NEWLINE "\r\n"
#define TIMEZONE "UTC"
#define TRACE(...) Serial.printf(__VA_ARGS__)

tm* getTime();
long timeTypeToEpoch(time_t time);
tm getTimeFromEpoch(time_t seconds);
time_t timeSinceEpoch(tm time);
long getCurrentTimeInLongSinceEpoch();
double getCurrentTimeDiffStartTime(tm time);
double getStartTimeDiffCurrentTime(tm time);
String timeToString(tm* timeptr);
long timeStructToLong(tm time);
long currentTimeToLong();