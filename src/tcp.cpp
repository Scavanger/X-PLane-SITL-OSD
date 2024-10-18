#include "platform.h"
#include "tcp.h"
#include "helper.h"

#ifdef LINUX
    #include <arpa/inet.h>
    #include <netinet/in.h>
    #include <sys/socket.h>
    #include <fcntl.h> 
    #include <errno.h> 
    #include <termios.h> 
    #include <sys/ioctl.h>
#endif
#include <string.h>

using namespace Helper;

TCP::~TCP() {
    if (this->isConnected) {
        this->closeConnection();
    }
}

void TCP::openConnection(std::string address, int port) 
{
    //TODO: Winndows

    this->socketFd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (this->socketFd == -1) {
        Log("Unable to create socket:", strerror(errno));
        return;
    }

    struct sockaddr_in serverAddr;
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port); 
    serverAddr.sin_addr.s_addr = inet_addr(address.c_str());

    if (connect(this->socketFd, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0)
    {
        Log("Failed to connect to ", address, ": ", strerror(errno));
        this->closeConnection();
        return;
    }
    this->isConnected = true;

    int flags = fcntl(this->socketFd, F_GETFL, 0);
    if (fcntl(this->socketFd, F_SETFL, flags | O_NONBLOCK) < 0) {
        Log("Failed to set NONBLOCK : ", strerror(errno));
        this->closeConnection();
    };
}

void TCP::closeConnection()
{
    if (this->isConnected) {
        if (close(this->socketFd) < 0) {
            Log("Warning: Unable to close TCP connection properly!");
        }
        this->isConnected = false;
    }
}

unsigned int TCP::send(std::vector<uint8_t> &buffer)
{
    if (!this->isConnected || buffer.empty()) {
        return 0;
    }

    int sent = write(this->socketFd, (const char*)buffer.data(), buffer.size());
    if (sent < 0) {
        Log("Unable to send over TCP: ", strerror(errno));
        return 0;
    }

    return sent;
}

std::vector<uint8_t> TCP::read()
{
    int count;
    int ret = ioctl(this->socketFd, FIONREAD, &count);
    if (ret < 0 || count <= 0) {
        return std::vector<uint8_t>();
    }
    
    std::vector<uint8_t> buffer = std::vector<uint8_t>(count);
    int read = recv(this->socketFd, buffer.data(), count, MSG_DONTWAIT);

    if (read > 0) {
       return buffer; 
    }

    return std::vector<uint8_t>();
}