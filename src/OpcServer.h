#pragma once

#include "Platforms.h"

#include "Config.h"
#include "Opc.h"

template <class T>
class OpcServer {
 public:
  OpcServer<T>(T& server,
            uint8_t opcChannel,
            OpcClient opcClients[],
            uint8_t clientSize,
            uint8_t buffer[],
            uint32_t bufferSize,
            OpcMsgReceivedCallback opcMsgReceivedCallback = [](uint8_t channel, uint8_t command, uint16_t length, uint8_t* data) -> void {},
            OpcClientConnectedCallback opcClientConnectedCallback = [](WiFiClient&) -> void {},
            OpcClientDisconnectedCallback opcClientDisconnectedCallback = [](OpcClient&) -> void {});

  OpcServer(T& server,
            uint8_t opcChannel,
            uint8_t buffer[],
            uint32_t bufferSize,
            OpcMsgReceivedCallback opcMsgReceivedCallback = [](uint8_t channel, uint8_t command, uint16_t length, uint8_t* data) -> void {});         

  bool begin();
  void process();

  uint32_t getBufferSize() const {
    return bufferSize_;
  }

  uint16_t getBufferSizeInPixels() const {
    return ((bufferSize_ - OPC_HEADER_BYTES) / 3);
  }

  uint32_t getBytesAvailable() const {
    uint32_t b = 0;
    for (size_t i = 0; i < clientSize_; i++) {
      b += opcClients_[i].bytesAvailable;
    }
    return b;
  }
  
  uint8_t getClientCount() const {
    return clientCount_;
  }
  
  uint8_t getClientSize() const {
    return clientSize_;
  }

  void setClientConnectedCallback(OpcClientConnectedCallback opcClientConnectedCallback) {
    opcClientConnectedCallback_ = opcClientConnectedCallback;
  }
  
  void setClientDisconnectedCallback(OpcClientDisconnectedCallback opcClientDisconnectedCallback) {
    opcClientDisconnectedCallback_ = opcClientDisconnectedCallback;
  }
  
  void setMsgReceivedCallback(OpcMsgReceivedCallback opcMsgReceivedCallback) {
    opcMsgReceivedCallback_ = opcMsgReceivedCallback;
  }

 private:
  bool processClient(OpcClient& opcClient);
  void opcRead(OpcClient& opcClient);

  OpcClient* opcClients_;

  T& server_;

  OpcMsgReceivedCallback opcMsgReceivedCallback_;
  OpcClientConnectedCallback opcClientConnectedCallback_;
  OpcClientDisconnectedCallback opcClientDisconnectedCallback_;

  uint32_t bufferSize_;

  uint8_t opcChannel_;
  uint8_t clientSize_;
  uint8_t clientCount_;
};

struct UDPLISTENER {
  WiFiUDP listener;
  uint16_t port;

  UDPLISTENER(WiFiUDP server, uint16_t portNumber) {
    listener = server;
    port = portNumber;
  }
  
};
