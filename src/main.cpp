#include "Server.h"
#include <iostream>

const int PORT = 8080;

int main() {
    try {
        // Create an instance of our Server class on port 8080
        Server chat_server(PORT);

        // Run the server
        chat_server.run();

    } catch (const std::exception& e) {
        std::cerr << "An unexpected error occurred: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "An unknown error occurred." << std::endl;
        return 1;
    }

    return 0;
}