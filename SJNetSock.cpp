#include "SJNetSock.hpp"
#include <vector>
#include <cstring>
#include <array>
#include <algorithm>

#define DATAPACKET_SIZE_T uint16_t
#define DATAPACKET_SIZE_T_MAX UINT16_MAX

using namespace sj::API_RESERVED;

//LINUX
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <netinet/ip.h>
#include <arpa/inet.h>
//LINUX

//DataPacket Class
sj::DataPacket::DataPacket(){};
sj::DataPacket::~DataPacket(){};

sj::Status sj::DataPacket::allDataReaded(){
    return data.size() == 0 ? Status::OK : Status::ERROR;
}
sj::DataPacket& sj::DataPacket::operator<<(const std::int8_t value){ return operator<< <std::int8_t>(value); }
sj::DataPacket& sj::DataPacket::operator<<(const std::int16_t value){ return operator<< <std::int16_t>(value); }
sj::DataPacket& sj::DataPacket::operator<<(const std::int32_t value){ return operator<< <std::int32_t>(value); }
sj::DataPacket& sj::DataPacket::operator<<(const std::int64_t value){ return operator<< <std::int64_t>(value); }
sj::DataPacket& sj::DataPacket::operator<<(const std::string& value){
    for(auto strIt = value.begin(); strIt != value.end(); strIt++)
        operator<<( (std::int8_t)(*strIt) );

    operator<<( (std::int8_t)('\0') );
    return (*this);
}

sj::DataPacket& sj::DataPacket::operator>>(std::int8_t& value){ return operator>> <std::int8_t>(value); }
sj::DataPacket& sj::DataPacket::operator>>(std::int16_t& value){ return operator>> <std::int16_t>(value); }
sj::DataPacket& sj::DataPacket::operator>>(std::int32_t& value){ return operator>> <std::int32_t>(value); }
sj::DataPacket& sj::DataPacket::operator>>(std::int64_t& value){ return operator>> <std::int64_t>(value); }
sj::DataPacket& sj::DataPacket::operator>>(std::string& value){
    char currentChar = '\0';
    value.clear();
    do{
        operator>>( (std::int8_t&)currentChar);
        if(currentChar != '\0')
            value.insert(value.end(), currentChar);
    } while (currentChar != '\0');
    
    return (*this);
}
//DataPacket Class

//Socket Class
sj::API_RESERVED::Socket::Socket(const Mode mode) : socket_fd(-1), mode(mode), dataPacketsBuffer(0)
{}

sj::API_RESERVED::Socket::~Socket() {}

int sj::API_RESERVED::Socket::create(const Socket::Type type){
    socket_fd = ::socket(AF_INET, type == Type::TCP ? SOCK_STREAM : SOCK_DGRAM, 0);
    return socket_fd;
}
int sj::API_RESERVED::Socket::bind(const short port){
    sockaddr_in addr;
    addr.sin_port = htons(port);
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    return ::bind(socket_fd, (sockaddr*)&addr, sizeof(addr));
}
sj::Status sj::API_RESERVED::Socket::receiveInto(DataPacket& dataPacket){
    do{
        if(dataPacketsBuffer.size() >= sizeof(DATAPACKET_SIZE_T)){
            DATAPACKET_SIZE_T packetSizeInBuffer;
            {
                size_t currentByte = 0;
                auto it = dataPacketsBuffer.begin();
                while(currentByte < sizeof(packetSizeInBuffer)){
                    char* const packetSizeInBufferPtr = (char*) &packetSizeInBuffer;
                    *(packetSizeInBufferPtr + currentByte) = *it;

                    currentByte++;
                    it++;
                }

                if(packetSizeInBuffer >= (dataPacketsBuffer.size() - sizeof(packetSizeInBuffer)) ){
                    while(currentByte < (packetSizeInBuffer + sizeof(packetSizeInBuffer))){
                        dataPacket.data.push(*it);

                        currentByte++;
                        it++;
                    }

                    dataPacketsBuffer.erase(dataPacketsBuffer.begin(), it);
                    return Status::OK;
                }
            }   
        }

        std::array<char, DATAPACKET_SIZE_T_MAX + sizeof(DATAPACKET_SIZE_T)> receiveBuffor;
        size_t readedBytes = 0;
        Status status = receiveInto(receiveBuffor.data(), receiveBuffor.max_size(), &readedBytes);
        if(status != Status::OK)
            return status;

        std::copy_n(receiveBuffor.begin(), readedBytes-1, dataPacketsBuffer.end());
    }while(true);
}
sj::Status sj::API_RESERVED::Socket::receiveInto(void* buffer, const size_t bufferSize, size_t* readedBytes){
    const ssize_t recvStatus = recv(socket_fd, buffer, bufferSize, mode == Mode::NON_BLOCKING ? MSG_DONTWAIT : 0);

    if(recvStatus == -1){
        *readedBytes = 0;

        if(mode == Mode::NON_BLOCKING && (errno == EAGAIN || errno == EWOULDBLOCK))
            return Status::UNAVAILABLE;

        return Status::ERROR;
    }

    *readedBytes = recvStatus;
    return Status::OK;
}

