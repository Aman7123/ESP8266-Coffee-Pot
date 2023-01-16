## ESP8266 Microservice Coffee Pot

### Description
This code is to be loaded onto any Arduino board which supports WiFi. This guide is written for my ESP8266 LiLon NodeMCU V3. 
A helpful setup guide to get your board started can be found [here](https://www.instructables.com/Getting-Started-With-ESP8266LiLon-NodeMCU-V3Flashi/)

### Installation
Open the `.ino` file in the [Arduino IDE](https://www.arduino.cc/en/software)

Use this like to install the requred ESP8266 libraries into your IDE:
```
http://arduino.esp8266.com/stable/package_esp8266com_index.json
```

Other libraries from Arduino imported through the IDE:
[Arduino_JSON by Arduino](https://github.com/arduino-libraries/Arduino_JSON)

### Configuration
The code needs to be configured to your WiFi network. 
[These lines](https://github.com/Aman7123/ESP8266-Coffee-Pot/blob/5647565b01a936967c40c6259acfd2c4815e8e94/ESP8266-Coffee-Pot.ino#L11-L12) needs to contain for WiFi ssid and password.

NOTE: The esp8266 chip does not have WiFi 5g capibilities.

Within the `ESP8266-Coffee-Pot.ino` file you will find many values in the define section at the top of the file. 
These values can be used to edit things like the HTTP endpoint `RESOURCE_ENDPOINT` and the port the API listens on `API_PORT`.

### Connecting to the board
If you start the board while wired into your Ardino IDE within the build and upload screen you will find the board MAC Adress which can be used to assign a static IP within your routers configurations.

If you check the serial output tab of the Arduino IDE the board will print it's MAC and IP address on startup, set the baud rate to `115200`.

![ESP8266 board serial monitor output showing mac and ip address](./resources/startup-serial-output.png)

### How To Brew Coffee
Brewing coffee is as simple as interacting with the embeded microservice how on your WiFi network.
This board comes equiped with these endpoints on the default configuration:
| Method | Endpoint | Decription |
| --- | --- | --- |
| GET | `/coffee` | Get information related to variables and coffee status |
| POST | `/coffee` | Set a new time and some other control values for brewing coffee |
| PATCH | `/coffee` | Update time and other variables |
| DELETE | `/coffee` | Cancels the coffee brewing |

The request body for the POST and PATCH methods is:
```json
{
  "reoccurringBrew": false,
  "brewStart": 1673826720
}
```