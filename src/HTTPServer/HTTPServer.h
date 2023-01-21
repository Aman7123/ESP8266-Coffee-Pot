#ifndef HTTPSERVER_H
#define HTTPSERVER_H
#endif

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>

class HTTPServer {
  public:
    HTTPServer();
    // Begin sets up WiFi
    void begin(const char *ssid, const char *passphrase = NULL);
    // After you run all the "on" methods you can being the HTTP Server
    void beginHTTPServer();
    void printWiFi();
    void handleClient();
    void httpException(int code, const char *message);
    bool isServerReady();
    bool determineRequestBodyExists();
    bool determineIfBodyIsReadable();
    void send(int code, const char* content_type = NULL, const String& content = emptyString);

    typedef std::function<void(void)> THandlerFunction;
    void on(const Uri &uri, HTTPMethod method, THandlerFunction fn);

    const String& arg(const String& name) const;
};