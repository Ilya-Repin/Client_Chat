#include <iostream>
#include "app/app.h"
#include "network/register.h"

int main(int argc, char* argv[]) {
    if (argc != 6) {
        return EXIT_FAILURE;
    }

    std::string name = argv[1];
    std::string password = argv[2];
    std::string file = argv[3];;
    std::string ip = argv[4];
    int port = std::atoi(argv[5]);
    std::string path = argv[6];

    network::HttpPostClient client(ip, path);
    std::string token = client.send_request(name, password);

    std::cout << "Token: " << token << std::endl;

    app::App app(ip, port, "test.private.key", name, token);
    app.Run();

    return 0;
}
