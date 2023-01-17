#include <Arduino.h>

// Time related things
// TODO: clean this up
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