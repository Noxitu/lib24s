#pragma once
#include <iostream>

struct Coords {
    int x, y;
    friend std::istream& operator>>(std::istream &in, Coords &pos) {
        return in >> pos.x >> pos.y;
    }
    friend std::ostream& operator<<(std::ostream &out, Coords const &pos) {
        return out << pos.x << ' ' << pos.y;
    }
    friend Coords operator "" _x (char const *x, size_t) {
        return Coords{atoi(x),0};
    }
    friend Coords operator "" _y (char const *y, size_t) {
        return Coords{0, atoi(y)};
    }
    friend Coords operator+(Coords const &a, Coords const &b) {
        return Coords{a.x+b.x, a.y+b.y};
    }
};

namespace Torus {
    constexpr int nn_mod(int a, int b) { return (a%b+b)%b; }

    struct Position : Coords {
        int const n, m;
        Position( int x, int y, int n, int m ) : Coords{nn_mod(x,n), nn_mod(y,m)}, n(n), m(m) {}
        
        friend Position operator+(Position const &a, Coords const &b) {
            return Position(a.x+a.y, a.y+b.y, a.n, a.m);
        }
    };
}
