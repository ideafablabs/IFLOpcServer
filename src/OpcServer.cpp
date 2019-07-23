#include "OpcServer.h"
#include "Definitions.h"

template<>
OpcServer<WiFiServer>::OpcServer(WiFiServer& server,
                     uint8_t opcChannel,
                     OpcClient opcClients[],
                     uint8_t clientSize,
                     uint8_t buffer[],
                     uint32_t bufferSize,
                     OpcMsgReceivedCallback OpcMsgReceivedCallback,
                     OpcClientConnectedCallback opcClientConnectedCallback,
                     OpcClientDisconnectedCallback opcClientDisconnectedCallback)
    : opcClients_(opcClients),
      server_(server),
      opcMsgReceivedCallback_(OpcMsgReceivedCallback),
      opcClientConnectedCallback_(opcClientConnectedCallback),
      opcClientDisconnectedCallback_(opcClientDisconnectedCallback),
      bufferSize_(bufferSize),
      opcChannel_(opcChannel),
      clientSize_(clientSize),
      clientCount_(0) {
  for (size_t i = 0; i < clientSize_; i++) {
    opcClients_[i].bufferSize = bufferSize_;
    opcClients_[i].buffer = buffer + (bufferSize_ * i);
  }
}

template<>
OpcServer<UDPLISTENER>::OpcServer(UDPLISTENER& server,
                     uint8_t opcChannel,
                     uint8_t buffer[],
                     uint32_t bufferSize,
                     OpcMsgReceivedCallback OpcMsgReceivedCallback)
    : server_(server),
      opcMsgReceivedCallback_(OpcMsgReceivedCallback),
      bufferSize_(bufferSize),
      opcChannel_(opcChannel),
      clientSize_(0),
      clientCount_(0) {}

template<>
bool OpcServer<WiFiServer>::begin() {
#if SERVER_BEGIN_BOOL
  if (!server_.begin()) {
    return false;
  }
#else
  server_.begin();
#endif

  return true;
}

template<>
bool OpcServer<UDPLISTENER>::begin() {
#if SERVER_BEGIN_BOOL
  if (!server_.listner.begin(server_.port)) {
    return false;
  }
#else
  server_.listener.begin(server_.port);
#endif

  return true;
}

template<>
void OpcServer<WiFiServer>::opcRead(OpcClient& opcClient) {
  size_t readLen;

  WiFiClient client = opcClient.tcpClient;
  uint8_t* buf = opcClient.buffer;

  while (opcClient.bytesAvailable > 0){
    
    if(opcClient.header.dataLength == 0) {
      readLen = client.read((uint8_t*)buf, OPC_HEADER_BYTES);
      // We have a new Packet! Read Header
      debug_sprint("New Data Packet\n");
      opcClient.header.channel = buf[0];
      opcClient.header.command = buf[1];
      opcClient.header.lenHigh = buf[2];
      opcClient.header.lenLow = buf[3];
      opcClient.header.dataLength = opcClient.header.lenLow | (unsigned(opcClient.header.lenHigh) << 8);
      
      opcClient.bytesAvailable -= OPC_HEADER_BYTES;
      opcClient.bufferLength += OPC_HEADER_BYTES;
      debug_sprint("Received Data Packet of ", opcClient.header.dataLength, " Bytes with ", opcClient.bytesAvailable, " bytes available\n");
    } 

    if(opcClient.bytesAvailable <= opcClient.header.dataLength) {
      // Our TCP Packet is a full OPC packet
      readLen = client.read((uint8_t*)buf + opcClient.bufferLength, opcClient.bytesAvailable);
      opcClient.bufferLength += readLen;

    } else {
      // Our TCP Packet is a more than a full OPC packet
      readLen = client.read((uint8_t*)buf + opcClient.bufferLength, ( opcClient.header.dataLength + OPC_HEADER_BYTES ) - opcClient.bufferLength);
      opcClient.bufferLength += readLen;
    }

    opcClient.bytesAvailable -= readLen;

    if (opcClient.bufferLength == opcClient.header.dataLength + OPC_HEADER_BYTES) {

      debug_sprint("Received Complete Data Packet\n");
      opcMsgReceivedCallback_(opcClient.header.channel, opcClient.header.command, opcClient.header.dataLength, buf + OPC_HEADER_BYTES);

      // Set to start buffer over on next call
      opcClient.bufferLength = 0;
      opcClient.header.channel = 0;
      opcClient.header.command = 0;
      opcClient.header.lenHigh = 0;
      opcClient.header.lenLow = 0;
      opcClient.header.dataLength = 0;
    }
  }
}

