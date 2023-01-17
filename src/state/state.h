#include <Arduino.h>

// Variables
enum State {Waiting, Pending, Brewing, Cancelled};

// Functions
String getStateAsString(enum State s);
bool setPinToState(enum State s);