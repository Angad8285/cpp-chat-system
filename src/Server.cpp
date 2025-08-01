#include "Server.h" // The header file for our class
#include <iostream>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/select.h>
#include <sstream>

// --- Constructor ---
Server::Server(int port) : port(port), is_running(true) {
    // Constructor initializes the port and running state
}

// --- Public Methods ---
void Server::run() {
    setup_server();
    accept_connections();
}

// --- Private Methods ---
void Server::setup_server() {
    struct sockaddr_in address;

    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);

    if (::bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, 10) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    std::cout << "Server listening on port " << port << "..." << std::endl;
    std::cout << "Type 'SHUTDOWN' to close the server." << std::endl;
}

void Server::accept_connections() {
    while (is_running) {
        fd_set read_fds;
        FD_ZERO(&read_fds);
        FD_SET(server_fd, &read_fds);
        FD_SET(STDIN_FILENO, &read_fds);

        int fdmax = (server_fd > STDIN_FILENO) ? server_fd : STDIN_FILENO;

        if (select(fdmax + 1, &read_fds, NULL, NULL, NULL) == -1) {
            if (!is_running) break;
            perror("select");
            continue;
        }

        if (FD_ISSET(STDIN_FILENO, &read_fds)) {
            std::string command;
            std::cin >> command;
            if (command == "SHUTDOWN") {
                std::cout << "Shutdown command received. Closing server." << std::endl;
                is_running = false;
            }
        }
        
        if (FD_ISSET(server_fd, &read_fds)) {
            int new_socket = accept(server_fd, nullptr, nullptr);
            if (new_socket < 0) {
                perror("accept");
                continue;
            }

            std::string default_name = "User" + std::to_string(new_socket);
            {
                std::lock_guard<std::mutex> guard(mtx);
                clients[new_socket] = default_name;
                std::cout << "New connection on socket " << new_socket << " as " << default_name << std::endl;

                std::string welcome_msg = "Server: " + default_name + " has joined.";
                broadcast_message(welcome_msg, new_socket);
            }
            
            // Spawn a new thread using a lambda to call the member function
            std::thread t([this, new_socket] { this->handle_client(new_socket); });
            t.detach();
        }
    }

    // Graceful shutdown sequence
    std::cout << "Notifying clients of shutdown..." << std::endl;
    broadcast_message("Server: Shutting down now. Goodbye!", -1);

    std::cout << "Closing all sockets..." << std::endl;
    {
        std::lock_guard<std::mutex> guard(mtx);
        for(auto const& [socket, name] : clients) {
            close(socket);
        }
        clients.clear();
    }
    
    close(server_fd);
    std::cout << "Server closed." << std::endl;
}

void Server::handle_client(int client_socket) {
    char buffer[1024];
    
    int bytes_received;
    while (is_running && (bytes_received = recv(client_socket, buffer, 1024, 0)) > 0) {
        std::string received_msg(buffer, bytes_received);
        std::stringstream ss(received_msg);
        std::string command;
        ss >> command;

        if (command == "NICK") {
            std::string new_nickname;
            if (ss >> new_nickname) {
                std::string notification;
                {
                    std::lock_guard<std::mutex> guard(mtx);
                    std::string old_nickname = clients[client_socket];
                    clients[client_socket] = new_nickname;
                    notification = "Server: " + old_nickname + " is now known as " + new_nickname + ".";
                }
                std::cout << notification << std::endl;
                broadcast_message(notification, -1);
            }
        } else if (command == "MSG") {
             std::string recipient_nick, private_msg;
            ss >> recipient_nick;
            getline(ss, private_msg);

            if (!private_msg.empty()) {
                private_msg = private_msg.substr(1);
                std::lock_guard<std::mutex> guard(mtx);
                int recipient_socket = -1;
                std::string sender_nick = clients[client_socket];

                for (auto const& [socket, name] : clients) {
                    if (name == recipient_nick) {
                        recipient_socket = socket;
                        break;
                    }
                }

                if (recipient_socket != -1) {
                    std::string formatted_msg = "(private) " + sender_nick + ": " + private_msg;
                    send(recipient_socket, formatted_msg.c_str(), formatted_msg.length(), 0);
                } else {
                    std::string error_msg = "Server: Error - User '" + recipient_nick + "' not found.";
                    send(client_socket, error_msg.c_str(), error_msg.length(), 0);
                }
            }
        } else if (command == "LIST") {
            std::string user_list = "Server: Users online - ";
             {
                std::lock_guard<std::mutex> guard(mtx);
                for (auto const& [socket, name] : clients) {
                    user_list += name + ", ";
                }
            }
             if (user_list.length() > 22) {
                user_list = user_list.substr(0, user_list.length() - 2);
            }
            send(client_socket, user_list.c_str(), user_list.length(), 0);
        } else {
            std::string public_msg;
             {
                std::lock_guard<std::mutex> guard(mtx);
                std::string sender_nick = clients[client_socket];
                public_msg = sender_nick + ": " + received_msg;
            }
            std::cout << public_msg << std::endl;
            broadcast_message(public_msg, client_socket);
        }
        memset(buffer, 0, 1024);
    }

    {
        std::lock_guard<std::mutex> guard(mtx);
        if (clients.count(client_socket)) {
            std::string disconnected_user = clients.at(client_socket);
            clients.erase(client_socket);
            if(is_running) {
                std::string goodbye_msg = "Server: " + disconnected_user + " has left.";
                std::cout << goodbye_msg << std::endl;
                broadcast_message(goodbye_msg, -1);
            }
        }
    }
    close(client_socket);
}

void Server::broadcast_message(const std::string& message, int sender_socket) {
    std::lock_guard<std::mutex> guard(mtx);
    for (auto const& [socket, name] : clients) {
        if (socket != sender_socket) {
            send(socket, message.c_str(), message.length(), 0);
        }
    }
}