// #include <iostream>
// #include <string>
// #include <vector>
// #include <map>
// #include <cstring>
// #include <unistd.h>
// #include <sys/socket.h>
// #include <netinet/in.h>
// #include <thread>
// #include <mutex>
// #include <algorithm>
// #include <sstream>
// #include <atomic>

// using namespace std;

// const int PORT = 8080;
// const int BUFFER_SIZE = 1024;

// map<int, string> clients;
// mutex mtx;
// atomic<bool> server_running(true);

// void broadcast_message(const string& message) {
//     lock_guard<mutex> guard(mtx);
//     for (auto const& [socket, name] : clients) {
//         send(socket, message.c_str(), message.length(), 0);
//     }
// }

// void broadcast_message(const string& message, int sender_socket) {
//     lock_guard<mutex> guard(mtx);
//     for (auto const& [socket, name] : clients) {
//         if (socket != sender_socket) {
//             send(socket, message.c_str(), message.length(), 0);
//         }
//     }
// }

// void handle_client(int client_socket) {
//     char buffer[BUFFER_SIZE];
    
//     int bytes_received;
//     while (server_running && (bytes_received = recv(client_socket, buffer, BUFFER_SIZE, 0)) > 0) {
//         string received_msg(buffer, bytes_received);
//         stringstream ss(received_msg);
//         string command;
//         ss >> command;

//         if (command == "NICK") {
//             string new_nickname;
//             if (ss >> new_nickname) {
//                 string notification;
//                 // **FIX**: Create a new scope for the lock_guard
//                 {
//                     lock_guard<mutex> guard(mtx);
//                     string old_nickname = clients[client_socket];
//                     clients[client_socket] = new_nickname;
//                     notification = "Server: " + old_nickname + " is now known as " + new_nickname + ".";
//                     cout << notification << endl;
//                 } // Mutex is automatically unlocked here as 'guard' goes out of scope

//                 // Now it's safe to call broadcast, which will acquire its own lock
//                 broadcast_message(notification);
//             }
//         } else if (command == "MSG") {
//             string recipient_nick, private_msg;
//             ss >> recipient_nick;
//             getline(ss, private_msg);

//             if (!private_msg.empty()) {
//                 private_msg = private_msg.substr(1);
//                 lock_guard<mutex> guard(mtx);
//                 int recipient_socket = -1;
//                 string sender_nick = clients[client_socket];

//                 for (auto const& [socket, name] : clients) {
//                     if (name == recipient_nick) {
//                         recipient_socket = socket;
//                         break;
//                     }
//                 }

//                 if (recipient_socket != -1) {
//                     string formatted_msg = "(private) " + sender_nick + ": " + private_msg;
//                     send(recipient_socket, formatted_msg.c_str(), formatted_msg.length(), 0);
//                 } else {
//                     string error_msg = "Server: Error - User '" + recipient_nick + "' not found.";
//                     send(client_socket, error_msg.c_str(), error_msg.length(), 0);
//                 }
//             }
//         } else if (command == "LIST") {
//             lock_guard<mutex> guard(mtx);
//             string user_list = "Server: Users online - ";
//             for (auto const& [socket, name] : clients) {
//                 user_list += name + ", ";
//             }
//             if (user_list.length() > 22) {
//                 user_list = user_list.substr(0, user_list.length() - 2);
//             }
//             send(client_socket, user_list.c_str(), user_list.length(), 0);
//         } else {
//             lock_guard<mutex> guard(mtx);
//             string sender_nick = clients[client_socket];
//             string public_msg = sender_nick + ": " + received_msg;
//             cout << public_msg << endl;
//             broadcast_message(public_msg, client_socket);
//         }
//         memset(buffer, 0, BUFFER_SIZE);
//     }

//     {
//         lock_guard<mutex> guard(mtx);
//         if (clients.count(client_socket)) {
//             string disconnected_user = clients.at(client_socket);
//             clients.erase(client_socket);
//             if(server_running) {
//                 string goodbye_msg = "Server: " + disconnected_user + " has left.";
//                 cout << goodbye_msg << endl;
//                 broadcast_message(goodbye_msg, -1);
//             }
//         }
//     }
//     close(client_socket);
// }


// int main() {
//     int server_fd;
//     struct sockaddr_in address;

//     if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
//         perror("socket failed"); exit(EXIT_FAILURE);
//     }

//     int opt = 1;
//     if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
//         perror("setsockopt"); exit(EXIT_FAILURE);
//     }

//     address.sin_family = AF_INET;
//     address.sin_addr.s_addr = INADDR_ANY;
//     address.sin_port = htons(PORT);

//     if (::bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
//         perror("bind failed"); exit(EXIT_FAILURE);
//     }

//     if (listen(server_fd, 10) < 0) {
//         perror("listen"); exit(EXIT_FAILURE);
//     }

//     cout << "Server listening on port " << PORT << "..." << endl;
//     cout << "Type 'SHUTDOWN' to close the server." << endl;

//     while (server_running) {
//         fd_set read_fds;
//         FD_ZERO(&read_fds);
//         FD_SET(server_fd, &read_fds);
//         FD_SET(STDIN_FILENO, &read_fds);

//         int fdmax = (server_fd > STDIN_FILENO) ? server_fd : STDIN_FILENO;

//         if (select(fdmax + 1, &read_fds, NULL, NULL, NULL) == -1) {
//             if (!server_running) break;
//             perror("select");
//             exit(EXIT_FAILURE);
//         }

//         if (FD_ISSET(STDIN_FILENO, &read_fds)) {
//             string command;
//             cin >> command;
//             if (command == "SHUTDOWN") {
//                 cout << "Shutdown command received. Closing server." << endl;
//                 server_running = false;
//             }
//         }
        
//         if (FD_ISSET(server_fd, &read_fds)) {
//             int new_socket = accept(server_fd, nullptr, nullptr);
//             if (new_socket < 0) {
//                 perror("accept"); continue;
//             }

//             {
//                 lock_guard<mutex> guard(mtx);
//                 string default_name = "User" + to_string(new_socket);
//                 clients[new_socket] = default_name;
//                 cout << "New connection on socket " << new_socket << " as " << default_name << endl;

//                 string welcome_msg = "Server: " + default_name + " has joined.";
//                 for (auto const& [socket, name] : clients) {
//                     if (socket != new_socket) {
//                         send(socket, welcome_msg.c_str(), welcome_msg.length(), 0);
//                     }
//                 }
//             }
            
//             thread t(handle_client, new_socket);
//             t.detach();
//         }
//     }

//     cout << "Notifying clients of shutdown..." << endl;
//     string shutdown_msg = "Server: Shutting down now. Goodbye!";
//     broadcast_message(shutdown_msg);

//     cout << "Closing all sockets..." << endl;
//     {
//         lock_guard<mutex> guard(mtx);
//         for(auto const& [socket, name] : clients) {
//             close(socket);
//         }
//         clients.clear();
//     }
    
//     close(server_fd);
//     cout << "Server closed." << endl;

//     return 0;
// }