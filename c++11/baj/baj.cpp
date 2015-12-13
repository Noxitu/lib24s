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


enum class FieldType { Grass, LemonTree, Cottage, Water };

struct Position {
    short x, y;
    short const n, m;
    Position( short x, short y, short n, short m ) : x(x%n), y(y%m), n(n), m(m) {}
};

struct Knights {
    Position pos;
    int owner;
    int amount;
    Knights( Position const& pos, int owner, int amount ) : pos(pos), owner(owner), amount(amount) {}
};

struct Faction {
    uint32_t id;
    uint32_t points = 0;
    vector<shared_ptr<Knights>> knights;
};

struct Tile {
    FieldType type;
    shared_ptr<Knights> knight = nullptr;
};

struct Player {
    size_t active_connections = 0;
    shared_ptr<Faction> faction = nullptr;
};

class Round {
    private:
        list<shared_ptr<Faction>> factions;
        list<shared_ptr<Faction>>::iterator unused_faction;
        size_t next_faction_id = 1;
        
        vector<Tile> tiles;
        Tile &getTile(Position const &p) {
            return tiles.at(p.x+p.y*n);
        }
    public:
        size_t n, m, c, l;
        
        Round() {
            int bars = 10;
            int bar_height = 9;
            int knights_per_faction = 1; // >1 is bad idea
            int knights_strength = 300;
            int lemons_per_faction = 3;
            int cottages_per_faction = 3;
            
            n = 90;
            m = bar_height * bars;
            c = cottages_per_faction * bars;
            l = lemons_per_faction * bars;
            
            auto seed = std::chrono::system_clock::now().time_since_epoch().count();
            std::default_random_engine rand(seed);
            int normal_tiles = n*bar_height - knights_per_faction - lemons_per_faction - cottages_per_faction;
            int water_count = std::uniform_int_distribution<int>(0, normal_tiles/10)(rand);
            normal_tiles -= water_count;
            
            int bar_step = std::uniform_int_distribution<int>(0,n-1)(rand);
            
            tiles.resize(n*m);
            
            vector<shared_ptr<Faction>> vec_factions;
            for( int i = 0; i < bars; i++ ) {
                vec_factions.push_back( spawnFaction() );
            }
            unused_faction = begin(factions);

            vector<int> chances = {water_count, knights_per_faction, lemons_per_faction, cottages_per_faction, normal_tiles};
            for( int x = 0; x < n; x++ )
                for( int y = 0; y < bar_height; y++ ) {
                    vector<int> distr;
                    {
                        int sum = 0;
                        for( int i = 0; i < chances.size(); i++ ) {
                            sum += chances[i];
                            distr.push_back(sum);
                        }
                    }
                    size_t type = 0;
                    {
                        int random_value = std::uniform_int_distribution<int>(0,distr.back()-1)(rand);
                        while( random_value >= distr[type] ) type++;
                    }
                    chances[type]--;
                    
                    for( int i = 0; i < bars; i++ ) {
                        Position pos(x+i*bar_step, y+i*bar_height, n, m);
                        Tile &tile = getTile(pos);
                        switch(type) {
                            case 0:
                                tile.type = FieldType::Water;
                                break;
                            case 1: {
                                shared_ptr<Knights> k = make_shared<Knights>(pos, i, knights_strength);
                                tile.type = FieldType::Grass;
                                tile.knight = k;
                                vec_factions[i]->knights.push_back(k);
                            }   break;
                            case 2:
                                tile.type = FieldType::LemonTree;
                                break;
                            case 3:
                                tile.type = FieldType::Cottage;
                                break;
                            case 4:
                                tile.type = FieldType::Grass;
                                break;
                        }
                    }
                }
        }
        
        shared_ptr<Faction> spawnFaction() {
            auto f = make_shared<Faction>();
            f->id = next_faction_id++;
            factions.push_back(f);
            return f;
        }
        
        void assignFaction(shared_ptr<Player> p) {
            if( p->faction )
                throw std::runtime_error("Player already controls faction.");
                
            if( unused_faction == end(factions) )
                spawnFaction();
                
            p->faction = *(unused_faction++);
        }
};



class Game {
    private:
        map<size_t, shared_ptr<Player>> players;
    public:
        shared_ptr<Round> round = nullptr;
        
        void createRound() {
            for( auto &p : players )
                p.second->faction.reset();

            round = make_shared<Round>();
            
            for( auto &p : players )
                if( p.second->active_connections ) 
                    round->assignFaction(p.second);
        }
        
        shared_ptr<Player> getPlayer(size_t id) {
            shared_ptr<Player> p = players[id];
            if( not p ) {
                players[id] = p = make_shared<Player>();
                if( round )
                    round->assignFaction(p);
            }
            return p;
        }
};

shared_ptr<Game> gGame = make_shared<Game>();

class BajConnection : public Connection {
    private:
        shared_ptr<Player> player;
        
        void requireRound() const {
            if( not gGame->round )
                throw Error::no_round;
            if( not player->faction )
                throw Error::no_round;
        }
        
    protected:
        static CommandsMap command_map;
        CONNECTION_RUNCMD_METHOD;
    public:
        void DESCRIBE_WORLD(istream &in) {
            requireRound();
            
            std::ostringstream sout;
            sout << gGame->round->n << ' ' << gGame->round->m << ' ' << player->faction->id << ' ' << gGame->round->c << ' ' << gGame->round->l << '\n';
            
            client->write(sout.str());
        }
        
        void DESCRIBE_SQUADS(istream &in) {
            requireRound();
                
            std::ostringstream sout;
            sout << player->faction->knights.size() << '\n';
            for( auto k : player->faction->knights )
                sout << k->pos.x << ' ' << k->pos.y << ' ' << k->amount << '\n';
                
            client->write(sout.str());    
        }
        
        void DESCRIBE_COTTAGES(istream &in) {
        }
        
        void EXPLORE(istream &in) {
        }
        
        void MOVE_SQUAD(istream &in) {
        }
    
        void LIST_MY_BATTLES(istream &in) {
        }
    
        BajConnection(shared_ptr<Client> client, size_t id) : Connection(client) {
            player = gGame->getPlayer(id);
            if( player->active_connections >= 2 )
                throw Error::too_many_connections;
                
            player->active_connections++;
        }
        
        ~BajConnection() {
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
    cmd("LIST_MY_BATTLES", &BajConnection::LIST_MY_BATTLES)
};

extern "C" void run(ConnectionFactoryList &cfl) {
    cfl << BajConnection::create;
    gGame->createRound();
}