template<>
bool OpcServer<WiFiServer>::processClient(OpcClient& opcClient) {
  if (opcClient.tcpClient.connected()) {
    opcClient.state = OpcClient::CLIENT_STATE_CONNECTED;

    if ((opcClient.bytesAvailable = opcClient.tcpClient.available()) > 0) {
      opcRead(opcClient);
      perf_sprint("*");
    } else {
      perf_sprint(".");
    }
    return true;
  }
  return false;
}

template<>
void OpcServer<WiFiServer>::process() {
  // process existing clients
  clientCount_ = 0;
  for (size_t i = 0; i < clientSize_; i++) {
    if (processClient(opcClients_[i])) {
      clientCount_++;
    } else if (opcClients_[i].state == OpcClient::CLIENT_STATE_CONNECTED) {
      opcClients_[i].state = OpcClient::CLIENT_STATE_DISCONNECTED;
      opcClients_[i].tcpClient.stop();
      opcClients_[i].bytesAvailable = 0;
      opcClientDisconnectedCallback_(opcClients_[i]);
    }
  }

  // Any new clients?
  WiFiClient tcpClient = server_.available();
  if (tcpClient) {
    opcClientConnectedCallback_(tcpClient);

    // Check if we have room for a new client
    if (clientCount_ >= clientSize_) {
      info_sprint("Too many clients, Connection Refused\n");
      tcpClient.stop();
    } else {
      for (size_t i = 0; i < clientSize_; i++) {
        if (!opcClients_[i].tcpClient.connected()) {
          if (opcClients_[i].state != OpcClient::CLIENT_STATE_DISCONNECTED) {
            opcClientDisconnectedCallback_(opcClients_[i]);
          }
          opcClients_[i].tcpClient.stop();
          opcClients_[i].bytesAvailable = 0;
          opcClients_[i].tcpClient = tcpClient;
#if HAS_REMOTE_IP
          opcClients_[i].ipAddress = tcpClient.remoteIP();
#endif
          opcClients_[i].state = OpcClient::CLIENT_STATE_CONNECTED;
          break;
        }
      }
    }
  }
}

template<>
void OpcServer<UDPLISTENER>::process() {
  int packetSize = server_.listener.parsePacket();
  if (packetSize == bufferSize_) {
    uint8_t buffer[bufferSize_];
    // receive incoming UDP packets
    debug_sprint("Received ", packetSize, " bytes from ", server_.listener.remoteIP().toString().c_str(), " port ", server_.listener.remotePort(), "\n");
    int len = server_.listener.read(buffer, bufferSize_);
    if (len == bufferSize_) {
      uint8_t channel = buffer[0];
      uint8_t command = buffer[1];
      uint8_t lenHigh = buffer[2];
      uint8_t lenLow = buffer[3];
      uint32_t dataLength = lenLow | (unsigned(lenHigh) << 8);

      opcMsgReceivedCallback_(channel, command, dataLength, buffer + OPC_HEADER_BYTES);
    } else {
      debug_sprint("Only Read in ", len, " of a ", packetSize, " byte UDP Packet\n");
    }
  } else {
    debug_sprint("Only Received ", packetSize, "in the UDP Packet\n");
  }
}
