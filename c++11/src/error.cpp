#include "error.hpp"

Error const Error::invalid_auth(1, "Incorrect login or password.");
Error const Error::unknown_command(3, "Unknown command name.");
Error const Error::wrong_arg_num(4, "Wrong number of arguments");
Error const Error::invalid_arg_syntax(5, "Invalid syntax of arguments");
Error const Error::too_many_connections(6, "Too many connections.");
Error const Error::internal(8, "Internal server error.");
Error const Error::no_round(9, "No current round.");