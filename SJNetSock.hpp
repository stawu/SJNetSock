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

#pragma once

#include <string>
#include <queue>
#include <list>
#include <cstdint>

namespace sj{

enum struct Status{
    ERROR,
    OK,
    //Used when NON_BLOCKING Mode is selected.
    //Meaning: Current operation (ex: sending, receiving) at this moment cannot be done.
    //Example: Trying to receive when none data is available,
    //         Trying to send when socket buffer is full.
    UNAVAILABLE
};

enum struct Mode{
    BLOCKING,
    NON_BLOCKING
};

namespace API_RESERVED { class Socket; }

class DataPacket {
    public:
        DataPacket();

        ~DataPacket();

        Status allDataReaded();

        //ADD compress
        //ADD encrypt

        DataPacket& operator<<(const std::int8_t value);
        DataPacket& operator<<(const std::int16_t value);
        DataPacket& operator<<(const std::int32_t value);
        DataPacket& operator<<(const std::int64_t value);
        DataPacket& operator<<(const std::uint8_t value);
        DataPacket& operator<<(const std::uint16_t value);
        DataPacket& operator<<(const std::uint32_t value);
        DataPacket& operator<<(const std::uint64_t value);
        DataPacket& operator<<(const std::string& value);

        DataPacket& operator>>(std::int8_t& value);
        DataPacket& operator>>(std::int16_t& value);
        DataPacket& operator>>(std::int32_t& value);
        DataPacket& operator>>(std::int64_t& value);
        DataPacket& operator>>(std::uint8_t& value);
        DataPacket& operator>>(std::uint16_t& value);
        DataPacket& operator>>(std::uint32_t& value);
        DataPacket& operator>>(std::uint64_t& value);
        DataPacket& operator>>(std::string& value);

    private:
        friend class API_RESERVED::Socket;//accesing 'data' in 'send'

        std::queue<char> data;

        template<typename T> 
        DataPacket& operator<<(const T& value){
            const char* firstBytePtr = (char*) &value;
            for(size_t currentByte = 0; currentByte < sizeof(value); currentByte++){
                const char byteValue = *(firstBytePtr + currentByte);
                data.push(byteValue);
            }

            return *this;
        }

        template<typename T> 
        DataPacket& operator>>(T& value){
            if(data.size() != 0){
                char* firstBytePtr = (char*) &value;
                for(size_t currentByte = 0; currentByte < sizeof(value); currentByte++){
                    const char byteValue = data.front();
                    data.pop();
                    *(firstBytePtr + currentByte) = byteValue;
                }
            }
            
            return *this;
        }
};

namespace API_RESERVED {
class Socket {
    public:
        enum Type{
            TCP,
            UDP
        };

        Socket(const Mode mode);
        ~Socket();
        int create(const Type type);
        int bind(const short port);
        Status receiveInto(DataPacket& dataPacket);
        Status receiveInto(void* buffer, const size_t bufferSize, size_t* readedBytes);
        Status sendTo(DataPacket& dataPacket, const std::string* ipAddress = nullptr, const short port = -1);
        Status sendTo(const void* data, const size_t dataSize, const std::string* ipAddress = nullptr, const short port = -1);
        int getFD();
        void asignFD(const int fd);
        Mode getMode();
        int close();

    private:
        int socket_fd;
        Mode mode;
        std::list<char> dataPacketsBuffer;
};
}

class TCPClientSocket {
    public:
        TCPClientSocket(const Mode mode);

        ~TCPClientSocket();

        Status connect(const std::string& ipAddress, const short port);

        Status disconnect();

        Status send(DataPacket& dataPacket);

        //If DataPacket is used, it's not recommended to use
        //low level send(const* void data, ...) because DataPackets 
        //lost may occur.
        Status send(const void* data, const size_t dataSize);

        Status receiveInto(DataPacket& dataPacket);

        //If DataPacket is used, it's not recommended to use
        //low level receiveInto(const* void buffer, ...) because DataPackets 
        //lost may occur.
        Status receiveInto(void* buffer, const size_t bufferSize, size_t* readedBytes);

        bool isConnected();

    private:
        friend class TCPListenSocket;//accesing 'socket' in 'acceptNewClient'

        API_RESERVED::Socket socket;
};

class TCPListenSocket {
    public:
        TCPListenSocket(const Mode mode);

        ~TCPListenSocket();

        Status beginListening(const short port);
        
        Status endListening();

        Status acceptNewClient(TCPClientSocket& newClient);

        bool isListening();

    private:
        API_RESERVED::Socket socket;
};

class UDPSocket {
    public:
        UDPSocket(const Mode mode);

        ~UDPSocket();

        Status bind(const short port);
        
        Status unbind();

        Status sendTo(DataPacket& dataPacket, const std::string& ipAddress, const short port);

        //If DataPacket is used, it's not recommended to use
        //low level send(const* void data, ...) because DataPackets 
        //lost may occur.
        Status sendTo(const void* data, const size_t dataSize, const std::string& ipAddress, const short port);

        Status receiveInto(DataPacket& dataPacket);

        //If DataPacket is used, it's not recommended to use
        //low level receiveInto(const* void buffer, ...) because DataPackets 
        //lost may occur.
        Status receiveInto(void* buffer, const size_t bufferSize, size_t* readedBytes);

        bool isBinded();

    private:
        API_RESERVED::Socket socket;
};
}//namespace sj