//
// Simple Network Sockets library for Windows and Linux

// Copyright (C) 2020  Jakub Stawiski

// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.

#include "SJNetSock.hpp"
#include <vector>
#include <cstring>
#include <array>
#include <algorithm>
//LINUX
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <netinet/ip.h>
#include <arpa/inet.h>
//LINUX

#define DATAPACKET_SIZE_T uint16_t
#define DATAPACKET_SIZE_T_MAX UINT16_MAX

using namespace sj::API_RESERVED;

///////////////////////////////////////////////
//  DataPacket Class
///////////////////////////////////////////////
sj::DataPacket::DataPacket() {

};


///////////////////////////////////////////////
sj::DataPacket::~DataPacket() {

};


///////////////////////////////////////////////
sj::Status sj::DataPacket::allDataReaded() {
    return data.size() == 0 ? Status::OK : Status::ERROR;
}


///////////////////////////////////////////////
sj::DataPacket& sj::DataPacket::operator<<(const std::int8_t value) {
    return operator<< <std::int8_t>(value); 
}


///////////////////////////////////////////////
sj::DataPacket& sj::DataPacket::operator<<(const std::int16_t value) { 
    return operator<< <std::int16_t>(value); 
}


///////////////////////////////////////////////
sj::DataPacket& sj::DataPacket::operator<<(const std::int32_t value) {
    return operator<< <std::int32_t>(value); 
}


///////////////////////////////////////////////
sj::DataPacket& sj::DataPacket::operator<<(const std::int64_t value) { 
    return operator<< <std::int64_t>(value); 
}


///////////////////////////////////////////////
sj::DataPacket& sj::DataPacket::operator<<(const std::uint8_t value) {
    return operator<< <std::uint8_t>(value); 
}


///////////////////////////////////////////////
sj::DataPacket& sj::DataPacket::operator<<(const std::uint16_t value) { 
    return operator<< <std::uint16_t>(value); 
}


///////////////////////////////////////////////
sj::DataPacket& sj::DataPacket::operator<<(const std::uint32_t value) {
    return operator<< <std::uint32_t>(value); 
}


///////////////////////////////////////////////
sj::DataPacket& sj::DataPacket::operator<<(const std::uint64_t value) { 
    return operator<< <std::uint64_t>(value); 
}


///////////////////////////////////////////////
sj::DataPacket& sj::DataPacket::operator<<(const std::string& value) {
    for(auto strIt = value.begin(); strIt != value.end(); strIt++)
        operator<<( (std::int8_t)(*strIt) );

    operator<<( (std::int8_t)('\0') );
    return (*this);
}


///////////////////////////////////////////////
sj::DataPacket& sj::DataPacket::operator>>(std::int8_t& value) { 
    return operator>> <std::int8_t>(value); 
}


///////////////////////////////////////////////
sj::DataPacket& sj::DataPacket::operator>>(std::int16_t& value) { 
    return operator>> <std::int16_t>(value); 
}


///////////////////////////////////////////////
sj::DataPacket& sj::DataPacket::operator>>(std::int32_t& value) { 
    return operator>> <std::int32_t>(value); 
}


///////////////////////////////////////////////
sj::DataPacket& sj::DataPacket::operator>>(std::int64_t& value) { 
    return operator>> <std::int64_t>(value); 
}


///////////////////////////////////////////////
sj::DataPacket& sj::DataPacket::operator>>(std::uint8_t& value) { 
    return operator>> <std::uint8_t>(value); 
}


///////////////////////////////////////////////
sj::DataPacket& sj::DataPacket::operator>>(std::uint16_t& value) { 
    return operator>> <std::uint16_t>(value); 
}


///////////////////////////////////////////////
sj::DataPacket& sj::DataPacket::operator>>(std::uint32_t& value) { 
    return operator>> <std::uint32_t>(value); 
}


///////////////////////////////////////////////
sj::DataPacket& sj::DataPacket::operator>>(std::uint64_t& value) { 
    return operator>> <std::uint64_t>(value); 
}


///////////////////////////////////////////////
sj::DataPacket& sj::DataPacket::operator>>(std::string& value) {
    char currentChar = '\0';
    value.clear();
    do{
        operator>>( (std::int8_t&)currentChar);
        if(currentChar != '\0')
            value.insert(value.end(), currentChar);
    } while (currentChar != '\0');
    
    return (*this);
}


///////////////////////////////////////////////
//  Socket class
///////////////////////////////////////////////
sj::API_RESERVED::Socket::Socket(const Mode mode) 
    : socket_fd(-1), mode(mode), dataPacketsBuffer(0) {

}


///////////////////////////////////////////////
sj::API_RESERVED::Socket::~Socket() {

}


