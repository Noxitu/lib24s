#pragma once
#include <bits/stdc++.h>

class Config {
    private:
        std::unordered_map<std::string, std::vector<std::string>> data;
    public:
        Config(std::string filename) {
            std::ifstream in(filename);
            in >> *this;
        }
        
        friend std::istream& operator>> (std::istream &in, Config &config) {
            std::string line;
            while( std::getline(in, line) ) {
                // rstrip whitespaces
                while( not line.empty() and isspace(line.back()) )
                    line.pop_back();
                    
                auto it1 = std::find_if_not( std::begin(line), std::end(line), isspace );
                if( it1 == std::end(line) or *it1 == '#' )
                    continue;
                
                auto it2 = std::find_if( it1, std::end(line), isspace );
                auto it3 = std::find_if_not( it2, std::end(line), isspace );
                
                std::string key(it1, it2);
                std::string value(it3, end(line));
                config.data[key].push_back(value);
            }
            return in;
        }
        
        friend std::ostream& operator<< (std::ostream &out, Config const &config) {
            for( auto const &p : config.data ) {
                for( auto const &value : p.second )
                    out << p.first << '\t' << value << '\n';
            }
            return out;
        }
};