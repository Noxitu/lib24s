#include "auth.hpp"
#include "error.hpp"
#include "listener.hpp"
#include <bits/stdc++.h>

#define CONNECTION_RUNCMD_METHOD void runcmd(std::string const& name, std::istream& in) override { \
    auto it = command_map.find(name); \
    if( it == end(command_map) ) \
        throw Error::unknown_command; \
    (this->*(it->second))(in); \
}

class Connection {
    public:
        typedef void(Connection::*Command)(std::istream&);
    private:
    protected:
        std::shared_ptr<Client> client;
        typedef std::unordered_map<std::string, Command> CommandsMap;
        virtual void runcmd(std::string const&, std::istream&) {
            throw std::runtime_error("Connection::cmd nor Connection::read overriden.");
        }
    public:
        void OK() {
            client->write("OK");
        }
        Connection(std::shared_ptr<Client> client) : client(client) {}
        
        virtual void read() {
            std::istringstream sin(client->read());
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
