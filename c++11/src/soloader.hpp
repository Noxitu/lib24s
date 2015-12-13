#pragma once

#include <string>
#include <iostream>
#include <cstdlib>
#include <dlfcn.h>

typedef void(*main_so)(ConnectionFactoryList&);
main_so load_so(std::string const &filename) {
    void *handle = dlopen(filename.c_str(), RTLD_NOW);
    if (!handle) {
        std::clog << dlerror() << std::endl;
        exit(1);
    }

    main_so f = (main_so) dlsym(handle, "run");
    char *error = dlerror();
    if (error != NULL)  {
        std::clog << error << std::endl;
        exit(1);
    }

    return f;
}