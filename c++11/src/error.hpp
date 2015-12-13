#pragma once
#include "listener.hpp"
#include <bits/stdc++.h>

struct Error {
    private:
        int const code;
        char const *message;
    protected:
        Error(int code, char const *message) : code(code), message(message) {}
    public:
        friend void operator<<(std::shared_ptr<Client> const client, Error const &error) {
            std::ostringstream out;
            out << "ERROR " << error.code << ' ' << error.message << '\n';
            client->write(out.str());
        }
        static const Error invalid_auth;
        static const Error unknown_command;
        static const Error too_many_connections;
        static const Error internal;
        static const Error no_round;
};