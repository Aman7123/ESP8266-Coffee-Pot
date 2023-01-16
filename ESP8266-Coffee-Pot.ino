// Libraries
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <Arduino_JSON.h>
#include "src/helpers/helpers.h"

// Global variables
#define RELAY_PIN D2
#define POWER_ON 0
#define POWER_OFF 1
#define SSID "Dunder Mifflin Paper Company"
#define PASS "01072019"
#define API_PORT 8080
#define RESOURCE_ENDPOINT "/coffee"
#define JSON_MEDIA_TYPE "application/json"
enum State {Waiting, Pending, Brewing, Cancelled};
enum State state = Waiting;
#define REOCCURING_FIELD "reoccurringBrew"
bool reoccurringBrew = false;
#define BREW_START_FIELD "brewStart"
tm brewStart;
#define BREW_CYCLE_TIMEOUT 3600 // one hour in seconds
tm brewTimeout;

// Global components
ESP8266WebServer apiServer(API_PORT);

// Default 503 error
void handleServiceUnavailable(String message) {
  JSONVar svcUnavailErrObj;
  svcUnavailErrObj["message"]       = message;
  svcUnavailErrObj["timestamp"]     = currentTimeToLong();
  svcUnavailErrObj["requestUri"]    = apiServer.uri();
  apiServer.sendHeader("Retry-After", "30");
  apiServer.send(503, JSON_MEDIA_TYPE, 
    // Body of that response
    JSON.stringify(svcUnavailErrObj)
  );
}

// Helper function check local times to ensure API is ready
void checkAppReady() {
  // Check internal clock
  long currentSeconds = getCurrentTimeInLongSinceEpoch();
  // Ensure larger than a few days in from 1970 
  if(currentSeconds < 90000) handleServiceUnavailable("NTP connection not established yet");
}
  
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
void setPinToState(enum State s) {
  switch (s) {
    case Waiting: 
    	digitalWrite(RELAY_PIN, POWER_OFF);
      break;
    case Pending:
    	digitalWrite(RELAY_PIN, POWER_OFF);
      break;
    case Brewing:
    	digitalWrite(RELAY_PIN, POWER_ON);
      break;
    case Cancelled:
    	digitalWrite(RELAY_PIN, POWER_OFF);
      break;
    default:
      break;
  }
}

// Default 404 error
void handleNotFound() {
  JSONVar notFoundErrObj;
  notFoundErrObj["message"]    = "no route matched with those values";
  notFoundErrObj["timestamp"]  = currentTimeToLong();
  notFoundErrObj["requestUri"] = apiServer.uri();
  apiServer.send(404, JSON_MEDIA_TYPE, 
    // Body of that response
    JSON.stringify(notFoundErrObj)
  );
}

// Default 400 error
void handleBadRequest(String message) {
  JSONVar badReqErrObj;
  badReqErrObj["message"]    = message;
  badReqErrObj["timestamp"]  = currentTimeToLong();
  badReqErrObj["requestUri"] = apiServer.uri();
  if(apiServer.hasArg("plain")) 
    badReqErrObj["requestBodySize"] = strlen(apiServer.arg("plain").c_str());
  apiServer.send(400, JSON_MEDIA_TYPE, 
    // Body of that response
    JSON.stringify(badReqErrObj)
  );
}

// Print WIFI info
void printWiFi() {
  TRACE("mac=");
  Serial.print(WiFi.macAddress());
  TRACE(NEWLINE);
  TRACE("ip=");
  Serial.print(WiFi.localIP());
  TRACE(NEWLINE);
  TRACE("api port=");
  Serial.print(API_PORT);
  TRACE(NEWLINE);
}

// Helper function return true if current request has a body
boolean determineRequestBodyExists() {
  // Check for body
  if(!apiServer.hasArg("plain")) {
    // throw error if not exist
    handleBadRequest("Request body not found");
    return false;
  }
  return true;  
}

// Helper function to determine if the body is parsable into JSON
boolean determineIfBodyIsReadable() {
  // The body is always in plain arg from the server
  JSONVar requestBody = JSON.parse(apiServer.arg("plain"));
  // Validate the parsing is not null
  if (JSON.typeof(requestBody) == "undefined") {
    handleBadRequest("Request body not parsable JSON");
    return false;
  }
  return true;
}

