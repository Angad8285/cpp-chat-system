#include <iostream>
#include <string>
#include <vector>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/select.h> // The main header for select()

using namespace std;

const int PORT = 8080;
const int BUFFER_SIZE = 1024;

int main() {
    int server_fd, new_socket;
    struct sockaddr_in address;
    int opt = 1;
    char buffer[BUFFER_SIZE] = {0};

    // Data structures for select()
    fd_set master_set, read_fds;
    int fdmax;

    // Initial server socket setup
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if (::bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, 10) < 0) { // Increased backlog for more clients
        perror("listen");
        exit(EXIT_FAILURE);
    }

    // Initialize the master fd_set
    FD_ZERO(&master_set);
    FD_SET(server_fd, &master_set);
    fdmax = server_fd; // So far, it's this one

    cout << "Server listening on port " << PORT << "..." << endl;

    // Main server loop
    while (true) {
        read_fds = master_set; // Copy the master set

        // select() is a blocking call, waiting for activity on any of the sockets
        if (select(fdmax + 1, &read_fds, NULL, NULL, NULL) == -1) {
            perror("select");
            exit(EXIT_FAILURE);
        }

        // Loop through existing connections looking for data to read
        for (int i = 0; i <= fdmax; i++) {
            if (FD_ISSET(i, &read_fds)) { // We found one with activity
                
                // Case 1: Activity on the listening socket -> New connection
                if (i == server_fd) {
                    int addrlen = sizeof(address);
                    if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen)) < 0) {
                        perror("accept");
                    } else {
                        FD_SET(new_socket, &master_set); // Add to master set
                        if (new_socket > fdmax) {
                            fdmax = new_socket; // Keep track of the max
                        }
                        cout << "New connection on socket " << new_socket << endl;
                    }
                } 
                // Case 2: Activity on an existing client socket -> Incoming data
                else {
                    int bytes_received;
                    if ((bytes_received = recv(i, buffer, BUFFER_SIZE, 0)) <= 0) {
                        // Error or connection closed by client
                        if (bytes_received == 0) {
                            cout << "Socket " << i << " disconnected." << endl;
                        } else {
                            perror("recv");
                        }
                        close(i);
                        FD_CLR(i, &master_set); // Remove from master set
                    } else {
                        // We got some data from a client
                        // Broadcast it to everyone else
                        for (int j = 0; j <= fdmax; j++) {
                            if (FD_ISSET(j, &master_set)) {
                                // Except the listener and ourselves
                                if (j != server_fd && j != i) {
                                    if (send(j, buffer, bytes_received, 0) == -1) {
                                        perror("send");
                                    }
                                }
                            }
                        }
                        // Clear the buffer after sending
                        memset(buffer, 0, BUFFER_SIZE);
                    }
                }
            }
        }
    }

    return 0;
}