// Bring the standard namespace into the global scope
using namespace std;

#include <iostream>
#include <string>
#include <cstring>      // For memset
#include <unistd.h>     // For close()
#include <sys/socket.h> // For socket APIs
#include <netinet/in.h> // For sockaddr_in


const int PORT = 8080;
const int BUFFER_SIZE = 1024;

int main() {
    // Step 1.1: Server Setup
    // -----------------------
    int server_fd, client_socket;
    struct sockaddr_in address;
    int opt = 1;

    // Create a socket file descriptor
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    // Set socket options to allow reuse of the address and port
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);


    // Bind the socket to our specified IP and port
    if (::bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    // Put the server socket in a passive mode, waiting for clients to connect
    if (listen(server_fd, 3) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    cout << "Server listening on port " << PORT << "..." << endl;

    // Step 1.2: Accepting a Connection
    // ---------------------------------
    int addrlen = sizeof(address);
    if ((client_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen)) < 0) {
        perror("accept");
        exit(EXIT_FAILURE);
    }

    cout << "Connection accepted!" << endl;

    // Step 1.3: The "Echo" Logic
    // ----------------------------
    char buffer[BUFFER_SIZE] = {0};
    int bytes_received;

    // Keep receiving data until the client disconnects
    while ((bytes_received = recv(client_socket, buffer, BUFFER_SIZE, 0)) > 0) {
        cout << "Received from client: " << buffer << endl;

        // Echo the data back to the client
        send(client_socket, buffer, bytes_received, 0);

        // Clear the buffer for the next message
        memset(buffer, 0, BUFFER_SIZE);
    }

    if (bytes_received == 0) {
        cout << "Client disconnected." << endl;
    } else {
        perror("recv failed");
    }
    
    // Close the sockets
    close(client_socket);
    close(server_fd);

    return 0;
}

