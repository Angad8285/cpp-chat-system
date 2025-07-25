#include <iostream>
#include <string>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h> // For inet_pton

// Bring the standard namespace into the global scope
using namespace std;

const int PORT = 8080;
const int BUFFER_SIZE = 1024;

int main() {
    int sock = 0;
    struct sockaddr_in serv_addr;
    char buffer[BUFFER_SIZE] = {0};

    // Create a socket
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        cout << "Socket creation error" << endl;
        return -1;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);

    // Convert IPv4 and IPv6 addresses from text to binary form
    // "127.0.0.1" is the loopback address (localhost)
    if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0) {
        cout << "Invalid address/ Address not supported" << endl;
        return -1;
    }

    // Connect to the server
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        cout << "Connection Failed" << endl;
        return -1;
    }

    cout << "Connected to the server. Type 'exit' to quit." << endl;

    string line;
    while (true) {
        cout << "Client: ";
        getline(cin, line);
        
        if (line == "exit") {
            break;
        }

        // Send the message to the server
        send(sock, line.c_str(), line.length(), 0);

        // Receive the echo from the server
        int bytes_received = recv(sock, buffer, BUFFER_SIZE, 0);
        if (bytes_received > 0) {
            cout << "Server echo: " << buffer << endl;
        } else {
            cout << "Server disconnected or error." << endl;
            break;
        }
        
        // Clear the buffer
        memset(buffer, 0, BUFFER_SIZE);
    }

    // Close the socket
    close(sock);
    return 0;
}