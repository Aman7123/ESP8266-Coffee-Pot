#include <Arduino_JSON.h>
#include "HTTPServer.h"
#include "../definitions/definitions.h"
#include "../helpers/helpers.h"

ESP8266WebServer _server(API_PORT);

HTTPServer::HTTPServer() {
}

void handleNotFound() {
  JSONVar notFoundErrObj;
  notFoundErrObj["message"]    = "no route matched with those values";
  notFoundErrObj["timestamp"]  = currentTimeToLong();
  notFoundErrObj["requestUri"] = _server.uri();
  _server.send(NOT_FOUND_CODE, JSON_MEDIA_TYPE, 
    // Body of that response
    JSON.stringify(notFoundErrObj)
  );
}

void HTTPServer::begin(const char *ssid, const char *passphrase) {
  WiFi.mode(WIFI_OFF);
  // Connect to WiFi
  TRACE("Connecting to %s%s", ssid, NEWLINE);
  WiFi.begin(ssid, passphrase);
  while (WiFi.status() != WL_CONNECTED) delay(500);
  TRACE("Connected to internet%s", NEWLINE);
  printWiFi();
  // Get time
  configTime(TIMEZONE, "time-b-g.nist.gov", "time-a-g.nist.gov", "time.nist.gov");
}

void HTTPServer::beginHTTPServer() {
  // Register default error handler
  _server.onNotFound(handleNotFound);
  // Start API server
  _server.begin();
  TRACE("Application microservice started%s", NEWLINE);
  TRACE("Please allow 30-45 seconds for internal clock to sync with NTP%s", NEWLINE);
}

void HTTPServer::printWiFi()
{
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

void HTTPServer::handleClient() {
  _server.handleClient();
}

void HTTPServer::httpException(int code, const char *message) {
  JSONVar httpErrorObject;
  httpErrorObject["message"]    = message;
  httpErrorObject["timestamp"]  = currentTimeToLong();
  httpErrorObject["requestUri"] = _server.uri();
  // For any errors with a payload append the recieved body length
  if(_server.hasArg("plain")) 
    httpErrorObject["requestBodySize"] = strlen(_server.arg("plain").c_str());
  // For any unavailable service errors add the retry
  if(code == SERVICE_UNAVAILABLE_CODE)
    _server.sendHeader("Retry-After", "30");
  _server.send(code, JSON_MEDIA_TYPE, 
    // Body of that response
    JSON.stringify(httpErrorObject)
  );
}

bool HTTPServer::isServerReady() {
  // Check internal clock
  long currentSeconds = getCurrentTimeInLongSinceEpoch();
  // Ensure larger than a few days in from 1970 
  if(currentSeconds < 90000) {
    httpException(SERVICE_UNAVAILABLE_CODE, "NTP connection not established yet");
    return false;
  }
  return true;
}

// Helper function return true if current request has a body
bool HTTPServer::determineRequestBodyExists() {
  // Check for body
  if(!_server.hasArg("plain")) {
    // throw error if not exist
    httpException(BAD_REQUEST_CODE, "Request body not found");
    return false;
  }
  return true;  
}

// Helper function to determine if the body is parsable into JSON
bool HTTPServer::determineIfBodyIsReadable() {
  // The body is always in plain arg from the server
  JSONVar requestBody = JSON.parse(_server.arg("plain"));
  // Validate the parsing is not null
  if (JSON.typeof(requestBody) == "undefined") {
    httpException(BAD_REQUEST_CODE, "Request body not parsable JSON");
    return false;
  }
  return true;
}

void HTTPServer::send(int code, const char *content_type, const String &content) {
  _server.send(code, content_type, content);
}

void HTTPServer::on(const Uri &uri, HTTPMethod method, THandlerFunction fn) {
  _server.on(uri, method, fn);
}

const String &HTTPServer::arg(const String &name) const {
  return _server.arg(name);
}
