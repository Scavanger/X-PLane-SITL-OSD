#pragma once

#include "platform.h"
#include "tcp.h"

#include <string>
#include <functional>
#include <memory>

typedef enum {
    MSP_FC_VARIANT = 0x2,
    MSP2_INAV_OSD_PREFERENCES = 0x2016,
    MSP_DISPLAYPORT = 182

} mspCommand_e;

typedef enum
{
    DS_IDLE,
    DS_PROTO_IDENTIFIER,
    DS_DIRECTION_V1,
    DS_DIRECTION_V2,
    DS_FLAG_V2,
    DS_PAYLOAD_LENGTH_V1,
    DS_PAYLOAD_LENGTH_JUMBO_LOW,
    DS_PAYLOAD_LENGTH_JUMBO_HIGH,
    DS_PAYLOAD_LENGTH_V2_LOW,
    DS_PAYLOAD_LENGTH_V2_HIGH,
    DS_CODE_V1,
    DS_CODE_JUMBO_V1,
    DS_CODE_V2_LOW,
    DS_CODE_V2_HIGH,
    DS_PAYLOAD_V1,
    DS_PAYLOAD_V2,
    DS_CHECKSUM_V1,
    DS_CHECKSUM_V2,
} decoderState_e;

class MSP {
    private:
        std::unique_ptr<TCP> tcp;
        uint32_t sendTime = 0;
        bool waitForResponse = false;

        decoderState_e decoderState = DS_IDLE;
        int unsupported;
        int message_direction;
        int message_length_expected;
        std::vector<uint8_t> message_buffer;
        int message_length_received;
        int code;
        uint8_t message_checksum;

        std::function<void(mspCommand_e, std::vector<uint8_t>)> onMessageReceived;
        std::function<void(void)> onDisconnect;

        void decode(std::vector<uint8_t> buffer);
        void dispatchMessage(uint8_t crc);
        static int crc8_Dvb_S2(int crc, int ch);

    public:
        MSP();

        bool connect(std::string ipAddress, int port);
        void disconnect();
        bool isConnected();
        
        void send(mspCommand_e cmd, std::vector<uint8_t> payload);
        void send(mspCommand_e cmd);
        void receive();

        void registerMessageReceivedCb(std::function<void(mspCommand_e, std::vector<uint8_t>)> callback);
        void registerDisconnectCb(std::function<void(void)> callback);

        
};