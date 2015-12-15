#include "auth.hpp"
#include "error.hpp"
#include "listener.hpp"
#include <bits/stdc++.h>
#include <chrono>

#define CONNECTION_RUNCMD_METHOD void runcmd(std::string const& name, std::istream& in) override { \
    auto it = command_map.find(name); \
    if( it == end(command_map) ) \
        throw Error::unknown_command; \
    (this->*(it->second))(in); \
}

class Connection {
    public:
        typedef void(Connection::*Command)(std::istream&);
        static void validate_stream(std::istream const &in) {
            if( in.bad() )
                throw Error::internal;
            if( not in.eof() ) {
                if( in.fail() )
                    throw Error::invalid_arg_syntax;
                else
                    throw Error::wrong_arg_num;
            }
            if( in.fail() )
                throw Error::wrong_arg_num;
        }
    private:
    protected:
        std::shared_ptr<Client> client;
        typedef std::unordered_map<std::string, Command> CommandsMap;
        virtual void runcmd(std::string const&, std::istream&) {
            throw std::runtime_error("Connection::cmd nor Connection::read overriden.");
        }
    public:
        void OK() {
            client->write("OK\n");
        }
        Connection(std::shared_ptr<Client> client) : client(client) {}
        ~Connection() {}
        
        virtual void read() {
            static auto prev = std::chrono::high_resolution_clock::now();
            std::chrono::duration<double> time_span = std::chrono::duration_cast<std::chrono::duration<double>>(std::chrono::high_resolution_clock::now() - prev);
            std::cout << time_span.count() << std::endl;
            std::istringstream sin(client->read());
            prev = std::chrono::high_resolution_clock::now();
            std::string name;
            sin >> name;
            runcmd(name, sin);
        };
};

template<class T>
std::pair<std::string, Connection::Command> cmd(std::string const &name, void(T::*func)(std::istream&)) {
    return make_pair( name, (Connection::Command) func ); 
}

typedef std::function<std::shared_ptr<Connection>(size_t, PermissionLevel, std::shared_ptr<Client>)> ConnectionFactory;

class ConnectionFactoryList {
    private:
        std::list<ConnectionFactory> storage;
    public:
        std::shared_ptr<Connection> operator()(size_t id, PermissionLevel level, std::shared_ptr<Client> client) {
            for( auto func : storage ) {
                std::shared_ptr<Connection> ptr = func(id, level, client);
                if( ptr )
                    return ptr;
            }
            throw std::runtime_error("Couldn't create Connection.");
        }
        friend ConnectionFactoryList& operator<<(ConnectionFactoryList& cfl, ConnectionFactory factory) {
            cfl.storage.push_back(factory);
            return cfl;
        }
};
