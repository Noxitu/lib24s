#include "auth.hpp"
#include "configparser.hpp"
#include "connection.hpp"
#include "error.hpp"
#include "listener.hpp"
#include "soloader.hpp"
#include <bits/stdc++.h>

using namespace std;

shared_ptr<Auth> gAuth;
shared_ptr<Config> gConfig;


void connection(shared_ptr<Client> client, ConnectionFactory factory) {
    try {
        client->write("LOGIN\n");
        std::string username = client->read();
        client->write("PASSWORD\n");
        std::string password = client->read();
        Auth::UserData data = gAuth->find_user(username, password);
        
        if( data.level == PermissionLevel::None ) {
            throw Error::invalid_auth;
        }
        shared_ptr<Connection> conn = factory(data.id, data.level, client);
        conn->OK();
        
        while( true ) {
            try {
                conn->read();
            } catch(Error const &error) {
                client << error;
            }
        }
    } catch(Error const &error) {
        client << error;
    } catch(Client::Disconnected const &exc) {
        clog << "Someone disconnected: " << exc.what() << endl;
    } catch(std::exception const &exc) {
        try {
            client << Error::internal;
        } catch(...) {}
        clog << "Unknown exception throwed: " << typeid(exc).name() << '(' << exc.what() << ')' << endl;
    } catch(char const *& str) {
        try {
            client << Error::internal;
        } catch(...) {}
        clog << "Unknown 'exception' throwed: " << str << endl;
    } catch(...) {
        try {
            client << Error::internal;
        } catch(...) {}
        clog << "Unknown 'exception' throwed." << endl;
        throw;
    }
}

class AdminConnection : public Connection {
    private:
    protected:
        static CommandsMap command_map;
        CONNECTION_RUNCMD_METHOD;
    public:
        void RELOAD_USERS(std::istream&) {
            auto new_users = make_shared<FileAuth>("./users.txt");
            OK();
            gAuth = new_users;
        }
        
        AdminConnection(shared_ptr<Client> client) : Connection(client) {}
        
        static shared_ptr<Connection> create(size_t, PermissionLevel level, shared_ptr<Client> client) {
            if( level == PermissionLevel::Op )
                return std::make_shared<AdminConnection>(client);
            return nullptr;
        }
};
Connection::CommandsMap AdminConnection::command_map = {
    cmd("RELOAD_USERS", &AdminConnection::RELOAD_USERS)
};

int main() {
    //gConfig = make_shared<Config>("./config.txt");
    //cout << *gConfig << endl;
    gAuth = make_shared<FileAuth>("./users.txt");
    
    TCPListener listener;
    ConnectionFactoryList conn_factory;
    conn_factory << AdminConnection::create;
    load_so("./oix.so")(conn_factory);
    
    while(true) {
        shared_ptr<Client> client = listener.accept();
        thread t(connection, client, conn_factory);
        t.detach();
    }
}