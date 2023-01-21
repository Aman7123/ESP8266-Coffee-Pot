// Libraries
#include <Arduino_JSON.h>
#include "src/helpers/helpers.h"
#include "src/state/state.h"
#include "src/definitions/definitions.h" // Most variables located here
#include "src/HTTPServer/HTTPServer.h"

// Global variables
#define SSID "Dunder Mifflin Paper Company"
#define PASS "01072019"
enum State state = Waiting;
tm reoccurringStart;
tm brewTimeout;
bool hopperLoaded = false;
bool instantBrew = false;
HTTPServer server;

// Helper function to determine the body hopper filled field
// requestBody - Arduino JSON library object
// hopperLoadedAsReference - Reference to the boolean hopper filled
// return if a boolean for if the funciton was successful or not
boolean determineHopperLoadedAsReference(JSONVar requestBody, boolean& hopperLoadedAsReference) {
  // Ensure the JSON object has the K/V pair
  if(requestBody.hasOwnProperty(HOPPER_LOADED_FIELD)) {
    // Get the type of the field's value
    String fieldType = JSON.typeof(requestBody[HOPPER_LOADED_FIELD]);
    // Ensure value is something parsable
    if(fieldType != "boolean") { 
      // If the field is not parsable then we need to throw an error
      char* output;
      asprintf(&output, "%s field not valid boolean", HOPPER_LOADED_FIELD);
      server.httpException(BAD_REQUEST_CODE, output);
      return false;
    }
    // This would be a valid request
    hopperLoadedAsReference = (boolean) requestBody[HOPPER_LOADED_FIELD];
  }
  return true;
}

// Helper function to determine the body instant brew field
// requestBody - Arduino JSON library object
// instantBrewAsReference - Reference to the boolean instant brew
// return if a boolean for if the funciton was successful or not
boolean determineInstantBrewAsReference(JSONVar requestBody, boolean& instantBrewAsReference) {
  // Ensure the JSON object has the K/V pair
  if(requestBody.hasOwnProperty(INSTANT_BREW_FIELD)) {
    // Get the type of the field's value
    String fieldType = JSON.typeof(requestBody[INSTANT_BREW_FIELD]);
    // Ensure value is something parsable
    if(fieldType != "boolean") { 
      // If the field is not parsable then we need to throw an error
      char* output;
      asprintf(&output, "%s field not valid boolean", INSTANT_BREW_FIELD);
      server.httpException(BAD_REQUEST_CODE, output);
      return false;
    }
    // This would be a valid request
    instantBrewAsReference = (boolean) requestBody[INSTANT_BREW_FIELD];
  }
  return true;
}

// Helper function to determine the body startTime field
// requestBody - Arduino JSON library object
// startTimeAsReference - Reference to the in-memory start time
// return is a boolean representing if the function was successful or not
boolean determineReoccurringStartAsReference(JSONVar requestBody, tm& startTimeAsReference) {
  // Some checks to ensure datatype
  if(JSON.typeof(requestBody[REOCCURRING_START_FIELD]) != "number") { 
    char* output;
    asprintf(&output, "%s field not valid number", REOCCURRING_START_FIELD);
    server.httpException(BAD_REQUEST_CODE, output);
    return false;
  }
  // Assign user time to local variable
  tm userProvidedTime = getTimeFromEpoch((long) requestBody[REOCCURRING_START_FIELD]);
  // Ensure reoccurringStart is after currentTime
  long secondsSinceEpoch = getCurrentTimeInLongSinceEpoch();
  double diff = getCurrentTimeDiffStartTime(userProvidedTime);
  double startOffsetInSeconds = 30.0; // 30 seconds
  // If provided reoccurringStart is not 1 minute pst now we return 400 Bad Request
  if(diff > startOffsetInSeconds) {
    char* output;
    long startOffsetLong = (long) startOffsetInSeconds;
    asprintf(&output, "%s must be greater than %ld, current time plus %ld seconds", REOCCURRING_START_FIELD, (secondsSinceEpoch + startOffsetLong), startOffsetLong);
    server.httpException(BAD_REQUEST_CODE, output);
    return false;
  }
  // Save passed in start time
  startTimeAsReference = userProvidedTime;
  return true;
}

