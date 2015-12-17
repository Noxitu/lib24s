#include "connection.hpp"
#include "round.hpp"
#include <bits/stdc++.h>

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

class Game {
    private:
        map<size_t, shared_ptr<Player>> players;
        shared_ptr<thread> game_loop;
    public:
        std::mutex mutex;
        
        shared_ptr<Round> round = nullptr;
        
        Game() {
            game_loop = make_shared<thread>(Game::gameLoop, this);
        }
        
        void createRound() {
            round = Round::generateRandomRound();
            
            for( auto &p : players )
                if( p.second->active_connections ) 
                    round->assignFaction(p.second);
        }
        
        void endRound() {
            for( auto &p : players )
                p.second->faction.reset();
                
            round.reset();
        }
        
        shared_ptr<Player> getPlayer(size_t id) {
            shared_ptr<Player> p = players[id];
            if( not p ) {
                players[id] = p = make_shared<Player>(id);
                if( round )
                    round->assignFaction(p);
            }
            return p;
        }
        
        void gameLoop() {
            while(true) {
                sleep(5);
                
                mutex.lock();
                createRound();
                thread round_thread(Round::roundLoop, round.get(), &mutex);
                round_thread.join();
                endRound();
                mutex.unlock();
                
                sleep(30);
            }
        }
};

shared_ptr<Game> gGame;

class BajConnection : public Connection {
    private:
        shared_ptr<Player> player;
        
        shared_ptr<Round> requireRound() const {
            if( not gGame->round )
                throw Error::no_round;
            if( not player->faction )
                throw Error::no_round;
            return gGame->round;
        }
        
    protected:
        static CommandsMap command_map;
        CONNECTION_RUNCMD_METHOD;
    public:
        void DESCRIBE_WORLD(istream &in) {
            std::ostringstream sout;
            {
                unique_lock<mutex> _(gGame->mutex);
                auto round = requireRound();
                
                sout << "OK\n";
                sout << round->n << ' ' << round->m << ' ' << player->faction->id << ' ' << round->c << ' ' << round->l << '\n';
            }
            client->write(sout.str());
        }
        
        void DESCRIBE_SQUADS(istream &in) {
            std::ostringstream sout;
            {
                unique_lock<mutex> _(gGame->mutex);
                requireRound();
                 
                sout << "OK\n";
                sout << player->faction->knights.size() << '\n';
                for( auto k : player->faction->knights )
                    sout << k->pos << ' ' << k->amount << '\n';
            }
            client->write(sout.str());    
        }
        
        void DESCRIBE_COTTAGES(istream &in) {
            vector<Coords> cottages;
            {
                unique_lock<mutex> _(gGame->mutex);
                requireRound();
                
                for( auto k : player->faction->knights )
                    if( gGame->round->getTile(k->pos).type == FieldType::Cottage )
                        cottages.push_back(k->pos);
                        
            }   
            std::ostringstream sout;
            sout << "OK\n";
            sout << cottages.size() << '\n';
            for( auto &pos : cottages )
                sout << pos.x << ' ' << pos.y << '\n';
            client->write(sout.str());  
        }
        
        void EXPLORE(istream &in) {
            std::ostringstream sout, sout2;
            {
                unique_lock<mutex> _(gGame->mutex);
                auto round = requireRound();
                Coords coords;
                in >> coords;
                validate_stream(in);
                
                if( not round->is_valid(coords) ) {
                    cerr << "invalid coord" << endl;
                    throw Error::internal;
                }
                    
                Position pos = round->position(coords);
                
                auto has_own = [&round, this](Tile const &t) {
                    return t.knight and t.knight->owner == player->faction->id;
                };            
                auto has_enemy = [&round, this](Tile const &t) {
                    return t.knight and t.knight->owner != player->faction->id;
                };
                    
                if( not has_own(round->getTile(pos)) ) {
                    cerr << "no squad" << endl;
                    throw Error::internal;
                }
                    
                std::ostringstream sout, sout2;
                int r = gGame->round->sight_range, enemies = 0;
                sout << "OK\n";
                sout << r << '\n';
                for( int dy = -r; dy <= r; dy++ ) {
                    for( int dx = -r; dx <= r; dx++ ) {
                        Tile const &tile = round->getTile(pos + Coords{dx, dy});
                        sout << (char)tile.type;
                        if( has_enemy(round->getTile(pos)) ) {
                            enemies++;
                            sout2 << pos << '\n';
                        }
                    }
                    sout << '\n';
                }
                sout << enemies << '\n';
            }
            sout << sout2.rdbuf();
            client->write(sout.str());
        }
        
        void MOVE_SQUAD(istream &in) {
        }
    
        void LIST_MY_BATTLES(istream &in) {
        }
        
        void WAIT(istream &in) {
            {
                unique_lock<mutex> lock(gGame->mutex);
                auto round = requireRound();
                client->write("OK\nWAITING\n");
                round->waiting.wait(lock);
            }
            client->write("OK\n");
        }
    
        BajConnection(shared_ptr<Client> client, size_t id) : Connection(client) {
            unique_lock<mutex> lock(gGame->mutex);
            player = gGame->getPlayer(id);
            if( player->active_connections >= 2 )
                throw Error::too_many_connections;
                
            player->active_connections++;
        }
        
        ~BajConnection() {
            unique_lock<mutex> lock(gGame->mutex);
            player->active_connections--;
        }
        
        static shared_ptr<Connection> create(size_t id, PermissionLevel level, shared_ptr<Client> client) {
            if( level == PermissionLevel::User ) {
                return make_shared<BajConnection>(client, id);
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
    cmd("LIST_MY_BATTLES", &BajConnection::LIST_MY_BATTLES),
    cmd("WAIT", &BajConnection::WAIT)
};

extern "C" void run(ConnectionFactoryList &cfl) {
    cfl << BajConnection::create;
    gGame = make_shared<Game>();
}