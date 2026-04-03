#include <iostream>
#include "Server/server.h"
#include "Server/session.h"
#include "Server/kv_api.h"

int main()
{
    if (!storage::init)

    if (!server::init_engine()) {
        std::cerr << "engine init failed\n";
        return 1;
    }



    int listen_fd = server::create_listen_socket("0.0.0.0", 19090, 128);
    std::cout << "DataAgentDB listening on 0.0.0.0:19090\n";

    ///todo: thread pool
    server::run_accept_loop(listen_fd, [](int client_fd) {
        server::handle_client(client_fd);
    });

    return 0;
}

/*
 connect: nc 127.0.0.1 19090

*/