// Helper function to determine the body reoccuring field
// requestBody - Arduino JSON library object
// reoccuringFieldAsReference - Reference to the boolean holdiong reoccuring
// return if a boolean for if the funciton was successful or not
boolean determineReoccuringFieldAsReference(JSONVar requestBody, boolean& reoccuringFieldAsReference) {
  // Ensure the JSON object has the K/V pair
  if(requestBody.hasOwnProperty(REOCCURING_FIELD)) {
    // Get the type of the field's value
    String reOccFieldType = JSON.typeof(requestBody[REOCCURING_FIELD]);
    // Ensure value is something parsable
    if( (reOccFieldType != "number") && (reOccFieldType != "boolean")) { 
      // If the field is not parsable then we need to throw an error
      char* output;
      asprintf(&output, "%s field not valid number or boolean", REOCCURING_FIELD);
      handleBadRequest(output);
      return false;
    }
    // This would be a valid request
    reoccuringFieldAsReference = (boolean) requestBody[REOCCURING_FIELD];
  }
  return true;
}

// Helper function to determine the body startTime field
// requestBody - Arduino JSON library object
// startTimeAsReference - Reference to the in-memory start time
// return is a boolean representing if the function was successful or not
boolean determineStartTimeAsReference(JSONVar requestBody, tm& startTimeAsReference) {
  // Some initial checks to ensure the field exists
  if(!requestBody.hasOwnProperty(BREW_START_FIELD)) {
    char* output;
    asprintf(&output, "%s field not provided but is required", BREW_START_FIELD);
    handleBadRequest(output);
    return false;
  }
  // Some checks to ensure datatype
  if(JSON.typeof(requestBody[BREW_START_FIELD]) != "number") { 
    char* output;
    asprintf(&output, "%s field not valid number", BREW_START_FIELD);
    handleBadRequest(output);
    return false;
  }
  // Assign user time to local variable
  tm tempBrewStart = getTimeFromEpoch((long) requestBody[BREW_START_FIELD]);
  // Ensure brewStart is after currentTime
  long secondsSinceEpoch = getCurrentTimeInLongSinceEpoch();
  double diff = getCurrentTimeDiffStartTime(tempBrewStart);
  double startOffsetInSeconds = 30.0; // 30 seconds
  // If provided brewStart is not 1 minute pst now we return 400 Bad Request
  if(diff > startOffsetInSeconds) {
    char* output;
    long startOffsetLong = (long) startOffsetInSeconds;
    asprintf(&output, "%s must be greater than %ld, current time plus %ld seconds", BREW_START_FIELD, (secondsSinceEpoch + startOffsetLong), startOffsetLong);
    handleBadRequest(output);
    return false;
  }
  // Save passed in start time
  startTimeAsReference = tempBrewStart;
  return true;
}

