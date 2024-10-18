#include "msp.h"
#include "helper.h"

#define MSP_START '$'
#define MSP_V1 'M'
#define MSP_V2 'X'
#define MSP_FROM_FC '>'
#define MSP_TO_FC '<'
#define MSP_UNSUPPORTED '!'
#define MSP_V2_FRAMEID 0xFF
#define MAX_MSP_MESSAGE 1024
#define JUMBO_FRAME_MIN_SIZE  255
#define MSP_TIMEOUT 2500

using namespace Helper;


MSP::MSP()
{
    this->tcp = std::make_unique<TCP>();
}

bool MSP::connect(std::string ipAddress, int port)
{
    tcp->openConnection(ipAddress, port);

    if (!tcp->isConnected) {
        return false;
    }

    return true;
}

void MSP::disconnect()
{
    this->tcp->closeConnection();
    this->onDisconnect();
}

bool MSP::isConnected()
{
    this->waitForResponse = false;
    return this->tcp->isConnected;
}

void MSP::send(mspCommand_e cmd)
{
    this->send(cmd, std::vector<uint8_t>());
}

void MSP::send(mspCommand_e cmd, std::vector<uint8_t> payload)
{
    if (!this->isConnected() || this->waitForResponse) {
        return;
    }

    int bufferLength = 9;
    int payloadLength = 0;
    if (payload.empty()) {
        bufferLength += payload.size();
        payloadLength += payload.size();
    }

    std::vector<uint8_t> buffer = std::vector<uint8_t>(bufferLength);
    buffer[0] = MSP_START;
    buffer[1] = MSP_V2;
    buffer[2] = MSP_TO_FC;
    buffer[3] = 0;
    buffer[4] = getLowerByte((uint16_t)cmd);
    buffer[5] = getUpperByte((uint16_t)cmd);
    buffer[6] = getLowerByte((uint16_t)payloadLength);
    buffer[7] = getUpperByte((uint16_t)payloadLength);

    if (!payload.empty()) {
        buffer.insert(buffer.end(), payload.begin(), payload.end());
    }

    int crc = 0;
    for (unsigned int i = 3; i < buffer.size() - 1; i++) {
        crc = this->crc8_Dvb_S2(crc, buffer[i]);
    }
    buffer[buffer.size() - 1] = (uint8_t)crc;

    if (this->tcp->send(buffer) > 0) {
        this->sendTime = getTickCount();
        this->waitForResponse = true;
    } else {
      this->disconnect();
    }
}

void MSP::receive()
{
    if (getTickCount() > this->sendTime + MSP_TIMEOUT) {
        this->sendTime = 0;
        Log("MSP connection timed out");
        this->disconnect();
        return;
    }

    std::vector<uint8_t> buffer = tcp->read();
    if (buffer.empty()) {
        return;
    } 

    this->waitForResponse = false;
    this->decode(buffer);
}

void MSP::registerMessageReceivedCb(std::function<void(mspCommand_e, std::vector<uint8_t>)> callback)
{
  if (callback) {
    this->onMessageReceived = callback;
  }
}

void MSP::registerDisconnectCb(std::function<void(void)> callback)
{
  if (callback) {
    this->onDisconnect = callback;
  }
}

