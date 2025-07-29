#include <iostream>
#include <string>
#include <vector>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <thread>
#include <mutex>
#include <algorithm> // For std::remove

using namespace std;

const int PORT = 8080;
const int BUFFER_SIZE = 1024;

// Shared resources for all threads
vector<int> client_sockets;
mutex mtx;

void broadcast_message(const string& message, int sender_socket) {
    lock_guard<mutex> guard(mtx);
    for (int client_socket : client_sockets) {
        if (client_socket != sender_socket) {
            send(client_socket, message.c_str(), message.length(), 0);
        }
    }
}

void handle_client(int client_socket) {
    char buffer[BUFFER_SIZE] = {0};
    
    // Announce the new user
    string welcome_msg = "Server: User " + to_string(client_socket) + " has joined.";
    cout << welcome_msg << endl;
    broadcast_message(welcome_msg, client_socket);

    int bytes_received;
    while ((bytes_received = recv(client_socket, buffer, BUFFER_SIZE, 0)) > 0) {
        string message = "User " + to_string(client_socket) + ": " + string(buffer, bytes_received);
        cout << message << endl;
        broadcast_message(message, client_socket);
        memset(buffer, 0, BUFFER_SIZE);
    }

    // Handle disconnection
    string goodbye_msg = "Server: User " + to_string(client_socket) + " has left.";
    cout << goodbye_msg << endl;
    broadcast_message(goodbye_msg, client_socket);

    // Clean up: remove socket from the list and close it
    {
        lock_guard<mutex> guard(mtx);
        client_sockets.erase(remove(client_sockets.begin(), client_sockets.end(), client_socket), client_sockets.end());
    }
    close(client_socket);
}

int main() {
    int server_fd;
    struct sockaddr_in address;
    int opt = 1;

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

    cout << "Server listening on port " << PORT << "..." << endl;

    while (true) {
        int addrlen = sizeof(address);
        int new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen);
        if (new_socket < 0) {
            perror("accept");
            continue; // Continue to the next iteration
        }

        // Add the new client to our shared list
        {
            lock_guard<mutex> guard(mtx);
            client_sockets.push_back(new_socket);
        }

        // Create a new thread to handle this client
        thread t(handle_client, new_socket);
        t.detach(); // The main thread doesn't need to wait for this client thread to finish
    }

    close(server_fd);
    return 0;
}