sj::Status sj::API_RESERVED::Socket::sendTo(DataPacket& dataPacket, const std::string* ipAddress, const short port){
    const DATAPACKET_SIZE_T dataSize = (DATAPACKET_SIZE_T) dataPacket.data.size();
    std::vector<char> buffer(dataSize);

    memcpy(buffer.data(), &dataSize, sizeof(dataSize));
    for(size_t i=0; i < dataPacket.data.size(); i++){
        buffer[i + sizeof(dataSize)] = dataPacket.data.front();
        dataPacket.data.pop();
    }

    return sendTo(buffer.data(), buffer.size(), ipAddress, port);
}
sj::Status sj::API_RESERVED::Socket::sendTo(const void* data, const size_t dataSize, const std::string* ipAddress, const short port){

    ssize_t sendStatus = -1;
    if(ipAddress != nullptr && port != -1){
        sockaddr_in addr;
        addr.sin_family = AF_INET;
        addr.sin_port = htons(port);
        if(inet_aton(ipAddress->c_str(), &addr.sin_addr) == -1){
            close();
            return Status::ERROR;
        }
        sendStatus = ::sendto(socket_fd, data, dataSize, mode == Mode::NON_BLOCKING ? MSG_DONTWAIT : 0, (sockaddr*) &addr, sizeof(addr));
    }
    else
        sendStatus = ::send(socket_fd, data, dataSize, mode == Mode::NON_BLOCKING ? MSG_DONTWAIT : 0);

    if(sendStatus == -1){
        if(mode == Mode::NON_BLOCKING && (errno == EAGAIN || errno == EWOULDBLOCK))
            return Status::UNAVAILABLE;

        return Status::ERROR;
    }
    
    return Status::OK;
}

int sj::API_RESERVED::Socket::getFD(){
    return socket_fd;
}

void sj::API_RESERVED::Socket::asignFD(const int fd){
    socket_fd = fd;
}

sj::Mode sj::API_RESERVED::Socket::getMode(){
    return mode;
}

int sj::API_RESERVED::Socket::close(){
    const int closeValue = ::close(socket_fd);
    if(closeValue == -1)
        socket_fd = -1;

    return closeValue;
}
//Socket Class

//TCPClientSocket Class
sj::TCPClientSocket::TCPClientSocket(const Mode mode) : socket(mode)
{}

sj::TCPClientSocket::~TCPClientSocket(){
    disconnect();
}

sj::Status sj::TCPClientSocket::connect(const std::string& ipAddress, const short port){
    if(isConnected())
        return Status::ERROR;

    if(socket.create(Socket::Type::TCP) == -1)
        return Status::ERROR;

    sockaddr_in addr;
    addr.sin_port = htons(port);
    addr.sin_family = AF_INET;
    if(inet_aton(ipAddress.c_str(), &addr.sin_addr) == -1){
        socket.close();
        return Status::ERROR;
    }

    if(::connect(socket.getFD(), (sockaddr*)&addr, sizeof(addr)) == -1){
        socket.close();
        return Status::ERROR;
    }

    return Status::OK;
}

sj::Status sj::TCPClientSocket::disconnect(){
    if(isConnected() == false)
        return Status::ERROR;

    Status returnStatus = Status::OK;
    
    if(shutdown(socket.getFD(), SHUT_RDWR) == -1)
        returnStatus = Status::ERROR;

    if(socket.close() == -1)
        returnStatus = Status::ERROR;

    return returnStatus;
}

sj::Status sj::TCPClientSocket::send(DataPacket& dataPacket){
    if(isConnected() == false)
        return Status::ERROR;

    return socket.sendTo(dataPacket);
}
sj::Status sj::TCPClientSocket::send(const void* data, const size_t dataSize){
    if(isConnected() == false)
        return Status::ERROR;

    return socket.sendTo(data, dataSize);
}

