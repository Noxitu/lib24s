#include "connection.hpp"
#include "position.hpp"

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


enum class FieldType : char { 
    Grass = '.',
    LemonTree = 'L',
    Cottage = 'C', 
    Water = '#'
};

using namespace Torus;

struct Knights {
    Position pos;
    int owner;
    int amount;
    Knights( Position const& pos, int owner, int amount ) : pos(pos), owner(owner), amount(amount) {}
};

struct Faction {
    int id;
    int points = 0;
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
    public:
        bool is_valid(Coords const &p) {
            return p.x >= 0 and p.y >= 0 and (unsigned int)p.x < n and (unsigned int)p.y < n;
        }
        Position position(Coords const &p) {
            return Position(p.x, p.y, n, m);
        }
        Tile &getTile(Position const &p) {
            return tiles.at(p.x+p.y*n);
        }
        
        size_t n, m, c, l;
        unsigned int const sight_range = 2;
        
        Round() {
            unsigned int bars = 10;
            unsigned int bar_height = 9;
            unsigned int knights_per_faction = 1; // >1 is bad idea
            unsigned int knights_strength = 300;
            unsigned int lemons_per_faction = 3;
            unsigned int cottages_per_faction = 3;
            
            n = 90;
            m = bar_height * bars;
            c = cottages_per_faction * bars;
            l = lemons_per_faction * bars;
            
            auto seed = std::chrono::system_clock::now().time_since_epoch().count();
            std::default_random_engine rand(seed);
            unsigned int normal_tiles = n*bar_height - knights_per_faction - lemons_per_faction - cottages_per_faction;
            unsigned int water_count = std::uniform_int_distribution<unsigned int>(0, normal_tiles/10)(rand);
            normal_tiles -= water_count;
            
            unsigned int bar_step = std::uniform_int_distribution<unsigned int>(0,n-1)(rand);
            
            tiles.resize(n*m);
            
            vector<shared_ptr<Faction>> vec_factions;
            for( unsigned int i = 0; i < bars; i++ ) {
                vec_factions.push_back( spawnFaction() );
            }
            unused_faction = begin(factions);

            vector<unsigned int> chances = {water_count, knights_per_faction, lemons_per_faction, cottages_per_faction, normal_tiles};
            for( unsigned int x = 0; x < n; x++ )
                for( unsigned int y = 0; y < bar_height; y++ ) {
                    vector<unsigned int> distr;
                    {
                        int sum = 0;
                        for( unsigned int i = 0; i < chances.size(); i++ ) {
                            sum += chances[i];
                            distr.push_back(sum);
                        }
                    }
                    size_t type = 0;
                    {
                        unsigned int random_value = std::uniform_int_distribution<unsigned int>(0,distr.back()-1)(rand);
                        while( random_value >= distr[type] ) type++;
                    }
                    chances[type]--;
                    
                    for( unsigned int i = 0; i < bars; i++ ) {
                        Position pos(x+i*bar_step, y+i*bar_height, n, m);
                        Tile &tile = getTile(pos);
                        switch(type) {
                            case 0:
                                tile.type = FieldType::Water;
                                break;
                            case 1: {
                                shared_ptr<Knights> k = make_shared<Knights>(pos, vec_factions.at(i)->id, knights_strength);
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
                sout << k->pos << ' ' << k->amount << '\n';
                
            client->write(sout.str());    
        }
        
        void DESCRIBE_COTTAGES(istream &in) {
            requireRound();
                
            std::ostringstream sout;
            vector<Coords> cottages;
            for( auto k : player->faction->knights )
                if( gGame->round->getTile(k->pos).type == FieldType::Cottage )
                    cottages.push_back(k->pos);
                    
            sout << cottages.size() << '\n';
            for( auto &pos : cottages )
                sout << pos.x << ' ' << pos.y << '\n';
                
            client->write(sout.str());  
        }
        
        void EXPLORE(istream &in) {
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
            sout << sout2.rdbuf();
            
            client->write(sout.str());
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