void MSP::decode(std::vector<uint8_t> buffer)
{

  for (uint8_t c: buffer)
  {
    switch (this->decoderState)
    {
    case DS_IDLE: // sync char 1
      if (c == MSP_START)
      {
        this->decoderState = DS_PROTO_IDENTIFIER;
      }
      break;

    case DS_PROTO_IDENTIFIER: // sync char 2
      switch (c)
      {
      case MSP_V1:
        this->decoderState = DS_DIRECTION_V1;
        break;
      case MSP_V2:
        this->decoderState = DS_DIRECTION_V2;
        break;
      default:
        //unknown protocol
        this->decoderState = DS_IDLE;
      }
      break;

    case DS_DIRECTION_V1: // direction (should be >)

    case DS_DIRECTION_V2:
      this->unsupported = 0;
      switch (c)
      {
      case MSP_FROM_FC:
        this->message_direction = 1;
        break;
      case MSP_TO_FC:
        this->message_direction = 0;
        break;
      case MSP_UNSUPPORTED:
        this->unsupported = 1;
        break;
      }
      this->decoderState = this->decoderState == DS_DIRECTION_V1 ? DS_PAYLOAD_LENGTH_V1 : DS_FLAG_V2;
      break;

    case DS_FLAG_V2:
      // Ignored for now
      this->decoderState = DS_CODE_V2_LOW;
      break;
    case DS_PAYLOAD_LENGTH_V1:
      this->message_length_expected = c;
      if (this->message_length_expected == JUMBO_FRAME_MIN_SIZE)
      {
        this->decoderState = DS_CODE_JUMBO_V1;
      }
      else
      {
        this->message_length_received = 0;
        this->message_buffer = std::vector<uint8_t>(this->message_length_expected);
        this->decoderState = DS_CODE_V1;
      }
      break;

    case DS_PAYLOAD_LENGTH_V2_LOW:
      this->message_length_expected = c;
      this->decoderState = DS_PAYLOAD_LENGTH_V2_HIGH;
      break;

    case DS_PAYLOAD_LENGTH_V2_HIGH:
      this->message_length_expected |= c << 8;
      this->message_buffer = std::vector<uint8_t>(this->message_length_expected);
      this->message_length_received = 0;
      if (this->message_length_expected <= MAX_MSP_MESSAGE)
      {
        this->decoderState = this->message_length_expected > 0 ? DS_PAYLOAD_V2 : DS_CHECKSUM_V2;
      }
      else
      {
        //too large payload
        this->decoderState = DS_IDLE;
      }
      break;

    case DS_CODE_V1:
    case DS_CODE_JUMBO_V1:
      this->code = c;
      if (this->message_length_expected > 0)
      {
        // process payload
        if (this->decoderState == DS_CODE_JUMBO_V1)
        {
          this->decoderState = DS_PAYLOAD_LENGTH_JUMBO_LOW;
        }
        else
        {
          this->decoderState = DS_PAYLOAD_V1;
        }
      }
      else
      {
        // no payload
        this->decoderState = DS_CHECKSUM_V1;
      }
      break;

    case DS_CODE_V2_LOW:
      this->code = c;
      this->decoderState = DS_CODE_V2_HIGH;
      break;

    case DS_CODE_V2_HIGH:
      this->code |= c << 8;
      this->decoderState = DS_PAYLOAD_LENGTH_V2_LOW;
      break;

    case DS_PAYLOAD_LENGTH_JUMBO_LOW:
      this->message_length_expected = c;
      this->decoderState = DS_PAYLOAD_LENGTH_JUMBO_HIGH;
      break;

    case DS_PAYLOAD_LENGTH_JUMBO_HIGH:
      this->message_length_expected |= c << 8;
      this->message_buffer = std::vector<uint8_t>(this->message_length_expected);
      this->message_length_received = 0;
      this->decoderState = DS_PAYLOAD_V1;
      break;

    case DS_PAYLOAD_V1:
    case DS_PAYLOAD_V2:
      this->message_buffer[this->message_length_received] = c;
      this->message_length_received++;

      if (this->message_length_received >= this->message_length_expected)
      {
        this->decoderState = this->decoderState == DS_PAYLOAD_V1 ? DS_CHECKSUM_V1 : DS_CHECKSUM_V2;
      }
      break;

    case DS_CHECKSUM_V1:
      if (this->message_length_expected >= JUMBO_FRAME_MIN_SIZE)
      {
        this->message_checksum = JUMBO_FRAME_MIN_SIZE;
      }
      else
      {
        this->message_checksum = this->message_length_expected;
      }
      this->message_checksum ^= this->code;
      if (this->message_length_expected >= JUMBO_FRAME_MIN_SIZE)
      {
        this->message_checksum ^= this->message_length_expected & 0xFF;
        this->message_checksum ^= (this->message_length_expected & 0xFF00) >> 8;
      }
      for (int ii = 0; ii < this->message_length_received; ii++)
      {
        this->message_checksum ^= this->message_buffer[ii];
      }
      this->dispatchMessage(c);
      break;

    case DS_CHECKSUM_V2:
      this->message_checksum = 0;
      this->message_checksum = this->crc8_Dvb_S2(this->message_checksum, 0); // flag
      this->message_checksum = this->crc8_Dvb_S2(this->message_checksum, getLowerByte(this->code));
      this->message_checksum = this->crc8_Dvb_S2(this->message_checksum, getUpperByte(this->code));
      this->message_checksum = this->crc8_Dvb_S2(this->message_checksum, getLowerByte(this->message_length_expected));
      this->message_checksum = this->crc8_Dvb_S2(this->message_checksum, getUpperByte(this->message_length_expected));
      for (int i = 0; i < this->message_length_received; i++)
      {
        this->message_checksum = this->crc8_Dvb_S2(this->message_checksum, this->message_buffer[i]);
      }
      this->dispatchMessage(c);
      break;

    default:
      break;
    }
  }
}

void MSP::dispatchMessage(uint8_t crc) 
{
  if (this->message_checksum == crc) {
    this->onMessageReceived((mspCommand_e)this->code, this->message_buffer);
    this->decoderState = DS_IDLE;
  }
}

int MSP::crc8_Dvb_S2(int crc, int ch)
{
    crc ^= ch;
    for (int i = 0; i < 8; ++i) {
        if ((crc & 0x80) != 0){
            crc = ((crc << 1) & 0xFF) ^ 0xD5;
        }
        else{
            crc = (crc << 1) & 0xFF;
        }
    }
    return crc;
}