///////////////////////////////////////////////
int sj::API_RESERVED::Socket::create(const Socket::Type type) {
    socket_fd = ::socket(AF_INET, type == Type::TCP ? SOCK_STREAM : SOCK_DGRAM, 0);
    return socket_fd;
}


///////////////////////////////////////////////
int sj::API_RESERVED::Socket::bind(const short port) {
    sockaddr_in addr;
    addr.sin_port = htons(port);
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    return ::bind(socket_fd, (sockaddr*)&addr, sizeof(addr));
}


///////////////////////////////////////////////
sj::Status sj::API_RESERVED::Socket::receiveInto(DataPacket& dataPacket) {
    do{
        //Check if already any bytes of packet is stored in buffer
        if(dataPacketsBuffer.size() >= sizeof(DATAPACKET_SIZE_T)){
            DATAPACKET_SIZE_T packetSizeInBuffer;//Packet size
            
            //Get packet size value
            size_t currentByte = 0;
            auto it = dataPacketsBuffer.begin();
            while(currentByte < sizeof(packetSizeInBuffer)){
                char* const packetSizeInBufferPtr = (char*) &packetSizeInBuffer;
                *(packetSizeInBufferPtr + currentByte) = *it;

                currentByte++;
                it++;
            }

            //Check if whole packet is already in buffer...
            if(packetSizeInBuffer >= (dataPacketsBuffer.size() - sizeof(packetSizeInBuffer)) ){
                //... if it is, read it and exit receive function
                while(currentByte < (packetSizeInBuffer + sizeof(packetSizeInBuffer))){
                    dataPacket.data.push(*it);

                    currentByte++;
                    it++;
                }

                dataPacketsBuffer.erase(dataPacketsBuffer.begin(), it);
                return Status::OK;
            }
        }

        //if buffor dont have full packet in it,
        //try to receive some data to it
        std::array<char, DATAPACKET_SIZE_T_MAX + sizeof(DATAPACKET_SIZE_T)> receiveBuffor;
        size_t readedBytes = 0;
        Status status = receiveInto(receiveBuffor.data(), receiveBuffor.max_size(), &readedBytes);
        if(status != Status::OK)//When mode is non-blocking, function return here
            return status;

        std::copy_n(receiveBuffor.begin(), readedBytes-1, dataPacketsBuffer.end());
    } while(true);//Repeat until any full packet will be available in packet buffor
}


