#pragma once

#include <string>
#include <vector>
#include <cstdint>

class TCP {
    private:
        int socketFd;

    public:
        bool isConnected = false;

        TCP() = default;
        ~TCP();
        void openConnection(std::string address, int port);
        void closeConnection();
        unsigned int send(std::vector<uint8_t> &buffer);
        std::vector<uint8_t> read();
};