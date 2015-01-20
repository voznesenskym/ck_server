#include <iostream>
#include "vz_server.h"
#include <curve.h>
#include <czmq.h>

int main(int argc, const char * argv[])
{
    std::cout << "Start.\n";
    vz_server *server = vz_server::instance();
    server->run();

    delete server;
    
    return 0;
}