///////////////////////////////////////////////
sj::Status sj::API_RESERVED::Socket::receiveInto(void* buffer, const size_t bufferSize, size_t* readedBytes) {
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


///////////////////////////////////////////////
sj::Status sj::API_RESERVED::Socket::sendTo(DataPacket& dataPacket, const std::string* ipAddress, const short port) {
    const DATAPACKET_SIZE_T dataSize = (DATAPACKET_SIZE_T) dataPacket.data.size();
    std::vector<char> buffer(dataSize);

    //Put packet size into buffer
    memcpy(buffer.data(), &dataSize, sizeof(dataSize));
    //Put packet data into buffer
    for(size_t i=0; i < dataPacket.data.size(); i++){
        buffer[i + sizeof(dataSize)] = dataPacket.data.front();
        dataPacket.data.pop();
    }

    return sendTo(buffer.data(), buffer.size(), ipAddress, port);
}


///////////////////////////////////////////////
sj::Status sj::API_RESERVED::Socket::sendTo(const void* data, const size_t dataSize, const std::string* ipAddress, const short port) {

    ssize_t sendStatus = -1;
    //UDP with receiver IP addres
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
    //TCP with connected receiver
    else
        sendStatus = ::send(socket_fd, data, dataSize, mode == Mode::NON_BLOCKING ? MSG_DONTWAIT : 0);

    if(sendStatus == -1){
        if(mode == Mode::NON_BLOCKING && (errno == EAGAIN || errno == EWOULDBLOCK))
            return Status::UNAVAILABLE;

        return Status::ERROR;
    }
    
    return Status::OK;
}


///////////////////////////////////////////////
int sj::API_RESERVED::Socket::getFD() {
    return socket_fd;
}


///////////////////////////////////////////////
void sj::API_RESERVED::Socket::asignFD(const int fd) {
    socket_fd = fd;
}


///////////////////////////////////////////////
sj::Mode sj::API_RESERVED::Socket::getMode() {
    return mode;
}


///////////////////////////////////////////////
int sj::API_RESERVED::Socket::close() {
    const int closeValue = ::close(socket_fd);
    if(closeValue != -1)
        socket_fd = -1;

    return closeValue;
}


///////////////////////////////////////////////
//  TCPClientSocket Class
///////////////////////////////////////////////
sj::TCPClientSocket::TCPClientSocket(const Mode mode) 
    : socket(mode) {

}


///////////////////////////////////////////////
sj::TCPClientSocket::~TCPClientSocket() {
    disconnect();
}


///////////////////////////////////////////////
sj::Status sj::TCPClientSocket::connect(const std::string& ipAddress, const short port) {
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


///////////////////////////////////////////////
sj::Status sj::TCPClientSocket::disconnect() {
    if(isConnected() == false)
        return Status::ERROR;

    Status returnStatus = Status::OK;
    
    if(shutdown(socket.getFD(), SHUT_RDWR) == -1)
        returnStatus = Status::ERROR;

    if(socket.close() == -1)
        returnStatus = Status::ERROR;

    return returnStatus;
}


///////////////////////////////////////////////
sj::Status sj::TCPClientSocket::send(DataPacket& dataPacket) {
    if(isConnected() == false)
        return Status::ERROR;

    return socket.sendTo(dataPacket);
}


///////////////////////////////////////////////
sj::Status sj::TCPClientSocket::send(const void* data, const size_t dataSize) {
    if(isConnected() == false)
        return Status::ERROR;

    return socket.sendTo(data, dataSize);
}


///////////////////////////////////////////////
sj::Status sj::TCPClientSocket::receiveInto(DataPacket& dataPacket) {
    if(isConnected() == false)
        return Status::ERROR;

    return socket.receiveInto(dataPacket);
}


///////////////////////////////////////////////
sj::Status sj::TCPClientSocket::receiveInto(void* buffer, const size_t bufferSize, size_t* readedBytes) {
    if(isConnected() == false)
        return Status::ERROR;

    return socket.receiveInto(buffer, bufferSize, readedBytes);
}


///////////////////////////////////////////////
bool sj::TCPClientSocket::isConnected() {
    return socket.getFD() != -1;
}


///////////////////////////////////////////////
//  TCPListenSocket Class
///////////////////////////////////////////////
sj::TCPListenSocket::TCPListenSocket(const Mode mode) 
    : socket(mode) {

}


///////////////////////////////////////////////
sj::TCPListenSocket::~TCPListenSocket() {
    endListening();
}


///////////////////////////////////////////////
sj::Status sj::TCPListenSocket::beginListening(const short port) {
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


///////////////////////////////////////////////
sj::Status sj::TCPListenSocket::endListening() {
    if(isListening() == false)
        return Status::ERROR;

    Status returnStatus = Status::OK;
    
    if(::shutdown(socket.getFD(), SHUT_RDWR) == -1)
        returnStatus = Status::ERROR;

    if(socket.close() == -1)
        returnStatus = Status::ERROR;

    return returnStatus;
}


///////////////////////////////////////////////
sj::Status sj::TCPListenSocket::acceptNewClient(TCPClientSocket& newClient) {
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


///////////////////////////////////////////////
bool sj::TCPListenSocket::isListening() {
    return socket.getFD() != -1;
}


///////////////////////////////////////////////
//  UDPSocket Class
///////////////////////////////////////////////
sj::UDPSocket::UDPSocket(const Mode mode) 
    : socket(mode) {

}


///////////////////////////////////////////////
sj::UDPSocket::~UDPSocket() {
    unbind();
}


///////////////////////////////////////////////
sj::Status sj::UDPSocket::bind(const short port) {
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


///////////////////////////////////////////////
sj::Status sj::UDPSocket::unbind() {
    if(isBinded() == false)
        return Status::ERROR;

    if(socket.close())
        return Status::ERROR;

    return Status::OK;
}


///////////////////////////////////////////////
sj::Status sj::UDPSocket::sendTo(DataPacket& dataPacket, const std::string& ipAddress, const short port) {
    if(isBinded() == false)
        return Status::ERROR;

    return socket.sendTo(dataPacket, &ipAddress, port);
}


///////////////////////////////////////////////
sj::Status sj::UDPSocket::sendTo(const void* data, const size_t dataSize, const std::string& ipAddress, const short port) {
    if(isBinded() == false)
        return Status::ERROR;

    return socket.sendTo(data, dataSize, &ipAddress, port);
}


///////////////////////////////////////////////
sj::Status sj::UDPSocket::receiveInto(DataPacket& dataPacket) {
    if(isBinded() == false)
        return Status::ERROR;

    return socket.receiveInto(dataPacket);
}


///////////////////////////////////////////////
sj::Status sj::UDPSocket::receiveInto(void* buffer, const size_t bufferSize, size_t* readedBytes) {
    if(isBinded() == false)
        return Status::ERROR;

    return socket.receiveInto(buffer, bufferSize, readedBytes);
}


///////////////////////////////////////////////
bool sj::UDPSocket::isBinded() {
    return socket.getFD() != -1;
}