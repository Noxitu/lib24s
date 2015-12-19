#pragma once
#include <iostream>

struct Coords {
    int x, y;
    Coords() = default;
    constexpr Coords(int x, int y) : x(x), y(y) {}
    friend std::istream& operator>>(std::istream &in, Coords &pos) {
        return in >> pos.x >> pos.y;
    }
    friend std::ostream& operator<<(std::ostream &out, Coords const &pos) {
        return out << pos.x << ' ' << pos.y;
    }
    friend Coords operator+(Coords const &a, Coords const &b) {
        return Coords{a.x+b.x, a.y+b.y};
    }
    constexpr Coords operator- () const {
        return Coords{-x, -y};
    }
};
constexpr Coords operator "" _x (long long unsigned x) {
    return Coords{(int)x, 0};
}
constexpr Coords operator "" _y (long long unsigned y) {
    return Coords{0, (int)y};
}

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

namespace HexagonalRectangle {
    enum class Direction { W, NW, NE, E, SE, SW };
    inline Coords operator+(Coords const &p, Direction dir) {
        switch(dir) {
            case Direction::W: return p + -1_x;
            case Direction::E: return p +  1_x;
            case Direction::NW: return p + -1_y + (p.y%2 == 1 ? -1_x : 0_x);
            case Direction::NE: return p + -1_y + (p.y%2 == 0 ?  1_x : 0_x);
            case Direction::SW: return p +  1_y + (p.y%2 == 1 ? -1_x : 0_x);
            case Direction::SE: return p +  1_y + (p.y%2 == 0 ?  1_x : 0_x);
            default: return p;
        }
    }
}