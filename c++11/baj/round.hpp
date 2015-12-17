#pragma once
#include "position.hpp"
#include <bits/stdc++.h>

using namespace std;
using namespace Torus;

enum class FieldType : char { 
    Grass = '.',
    LemonTree = 'L',
    Cottage = 'C', 
    Water = '#'
};

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
    size_t player_id;
    size_t active_connections = 0;
    shared_ptr<Faction> faction = nullptr;
    Player(size_t id) : player_id(id) {}
};

class Round {
    public:
        int const n, m, c, l;
        int const sight_range = 2;
        
        int const round_no = 0, turn_count = 10;
        int turn = 1;
    private:
        list<shared_ptr<Faction>> factions;
        list<shared_ptr<Faction>>::iterator unused_faction;
        size_t next_faction_id = 1;
        
        vector<Tile> tiles;
    public:
        std::condition_variable waiting;
        bool is_valid(Coords const &p) {
            return p.x >= 0 and p.y >= 0 and p.x < n and p.y < n;
        }
        Position position(Coords const &p) {
            return Position(p.x, p.y, n, m);
        }
        Tile &getTile(Position const &p) {
            return tiles.at(p.x+p.y*n);
        }
        
        Round(size_t n, size_t m, size_t c, size_t l) : n(n), m(m), c(c), l(l) {}
        
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
        
        static shared_ptr<Round> generateRandomRound();
        
        void roundLoop(mutex *mutex) {
            while( true ) {
                cout << "Turn: " << turn << " / " << turn_count << endl;
                mutex->unlock();
                std::this_thread::sleep_for (std::chrono::seconds(7));
                mutex->lock();
                waiting.notify_all();
                turn++;
                if( turn > turn_count )
                    break;
            }
        }
        
        
};