#include <iostream>
#include <string>
#include <vector>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/select.h>

using namespace std;

const int PORT = 8080;
const int BUFFER_SIZE = 1024;

int main() {
    int server_fd, new_socket;
    struct sockaddr_in address;
    int opt = 1;
    char buffer[BUFFER_SIZE] = {0};

    fd_set master_set, read_fds;
    int fdmax;

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

    if (listen(server_fd, 10) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    FD_ZERO(&master_set);
    FD_SET(server_fd, &master_set);
    fdmax = server_fd;

    cout << "Server listening on port " << PORT << "..." << endl;

    while (true) {
        read_fds = master_set;
        if (select(fdmax + 1, &read_fds, NULL, NULL, NULL) == -1) {
            perror("select");
            exit(EXIT_FAILURE);
        }

        for (int i = 0; i <= fdmax; i++) {
            if (FD_ISSET(i, &read_fds)) {
                if (i == server_fd) {
                    // New connection
                    int addrlen = sizeof(address);
                    if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen)) < 0) {
                        perror("accept");
                    } else {
                        FD_SET(new_socket, &master_set);
                        if (new_socket > fdmax) fdmax = new_socket;
                        
                        cout << "New connection on socket " << new_socket << endl;

                        // **FIX**: Announce the new user to everyone else
                        string welcome_msg = "Server: User " + to_string(new_socket) + " has joined.";
                        for (int j = 0; j <= fdmax; j++) {
                            if (FD_ISSET(j, &master_set) && j != server_fd && j != new_socket) {
                                send(j, welcome_msg.c_str(), welcome_msg.length(), 0);
                            }
                        }
                    }
                } else {
                    // Data from an existing client
                    int bytes_received;
                    if ((bytes_received = recv(i, buffer, BUFFER_SIZE, 0)) <= 0) {
                        if (bytes_received == 0) {
                            cout << "Socket " << i << " disconnected." << endl;
                            
                            // **FIX**: Announce the user's departure
                            string goodbye_msg = "Server: User " + to_string(i) + " has left.";
                            for (int j = 0; j <= fdmax; j++) {
                                if (FD_ISSET(j, &master_set) && j != server_fd && j != i) {
                                    send(j, goodbye_msg.c_str(), goodbye_msg.length(), 0);
                                }
                            }
                        } else {
                            perror("recv");
                        }
                        close(i);
                        FD_CLR(i, &master_set);
                    } else {
                        // Prepend sender info to the message
                        string message = "User " + to_string(i) + ": " + string(buffer, bytes_received);

                        // Broadcast the message to everyone else
                        for (int j = 0; j <= fdmax; j++) {
                            if (FD_ISSET(j, &master_set) && j != server_fd && j != i) {
                                send(j, message.c_str(), message.length(), 0);
                            }
                        }
                        memset(buffer, 0, BUFFER_SIZE);
                    }
                }
            }
        }
    }

    return 0;
}