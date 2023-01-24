#include "helpers.h"

// Helper function get time from on-board
tm* getTime() {
  time_t now = time(nullptr);
  return gmtime(&now);
}

// Helper to return time type as epoch long
long timeTypeToEpoch(time_t time) {
  return static_cast<long> (time);
}

// Helper function to convert epoch seconds to time
tm getTimeFromEpoch(time_t seconds) {
  tm* timeResponse = gmtime(&seconds);
  return *timeResponse;
}

// Helper function get epoch from time
time_t timeSinceEpoch(tm time) {
  return mktime(&time);
}

// Helper function to get current time as long since epoch
long getCurrentTimeInLongSinceEpoch() {
    tm* time = getTime();
    time_t currentTime = timeSinceEpoch(*time);
    return timeTypeToEpoch(currentTime);
}

// Helper function to diff current time and start time
double getCurrentTimeDiffStartTime(tm time) {
    tm* currentTimeStruct = getTime();
    time_t currentTime = timeSinceEpoch(*currentTimeStruct);
    time_t brewStartTime = timeSinceEpoch(time);
    return difftime(currentTime, brewStartTime);
}

// Helper function to diff current time and start time
double getStartTimeDiffCurrentTime(tm time) {
    tm* currentTimeStruct = getTime();
    time_t currentTime = timeSinceEpoch(*currentTimeStruct);
    time_t brewStartTime = timeSinceEpoch(time);
    return difftime(brewStartTime, currentTime);
}

// Helper function to turn time into human readable format
String timeToString(tm* timeptr) {
  static const char wday_name[][4] = {
    "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"
  };
  static const char mon_name[][4] = {
    "Jan", "Feb", "Mar", "Apr", "May", "Jun",
    "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
  };
  static char result[26];
  sprintf(result, "%.3s %.3s%3d %.2d:%.2d:%.2d %d",
    wday_name[timeptr->tm_wday],
    mon_name[timeptr->tm_mon],
    timeptr->tm_mday, 
    timeptr->tm_hour,
    timeptr->tm_min, 
    timeptr->tm_sec,
    1900 + timeptr->tm_year
  );
  return result;
}

long timeStructToLong(tm time) {
  time_t timeToEpoch = timeSinceEpoch(time);
  if(timeToEpoch < 0) timeToEpoch = 0;
  long timeToLong = timeTypeToEpoch(timeToEpoch);
  return timeToEpoch;
}

long currentTimeToLong() {
  tm* time = getTime();
  return timeStructToLong(*time);
}