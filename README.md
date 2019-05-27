# OpcServer

Open Pixel Control (OPC) server for ESP on the Arduino platform.

Compatible with boards that utilize Arduino style [WiFiServer](https://www.arduino.cc/en/Reference/WiFiServer) / [WiFiClient](https://www.arduino.cc/en/Reference/WiFiClient) APIs.

- **ESP8266**
  - NodeMCU
  - [Adafruit HUZZAH](https://www.adafruit.com/products/2471)


If you've run this successfully on other boards let us know!

# Installation

`git clone https://github.com/ideafablabs/IFLOpcServer.git` 

# Usage

See [examples](https://github.com/ideafablabs/IFLOpcServer/tree/master/examples).

### Includes
```c++
#include "OpcServer.h"
```
Depending on your platform you may need to add other headers. For example _ESP8266WiFi.h_ for ESP8266.

### Initialize
Initialize `OpcServer` with `WiFiServer`, `OpcClient` sized for the number of clients you'd like to support, and a `buffer` large enough for the OPC messages you'd like to receive.

```c++
WiFiServer server = WiFiServer(OPC_PORT);
OpcClient opcClients[OPC_MAX_CLIENTS];
uint8_t buffer[OPC_BUFFER_SIZE * OPC_MAX_CLIENTS];

OpcServer opcServer = OpcServer(server, OPC_CHANNEL,
  opcClients, OPC_MAX_CLIENTS,
  buffer, OPC_BUFFER_SIZE);
```

### setup()

Optionally add any callback functions you'd like `OpcServer` to run when a message is received or a client connects/disconnects.
```c++
opcServer.setMsgReceivedCallback(cbOpcMessage);
opcServer.setClientConnectedCallback(cbOpcClientConnected);
opcServer.setClientDisconnectedCallback(cbOpcClientDisconnected);
```
Then call `.begin()`.
```c++
opcServer.begin();
```

### loop()
Call `.process()` in your loop to process any incoming OPC data.
```c++
opcServer.process();
```
