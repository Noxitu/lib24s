#include "connection.hpp"

using namespace std;

struct BajError : public Error {
    protected:
        BajError(int code, char const *message) : Error(code, message) {}
    public:
        static const BajError no_knights;
        static const BajError no_enough_knights;
        static const BajError field_blocked;
};

BajError const BajError::no_knights(100, "No squad on this field belongs to you.");
BajError const BajError::no_enough_knights(103, "There are not enough knights on the field.");
BajError const BajError::field_blocked(104, "It is impossible to move onto this field.");


class Round {

};

shared_ptr<Round> gRound = nullptr;

class BajConnection : public Connection {
    private:
    protected:
        static CommandsMap command_map;
        CONNECTION_RUNCMD_METHOD;
    public:
        void DESCRIBE_WORLD(istream &in) {
            if( not gRound )
                throw Error::no_round;
            std::ostringstream sout;
            sout << 0 << ' ' << 0 << ' ' << 0 << ' ' << 0 << ' ' << 0;
            client->write(sout.str());
        }
        
        void DESCRIBE_SQUADS(istream &in) {
        }
        
        void DESCRIBE_COTTAGES(istream &in) {
        }
        
        void EXPLORE(istream &in) {
        }
        
        void MOVE_SQUAD(istream &in) {
        }
    
        void LIST_MY_BATTLES(istream &in) {
        }
    
        BajConnection(shared_ptr<Client> client) : Connection(client) {}
        
        static shared_ptr<Connection> create(size_t, PermissionLevel level, shared_ptr<Client> client) {
            if( level == PermissionLevel::User ) {
                return make_shared<BajConnection>(client);
            }
            return nullptr;
        }
};
Connection::CommandsMap BajConnection::command_map = {
    cmd("DESCRIBE_WORLD", &BajConnection::DESCRIBE_WORLD),
    cmd("DESCRIBE_SQUADS", &BajConnection::DESCRIBE_SQUADS),
    cmd("DESCRIBE_COTTAGES", &BajConnection::DESCRIBE_COTTAGES),
    cmd("EXPLORE", &BajConnection::EXPLORE),
    cmd("MOVE_SQUAD", &BajConnection::MOVE_SQUAD),
    cmd("LIST_MY_BATTLES", &BajConnection::LIST_MY_BATTLES)
};

extern "C" void run(ConnectionFactoryList &cfl) {
    cfl << BajConnection::create;
}