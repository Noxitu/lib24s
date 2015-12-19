#include "connection.hpp"
#include "round.hpp"
#include <bits/stdc++.h>

struct OIXError : public Error {
    protected:
    public:
        OIXError(int code, char const *message) : Error(code, message) {}
};

struct Player {
    size_t id;
    size_t active_connections = 0;
    
    weak_ptr<Faction> faction;
    weak_ptr<Round> round;
    Player(size_t id) : id(id) {}
    std::condition_variable waiting;
};

class Game {
    private:
        map<size_t, shared_ptr<Player>> players;
        shared_ptr<thread> game_loop;
    public:
        std::mutex mutex;
        
        Game() {
            game_loop = make_shared<thread>(Game::gameLoop, this);
        }
        
        shared_ptr<Player> getPlayer(size_t id) {
            shared_ptr<Player> p = players[id];
            if( not p ) {
                players[id] = p = make_shared<Player>(id);
            }
            return p;
        }
        
        void roundThread(shared_ptr<Round> round, vector<shared_ptr<Player>> players) {
            round->roundLoop();
            unique_lock<std::mutex> _(mutex);
            for( auto p : players )
                if( p->faction.lock() == round->winner )
                    cout << "Player " << p->id << " won!" << endl;
        }
        
        void createRound(vector<shared_ptr<Player>> players) {
            shared_ptr<Round> round = make_shared<Round>();
            for( int i = 0; i < 2; i++ ) {
                players[i]->round = round;
                players[i]->faction = round->factions[i];
                players[i]->waiting.notify_all();
            }
            thread t(Game::roundThread, this, round, move(players));
            t.detach();
        }
        
        void gameLoop() {
            while(true) {
                sleep(3);
                unique_lock<std::mutex> _(std::mutex);
                //cout << "Game::Turn End" << endl; 
                
                vector<shared_ptr<Player>> unmatched;
                for( auto p : players )
                    if( p.second->round.expired() and p.second->active_connections > 0 )
                        unmatched.push_back(p.second);
                        
                shuffle(begin(unmatched), end(unmatched), default_random_engine(std::chrono::system_clock::now().time_since_epoch().count()));
                
                vector<shared_ptr<Player>> matched;
                for( auto p : unmatched ) {
                    matched.push_back(p);
                    if( matched.size() == 2 ) {
                        createRound(matched);
                        matched.clear();
                    }
                }
            }
        }
};

shared_ptr<Game> gGame;

class OIXConnection : public Connection {
    private:
        shared_ptr<Player> player;
        
        shared_ptr<Round> requireRound() const {
            shared_ptr<Round> round = player->round.lock();
            if( not round or round->ended )
                throw Error::no_round;
            return round;
        }
        
    protected:
        static CommandsMap command_map;
        CONNECTION_RUNCMD_METHOD;
    public:
        void DESCRIBE(istream &in) {
            auto round = requireRound();
            std::ostringstream sout;
            {
                unique_lock<mutex> _(round->round_mutex);
                requireRound();
                auto faction = player->faction.lock();
                 
                sout << "OK\n";
                sout << (char)faction->type << '\n';
                for( int y = 0; y < 3; y++ ) {
                    for( int x = 0; x < 3; x++ )
                        sout << (char) round->board[y][x];
                    sout << '\n';
                }
            }
            client->write(sout.str());    
        }
        
        void MOVE(istream &in) {
            auto round = requireRound();
            {
                unique_lock<mutex> _(round->round_mutex);
                requireRound();
                Coords c;
                in >> c;
                validate_stream(in);
                
                auto faction = player->faction.lock();
                if( not round->canMove(faction) )
                    throw OIXError(101, "Not your turn.");
                    
                if( c.x < 0 or c.y < 0 or c.x >= 3 or c.y >= 3 )
                    throw OIXError(102, "Coords outside of board.");
                    
                if( round->board[c.y][c.x] != FieldContent::None )
                    throw OIXError(103, "Field is occupied.");
                
                faction->move = make_shared<Coords>(c);
            }
            client->write("OK\n");
        }
        
        void WAIT(istream &in) {
            shared_ptr<Round> round;
            {
                unique_lock<mutex> lock(gGame->mutex);
                round = player->round.lock();
                if( not round ) {
                    client->write("OK\nWAITING\n");
                    player->waiting.wait(lock);
                    client->write("OK\n");
                    return;
                }
            }
            unique_lock<mutex> lock(round->round_mutex);
            requireRound();
            
            client->write("OK\nWAITING\n");
            player->faction.lock()->waiting.wait(lock);
            client->write("OK\n");
        }
    
        OIXConnection(shared_ptr<Client> client, size_t id) : Connection(client) {
            unique_lock<mutex> lock(gGame->mutex);
            player = gGame->getPlayer(id);
            if( player->active_connections >= 2 )
                throw Error::too_many_connections;
                
            player->active_connections++;
        }
        
        ~OIXConnection() {
            unique_lock<mutex> lock(gGame->mutex);
            player->active_connections--;
        }
        
        static shared_ptr<Connection> create(size_t id, PermissionLevel level, shared_ptr<Client> client) {
            if( level == PermissionLevel::User ) {
                return make_shared<OIXConnection>(client, id);
            }
            return nullptr;
        }
};
Connection::CommandsMap OIXConnection::command_map = {
    cmd("WAIT", &OIXConnection::WAIT),
    cmd("MOVE", &OIXConnection::MOVE),
    cmd("DESCRIBE", &OIXConnection::DESCRIBE)
};

extern "C" void run(ConnectionFactoryList &cfl) {
    cfl << OIXConnection::create;
    gGame = make_shared<Game>();
}