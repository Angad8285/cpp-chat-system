#include <iostream>
#include <string>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/select.h>

using namespace std;

const int PORT = 8080;
const int BUFFER_SIZE = 1024;

int main() {
    int sock = 0;
    struct sockaddr_in serv_addr;
    char buffer[BUFFER_SIZE] = {0};

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        cout << "Socket creation error" << endl;
        return -1;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);

    if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0) {
        cout << "Invalid address/ Address not supported" << endl;
        return -1;
    }

    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        cout << "Connection Failed" << endl;
        return -1;
    }

    cout << "Connected to the server. You can start typing." << endl;

    while (true) {
        fd_set read_fds;
        FD_ZERO(&read_fds);
        
        // Add standard input (keyboard) to the set
        FD_SET(STDIN_FILENO, &read_fds); // STDIN_FILENO is usually 0
        
        // Add the server socket to the set
        FD_SET(sock, &read_fds);

        // We need to find the highest file descriptor for select()
        int fdmax = (STDIN_FILENO > sock) ? STDIN_FILENO : sock;
        
        // Wait for activity on either stdin or the socket
        if (select(fdmax + 1, &read_fds, NULL, NULL, NULL) == -1) {
            perror("select");
            exit(EXIT_FAILURE);
        }

        // Check for activity on the socket (message from server)
        if (FD_ISSET(sock, &read_fds)) {
            int bytes_received = recv(sock, buffer, BUFFER_SIZE, 0);
            if (bytes_received <= 0) {
                cout << "\nServer disconnected." << endl;
                break;
            }
            cout << "\n" << string(buffer, bytes_received) << endl;
            memset(buffer, 0, BUFFER_SIZE);
            cout << "Client: ";
            fflush(stdout); // Ensure "Client: " prompt is displayed
        }

        // Check for activity on stdin (user typed something)
        if (FD_ISSET(STDIN_FILENO, &read_fds)) {
            string line;
            getline(cin, line);
            if (line == "exit") {
                break;
            }
            send(sock, line.c_str(), line.length(), 0);
            cout << "Client: ";
            fflush(stdout); // Ensure "Client: " prompt is displayed
        }
    }

    close(sock);
    return 0;
}