#include "round.hpp"
#include <bits/stdc++.h>


shared_ptr<Round> Round::generateRandomRound() {
    int bars = 10;
    int bar_height = 9;
    int knights_per_faction = 1; // >1 is bad idea
    int knights_strength = 300;
    int lemons_per_faction = 6;
    int cottages_per_faction = 6;
    
    int n = 90;
    int m = bar_height * bars;
    int c = cottages_per_faction * bars;
    int l = lemons_per_faction * bars;
    
    shared_ptr<Round> round = make_shared<Round>(n,m,c,l);
    
    auto seed = std::chrono::system_clock::now().time_since_epoch().count();
    std::default_random_engine rand(seed);
    int normal_tiles = n*bar_height - knights_per_faction - lemons_per_faction - cottages_per_faction;
    int water_count = std::uniform_int_distribution<int>(0, normal_tiles/5)(rand);
    normal_tiles -= water_count;
    
    int bar_step = std::uniform_int_distribution<int>(0,n-1)(rand);
    
    round->tiles.resize(n*m);
    
    vector<shared_ptr<Faction>> vec_factions;
    for( int i = 0; i < bars; i++ ) {
        vec_factions.push_back( round->spawnFaction() );
    }
    round->unused_faction = begin(round->factions);

    vector<int> chances = {water_count, knights_per_faction, lemons_per_faction, cottages_per_faction, normal_tiles};
    for( int y = 0; y < bar_height; y++ ) {
        for( int x = 0; x < n; x++ ) {
            vector<int> distr;
            {
                int sum = 0;
                for( size_t i = 0; i < chances.size(); i++ ) {
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
                Tile &tile = round->getTile(pos);
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
    
    return round;
}