// Declare API Routes
void restServerController() {
  // Check app is ready
  checkAppReady();
  // 
  // Create coffee
  apiServer.on(RESOURCE_ENDPOINT, HTTP_POST, []() {
    // Check for body
    if(!determineRequestBodyExists()) return;
    // Validate proper JSON
    if(!determineIfBodyIsReadable()) return;
    JSONVar requestBody = JSON.parse(apiServer.arg("plain"));
    // Save reoccuring
    if(!determineReoccuringFieldAsReference(requestBody, reoccurringBrew)) return;
    // Save startTime
    if(!determineStartTimeAsReference(requestBody, brewStart)) return;
    // Set status to pending
    state = Pending;
    // Respond to the client
    apiServer.send(201, JSON_MEDIA_TYPE);
  });
  // 
  // Read coffee
  apiServer.on(RESOURCE_ENDPOINT, HTTP_GET, []() {
    JSONVar body;
    body["state"] = getStateAsString(state);
    // Calculate for response purposes the start time are adequate before telling the client or just safely show null
    if(state != Waiting) {
      // Append the brew start time
      body[BREW_START_FIELD] = timeToString(&brewStart);
      body["startDiff"] = (long) getStartTimeDiffCurrentTime(brewStart);      
    } else {
      body[BREW_START_FIELD] = nullptr;
      body["startDiff"] = nullptr;
    }
    // Add some additional info if the pot if brewing currently
    if(state == Brewing) {
      body["brewTimeout"] = timeToString(&brewTimeout);
      body["timeoutDiff"] = (long) getStartTimeDiffCurrentTime(brewTimeout); 
    } else {
      body["brewTimeout"] = nullptr;   
      body["timeoutDiff"] = nullptr;   
    }
    // Show reoccur from memory
    body[REOCCURING_FIELD] = reoccurringBrew;
    // Print curent timestamp of board
    body["timestamp"] = currentTimeToLong();
    // response to client
    apiServer.send(200, JSON_MEDIA_TYPE,
      // Body of that response
      JSON.stringify(body)
    );
  });
  // 
  // Update coffee
  apiServer.on(RESOURCE_ENDPOINT, HTTP_PATCH, []() {
    // Ensure a POST has occured before running
    if(state != Waiting) handleBadRequest("No initial coffee object created please POST");
    // Check for body
    if(!determineRequestBodyExists()) return;
    // Validate proper JSON
    if(!determineIfBodyIsReadable()) return;
    JSONVar requestBody = JSON.parse(apiServer.arg("plain"));
    // Save reoccuring
    if(!determineReoccuringFieldAsReference(requestBody, reoccurringBrew)) return;
    // Save startTime
    if(!determineStartTimeAsReference(requestBody, brewStart)) return;
    // Set our state back to pending
    state = Pending;
    // Respond to the client
    apiServer.send(201, JSON_MEDIA_TYPE);
  });
  // 
  // Delete coffee
  apiServer.on(RESOURCE_ENDPOINT, HTTP_DELETE, []() {
    state = Cancelled;
    apiServer.send(204, JSON_MEDIA_TYPE);
  });
  // If no match return 404
  apiServer.onNotFound(handleNotFound);
}

// Startup
void setup() { 
  Serial.begin(115200);
	// Set RELAY_PIN as an output pin
	pinMode(RELAY_PIN, OUTPUT);
  // Turn off the device
	setPinToState(state);
  // Setup delay
  delay(10);
  // Connect to WiFi
  TRACE("Connecting to %s%s", SSID, NEWLINE);
  WiFi.mode(WIFI_STA);
  WiFi.begin(SSID, PASS);
  while(WiFi.status() != WL_CONNECTED) delay(500);
  TRACE("%sConnected to internet%s", NEWLINE, NEWLINE);
  printWiFi();
  // Get time
  configTime(TIMEZONE, "time-b-g.nist.gov", "time-a-g.nist.gov", "time.nist.gov");
  // Init API routes
  restServerController();
  // Start API server
  apiServer.begin();
  TRACE("Application microservice started%s", NEWLINE);
  TRACE("Please allow 30-45 seconds for internal clock to sync with NTP%s", NEWLINE);
}

// Loop
void loop() {
  // Every clock cycle check for pending http sockets
  apiServer.handleClient();
  // Set pin power based on state
	setPinToState(state);
  // Wait for internal clock to set
  long currentSeconds = currentTimeToLong();
  // Implementation for ensuring house does not burn down
  // Turns coffee pot off after timeout value which would be set when brew starts
  // Overwrites brew state to Cancelled after finish
  if(state == Brewing) {
    // Check brew timeout from variable
    long brewTimeoutSeconds = timeStructToLong(brewTimeout);
    // Check if current time is past the timeout value, if so turn off the pot
    if( currentSeconds > brewTimeoutSeconds ) {
      // Set state var to update the pin
      state = Pending;
    }
  }
  // Implementation for brew cycle when ready
  if(state != Brewing && state != Cancelled) {
    // Check start time from variable
    long brewStartSeconds = timeStructToLong(brewStart);
    // Check if brew time is supposed to have occured if it has, start it
    if( currentSeconds > brewStartSeconds ) {
      // Ensure before we exit this flow we must reset the next brew in memory
      if(reoccurringBrew) {
        brewStartSeconds += 86400; // 24 hours
        brewStart = getTimeFromEpoch(brewStartSeconds);
      }
      // Save brew timeout for another check
      // This should save the house from burning down
      brewTimeout = getTimeFromEpoch(currentSeconds + BREW_CYCLE_TIMEOUT);
      // Activate brew
      state = Brewing;
    }
  }
}