// Declare API Routes
void restServerController() {
  // 
  // Create coffee
  server.on(RESOURCE_ENDPOINT, HTTP_POST, []() {
    // Check app is ready
    server.isServerReady();
    // Check for body
    if(!server.determineRequestBodyExists()) return;
    // Validate proper JSON
    if(!server.determineIfBodyIsReadable()) return;
    JSONVar requestBody = JSON.parse(server.arg("plain"));
    // Save hopper loaded
    if(requestBody.hasOwnProperty(HOPPER_LOADED_FIELD)) determineHopperLoadedAsReference(requestBody, hopperLoaded);
    // Save instant brew
    if(requestBody.hasOwnProperty(INSTANT_BREW_FIELD)) determineInstantBrewAsReference(requestBody, instantBrew);
    // Save startTime
    if(requestBody.hasOwnProperty(REOCCURRING_START_FIELD)) determineReoccurringStartAsReference(requestBody, reoccurringStart);
    // Set status to pending
    state = Pending;
    // Respond to the client
    server.send(CREATED_CODE, JSON_MEDIA_TYPE);
  });
  // 
  // Read coffee
  server.on(RESOURCE_ENDPOINT, HTTP_GET, []() {
    // Check app is ready
    server.isServerReady();
    JSONVar body;  
    body["state"] = getStateAsString(state);
    // Calculate for response purposes the start time are adequate before telling the client or just safely show null
    if(state != Waiting) {
      // Append the brew start time
      body[REOCCURRING_START_FIELD] = timeStructToLong(reoccurringStart);
      body[REOCCURRING_DIFF_FIELD] = (long) getStartTimeDiffCurrentTime(reoccurringStart);      
    } else {
      body[REOCCURRING_START_FIELD] = nullptr;
      body[REOCCURRING_DIFF_FIELD] = nullptr;
    }
    // Add some additional info if the pot if brewing currently
    if(state == Brewing) {
      body[BREW_TIMEOUT_FIELD] = timeStructToLong(brewTimeout);
      body[BREW_TIMEOUT_DIFF_FIELD] = (long) getStartTimeDiffCurrentTime(brewTimeout); 
    } else {
      body[BREW_TIMEOUT_FIELD] = nullptr;   
      body[BREW_TIMEOUT_DIFF_FIELD] = nullptr;   
    } 
    // Show hopper loaded from memory
    body[HOPPER_LOADED_FIELD] = hopperLoaded;
    // Add the instant brew metric for debugging
    body[INSTANT_BREW_FIELD] = instantBrew;
    // Print curent timestamp of board
    body["timestamp"] = currentTimeToLong();
    // response to client
    server.send(SUCCESS_CODE, JSON_MEDIA_TYPE,
      // Body of that response
      JSON.stringify(body)
    );
  });
  // 
  // Update coffee
  server.on(RESOURCE_ENDPOINT, HTTP_PATCH, []() {
    // Check app is ready
    server.isServerReady();
    // Ensure a POST has occured before running
    if(state == Waiting) server.httpException(BAD_REQUEST_CODE, "No initial coffee object created please POST");
    // Check for body
    if(!server.determineRequestBodyExists()) return;
    // Validate proper JSON
    if(!server.determineIfBodyIsReadable()) return;
    JSONVar requestBody = JSON.parse(server.arg("plain"));
    // Save hopper loaded
    if(requestBody.hasOwnProperty(HOPPER_LOADED_FIELD)) determineHopperLoadedAsReference(requestBody, hopperLoaded);
    // Save instant brew
    if(requestBody.hasOwnProperty(INSTANT_BREW_FIELD)) determineInstantBrewAsReference(requestBody, instantBrew);
    // Save startTime
    if(requestBody.hasOwnProperty(REOCCURRING_START_FIELD)) determineReoccurringStartAsReference(requestBody, reoccurringStart);
    // Update state
    state = Pending;
    // Respond to the client
    server.send(NO_CONTENT_CODE, JSON_MEDIA_TYPE);
  });
  // 
  // Delete coffee
  server.on(RESOURCE_ENDPOINT, HTTP_DELETE, []() {
    // Check app is ready
    server.isServerReady();
    // Modify values in memory
    state = Cancelled;
    server.send(NO_CONTENT_CODE, JSON_MEDIA_TYPE);
  });
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
  // // Connect to WiFi
  server.begin(SSID, PASS);
  // Init API routes
  restServerController();
  // Start API server
  server.beginHTTPServer();
}

// Loop
void loop() {
  // Every clock cycle check for pending http sockets
  server.handleClient();
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
  // Detmine instant brew
  if(state != Brewing && hopperLoaded) {
    // If instant brew is set
    if(instantBrew) {
      // Save brew timeout for another check
      // This should save the house from burning down
      brewTimeout = getTimeFromEpoch(currentSeconds + BREW_CYCLE_TIMEOUT);
      // Activate brew
      state = Brewing;
      // Deactivate the filled hopper
      hopperLoaded = false;
      // Unset instant brew
      instantBrew = false;
    }
  }  
  long reoccurringStartSeconds = timeStructToLong(reoccurringStart);
  // Implementation for brew cycle when ready
  if( currentSeconds > reoccurringStartSeconds ) {
    // Bump up the brew start value by 24 hours for convience of just PATCHing the hopper loaded value
    reoccurringStartSeconds += 86400; // 24 hours
    // Increase our in memory value
    reoccurringStart = getTimeFromEpoch(reoccurringStartSeconds);
    // For safety mesures if our hopper is not loaded by the registered brew time we cancel
    if(state == Brewing || !hopperLoaded) return;
    // Double check our values just to be sure
    if(state != Brewing && hopperLoaded) {
      // Save brew timeout for another check
      // This should save the house from burning down
      brewTimeout = getTimeFromEpoch(currentSeconds + BREW_CYCLE_TIMEOUT);
      // Activate brew
      state = Brewing;
      // Deactivate the filled hopper
      hopperLoaded = false;
    }
  }
}