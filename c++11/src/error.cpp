#include "error.hpp"

Error const Error::invalid_auth(1, "Incorrect login or password.");
Error const Error::unknown_command(3, "Unknown command name.");
Error const Error::too_many_connections(6, "Too many connections.");
Error const Error::internal(8, "Internal server error.");
Error const Error::no_round(9, "No current round.");