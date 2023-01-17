// Functions
#define TRACE(...) Serial.printf(__VA_ARGS__)

// Runtime variables
#define RELAY_PIN D2
#define POWER_ON 0
#define POWER_OFF 1
#define API_PORT 8080
#define RESOURCE_ENDPOINT "/coffee"
#define BREW_CYCLE_TIMEOUT 3600 // one hour in seconds

// General application
#define NEWLINE "\r\n"
#define TIMEZONE "UTC"
#define JSON_MEDIA_TYPE "application/json"

// HTTP body variables
#define BREW_START_FIELD "brewStart"
#define BREW_START_DIFF_FIELD "startDiff"
#define BREW_TIMEOUT_FIELD "brewTimeout"
#define BREW_TIMEOUT_DIFF_FIELD "timeoutDiff"
#define HOPPER_LOADED_FIELD "hopperLoaded"

// HTTP status codes
#define SUCCESS_CODE 200
#define CREATED_CODE 201
#define NO_CONTENT_CODE 204
#define BAD_REQUEST_CODE 400
#define NOT_FOUND_CODE 404
#define SERVICE_UNAVAILABLE_CODE 503