sj::Status sj::TCPClientSocket::receiveInto(DataPacket& dataPacket){
    if(isConnected() == false)
        return Status::ERROR;

    return socket.receiveInto(dataPacket);
}
sj::Status sj::TCPClientSocket::receiveInto(void* buffer, const size_t bufferSize, size_t* readedBytes){
    if(isConnected() == false)
        return Status::ERROR;

    return socket.receiveInto(buffer, bufferSize, readedBytes);
}

bool sj::TCPClientSocket::isConnected(){
    return socket.getFD() != -1;
}
//TCPClientSocket

//TCPListenSocket Class
sj::TCPListenSocket::TCPListenSocket(const Mode mode) : socket(mode) 
{}

sj::TCPListenSocket::~TCPListenSocket(){
    endListening();
}

sj::Status sj::TCPListenSocket::beginListening(const short port){
    if(isListening())
        return Status::ERROR;

    if(socket.create(Socket::Type::TCP) == -1) 
        return Status::ERROR;

    if(socket.bind(port) == -1){
        socket.close();
        return Status::ERROR;
    }
    
    const size_t maxListenConnections = SOMAXCONN;//128
    if(listen(socket.getFD(), maxListenConnections) == -1){
        socket.close();
        return Status::ERROR;
    }

    return Status::OK;
}

sj::Status sj::TCPListenSocket::endListening(){
    if(isListening() == false)
        return Status::ERROR;

    Status returnStatus = Status::OK;
    
    if(::shutdown(socket.getFD(), SHUT_RDWR) == -1)
        returnStatus = Status::ERROR;

    if(socket.close() == -1)
        returnStatus = Status::ERROR;

    return returnStatus;
}

sj::Status sj::TCPListenSocket::acceptNewClient(TCPClientSocket& newClient){
    if(isListening() == false || newClient.isConnected())
        return Status::ERROR;

    Mode mode = socket.getMode();
    const int acceptStatus = accept4(socket.getFD(), NULL, NULL, mode == Mode::NON_BLOCKING ? SOCK_NONBLOCK : 0);

    if(acceptStatus == -1){
        if(mode == Mode::NON_BLOCKING && (errno == EAGAIN || errno == EWOULDBLOCK))
            return Status::UNAVAILABLE;

        return Status::ERROR;
    }

    newClient.socket.asignFD(acceptStatus);
    return Status::OK;
}

bool sj::TCPListenSocket::isListening(){
    return socket.getFD() != -1;
}
//TCPListenSocket Class

//UDPSocket Class
sj::UDPSocket::UDPSocket(const Mode mode) : socket(mode) {
    
}

sj::Status sj::UDPSocket::bind(const short port){
    if(isBinded())
        return Status::ERROR;

    if(socket.create(Socket::Type::UDP) == -1) 
        return Status::ERROR;

    if(socket.bind(port) == -1){
        socket.close();
        return Status::ERROR;
    }

    return Status::OK;
}

sj::Status sj::UDPSocket::unbind(){
    if(isBinded() == false)
        return Status::ERROR;

    if(socket.close())
        return Status::ERROR;

    return Status::OK;
}

sj::UDPSocket::~UDPSocket(){
    unbind();
}

sj::Status sj::UDPSocket::sendTo(DataPacket& dataPacket, const std::string& ipAddress, const short port){
    if(isBinded() == false)
        return Status::ERROR;

    return socket.sendTo(dataPacket, &ipAddress, port);
}

sj::Status sj::UDPSocket::sendTo(const void* data, const size_t dataSize, const std::string& ipAddress, const short port){
    if(isBinded() == false)
        return Status::ERROR;

    return socket.sendTo(data, dataSize, &ipAddress, port);
}

sj::Status sj::UDPSocket::receiveInto(DataPacket& dataPacket){
    if(isBinded() == false)
        return Status::ERROR;

    return socket.receiveInto(dataPacket);
}

sj::Status sj::UDPSocket::receiveInto(void* buffer, const size_t bufferSize, size_t* readedBytes){
    if(isBinded() == false)
        return Status::ERROR;

    return socket.receiveInto(buffer, bufferSize, readedBytes);
}

bool sj::UDPSocket::isBinded(){
    return socket.getFD() != -1;
}
//UDPSocket Class