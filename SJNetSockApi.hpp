 //Created by Jakub Stawiski

#pragma once
#include <string>

extern enum Status;

//Its not recommended to use anything from this namespace,
//content is reserved to be use only by API
namespace api_reserved{
class Socket{
    public:
        Status send(const void* data, const size_t dataSize);
        Status receiveInto(void* buffer, const size_t bufferSize, size_t* readedBytes);
    protected:
        Socket(const Mode mode);
        ~Socket();
    private:
        Mode mode;
        int socket_fd;
};
}//namespace api 
