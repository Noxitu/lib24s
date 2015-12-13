#pragma once
#include <bits/stdc++.h>

class Client {
    private:
    public:
        struct Disconnected : public std::runtime_error{ Disconnected(char const *what) : std::runtime_error(what) {} };
        virtual void write(std::string const &data) = 0;
        virtual std::string read() = 0;
        virtual ~Client() {}
};

class Listener {
    private:
    public:
        virtual std::shared_ptr<Client> accept() = 0;
};


#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

class TCPClient : public Client {
    private:
        int const socket;
        
        char _buffer[4096];
        char *_buffer_critical = _buffer+sizeof(_buffer)/2;
        char *_buffer_a = _buffer;
        char *_buffer_b = _buffer;
    public:
        TCPClient(int socket) :
            socket(socket)
        {
        }
        void write(std::string const &data) override {
            int ret = ::write(socket, data.c_str(), data.size());
            if( ret != data.size() )
                throw Disconnected("Couldn't write socket.");
        };
        
        std::string read() override {
            char *pos = std::find( _buffer_a, _buffer_b, '\n' );
            if( pos == _buffer_b and _buffer_a >= _buffer_critical ) {
                std::copy( _buffer_a, _buffer_b, _buffer );
                _buffer_b -= std::distance(_buffer_a, _buffer_b);
                _buffer_a = _buffer;
                pos = _buffer_b;
            }
            while( pos == _buffer_b and _buffer_b != std::end(_buffer) ) {
                int ret = ::read(socket, _buffer_b, std::distance(_buffer_b, std::end(_buffer)));
                
                if( ret == 0 )
                    throw Disconnected("Disconnected.");
                if( ret < 0 )
                    throw Disconnected("Couldn't read socket.");
                    
                _buffer_b += ret;
                
                pos = std::find( pos, _buffer_b, '\n' );
            }
            if( pos == std::end(_buffer) )
                throw std::runtime_error("Buffer size exceed.");
            
            std::string ret = std::string(_buffer_a, std::distance(_buffer_a, pos));
            if( not ret.empty() and ret.back() == '\r' ) ret.pop_back();
            _buffer_a = pos+1;
            
            return ret;
        };
        
        ~TCPClient() {
            close(socket);
        }
};

class TCPListener : public Listener {
    private:
        int const socket;
    public:
        TCPListener() :
            socket(::socket(AF_INET, SOCK_STREAM, 0))
        {
            if( socket < 0 )
                throw std::runtime_error("Couldn't create socket.");
            
            sockaddr_in addr;
            bzero((char *) &addr, sizeof(addr));
            addr.sin_family = AF_INET;
            addr.sin_addr.s_addr = INADDR_ANY;
            addr.sin_port = htons(8000);
            
            int ret = bind(socket, (sockaddr*) &addr, sizeof(addr));
            if( ret < 0 )
                throw std::runtime_error("Couldn't bind socket.");
                
            listen(socket, 5);
            std::clog << "Listening on port 8000..." << std::endl;
        }
        
        std::shared_ptr<Client> accept() override {
            sockaddr_in addr;
            socklen_t socklen = sizeof(addr);
            int client = ::accept(socket, (sockaddr*) &addr, &socklen);
            if( client < 0 )
                throw std::runtime_error("Couldn't accept socket.");
                
            char const *ip = inet_ntoa(addr.sin_addr);
            std::clog << "Accepted connection from " << ip << std::endl;
            
            return std::make_shared<TCPClient>(client);
        }
};
