#include "state.h"
#include "../definitions/definitions.h"

// Helper function State enum to string
String getStateAsString(enum State s) {
  switch (s) {
    case Waiting: return "Waiting";
    case Pending: return "Pending";
    case Brewing: return "Brewing";
    case Cancelled: return "Cancelled";
    default: return "Error";
  }
}

// Helper function set pin based on state
bool setPinToState(enum State s) {
  switch (s) {
    case Waiting: 
    	digitalWrite(RELAY_PIN, POWER_OFF);
      return true;
      break;
    case Pending:
    	digitalWrite(RELAY_PIN, POWER_OFF);
      return true;
      break;
    case Brewing:
    	digitalWrite(RELAY_PIN, POWER_ON);
      return true;
      break;
    case Cancelled:
    	digitalWrite(RELAY_PIN, POWER_OFF);
      return true;
      break;
    default:
      return false;
      break;
  }
}