#pragma once
#include "position.hpp"
#include <bits/stdc++.h>

using namespace std;

enum class FieldContent : char { 
    None = '.',
    X = 'X',
    O = 'O', 
};

struct Faction {
    FieldContent type;
    shared_ptr<Coords> move;
    std::condition_variable waiting;
    
    Faction(FieldContent type) : type(type) {}
};

class Round {
    private:
        
        vector<shared_ptr<Faction>>::iterator current_move;
    public:
        FieldContent board[3][3];
        vector<shared_ptr<Faction>> factions;
        shared_ptr<Faction> winner;
        mutex round_mutex;
        bool ended = false;
        
        Round() {
            cout << "Round()" << endl;
            factions.push_back(make_shared<Faction>(FieldContent::X));
            factions.push_back(make_shared<Faction>(FieldContent::O));
            current_move = begin(factions);
            
            for( int y = 0; y < 3; y++ )
                for(int x = 0; x < 3; x++ )
                    board[y][x] = FieldContent::None;
        }
        
        ~Round() {
            cout << "~Round()" << endl;
        }
        
        FieldContent& get(Coords const &c) {
            return board[c.y][c.x];
        }
        
        bool canMove(shared_ptr<Faction> faction) {
            return faction == *current_move;
        }
        
        bool hasEnded() {
            static Coords lines[][3] = { 
                {Coords(0,0), Coords(0,1), Coords(0,2)},
                {Coords(1,0), Coords(1,1), Coords(1,2)},
                {Coords(2,0), Coords(2,1), Coords(2,2)},
                {Coords(0,0), Coords(1,0), Coords(2,0)},
                {Coords(0,1), Coords(1,1), Coords(2,1)},
                {Coords(0,2), Coords(1,2), Coords(2,2)},
                {Coords(0,0), Coords(1,1), Coords(2,2)},
                {Coords(2,0), Coords(1,1), Coords(0,2)},
            };
            
            for( auto l : lines )
                if( get(l[0]) == get(l[1]) and get(l[0]) == get(l[2]) )
                    for( auto f : factions )
                        if( f->type == get(l[0]) ) {
                            winner = f;
                            return true;
                        }
            
            for( int y = 0; y < 3; y++ )
                for(int x = 0; x < 3; x++ )
                    if( board[y][x] == FieldContent::None )
                        return false;
            return true;
        }
        
        void roundLoop() {
            while(true) {
                (*current_move)->waiting.notify_all();
                sleep(7);
                unique_lock<mutex> _(round_mutex);
                cout << "Round::Turn End" << endl;
                
                if( (*current_move)->move ) {
                    get(*(*current_move)->move) = (*current_move)->type;
                    (*current_move)->move.reset();
                }
                
                if( hasEnded() )
                    break;
                
                current_move++;
                if( current_move == end(factions) )
                    current_move = begin(factions);
                    
            }
            ended = true;
            for( auto f : factions )
                f->waiting.notify_all();
        }
        
        
};