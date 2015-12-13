#pragma once
#include "utils.hpp"
#include <bits/stdc++.h>

enum class PermissionLevel { None, User, Voice, Op };

class Auth {
    private:
    public:
        struct UserData {
            size_t id;
            PermissionLevel level;
        };
        virtual UserData find_user(std::string username, std::string password) = 0;
};

class FileAuth : public Auth {
    private:
        std::unordered_map<std::pair<std::string, std::string>, UserData> storage;
    public:
        FileAuth(std::string filename) {
            std::ifstream in(filename);
            std::string line;
            while( std::getline(in, line) ) {
                while( not line.empty() and isspace(line.back()) )
                    line.pop_back();
                    
                std::stringstream sin(line);
                size_t user_id;
                std::string username, password;
                PermissionLevel level = PermissionLevel::User;
                
                if( not isdigit(sin.peek()) )
                    switch( sin.get() ) {
                        case '@':
                            level = PermissionLevel::Op;
                            break;
                        case '+':
                            level = PermissionLevel::Voice;
                            break;
                        default:
                            throw std::runtime_error("Unknown permission level.");
                    }
                sin >> user_id >> username >> password;
                storage[std::make_pair(username, password)] = {user_id, level};
            }
        }
    
        UserData find_user(std::string username, std::string password) override {
            auto ret = storage.find(std::make_pair(username, password));
            if( ret == end(storage) )
                return {0, PermissionLevel::None};
            return ret->second;
        }
};
