#ifndef SERVER_H
#define SERVER_H

#include <string>
#include <map>
#include <thread>
#include <mutex>
#include <atomic>

class Server {
public:
    // The constructor will initialize the server with a port
    Server(int port);

    // The main public function to start and run the server
    void run();

private:
    // --- Member Variables ---
    int port;
    int server_fd; // The listening socket descriptor for the server
    std::map<int, std::string> clients; // Map to store client socket and nickname
    std::mutex mtx; // Mutex to protect the shared 'clients' map
    std::atomic<bool> is_running; // Flag to control the server's main loop

    // --- Private Helper Methods ---

    // Sets up the server socket (socket, bind, listen)
    void setup_server();

    // The main loop to accept new connections and handle stdin
    void accept_connections();

    // The function that each client thread will run
    void handle_client(int client_socket);
    
    // Broadcasts a message to all clients, with an option to exclude the sender
    void broadcast_message(const std::string& message, int sender_socket = -1);
};

#endif // SERVER_H