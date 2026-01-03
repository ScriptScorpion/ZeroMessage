#include <iostream>
#include <vector>
#include <atomic>
#include <thread>
#include <cstring>
#include <algorithm>
#include <mutex>
#include <chrono>

#define SUCCESS 0

#ifdef _WIN32
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #pragma comment(lib, "ws2_32.lib")
#else
    #define INVALID_SOCKET -1
    #define SOCKET_ERROR -1
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <ifaddrs.h>
    #include <unistd.h>
    #include <netdb.h>
    #include <arpa/inet.h>
#endif
class ChatServer {
	private:
	    int Server_socket = INVALID_SOCKET;
	    std::vector<int> clients {};
		std::atomic <bool> running = false;
		std::vector <std::string> all_msgs {};
		std::mutex mtx;
	public:
	    bool start(const std::string &ip, const int &port) {
			#ifdef _WIN32
				WSADATA wsaData;
				if (WSAStartup(MAKEWORD(2, 2), &wsaData) != SUCCESS) {
				    std::cerr << "WSAStartup failed" << std::endl;
				    return false;
				}
			#endif

			Server_socket = socket(AF_INET, SOCK_STREAM, 0);
			if (Server_socket == INVALID_SOCKET) {
				std::cerr << "Socket creation failed" << std::endl;
				return false;
			}

			int opt = 1;
			if (setsockopt(Server_socket, SOL_SOCKET, SO_REUSEADDR, (char*)&opt, sizeof(opt)) == SOCKET_ERROR) {
				std::cerr << "Setsockopt failed" << std::endl;
				return false;
			}

			sockaddr_in Server_addr {};
			Server_addr.sin_family = AF_INET;
			inet_pton(AF_INET, ip.c_str(), &Server_addr.sin_addr);
			Server_addr.sin_port = htons(port); // Port

			if (bind(Server_socket, (sockaddr*)&Server_addr, sizeof(Server_addr)) == SOCKET_ERROR) { // this sets server IP address
				std::cerr << "Bind failed" << std::endl;
				return false;
			}

			if (listen(Server_socket, 5) == SOCKET_ERROR) { // 5 - amount of connections
				std::cerr << "Listen failed" << std::endl;
				return false;
			}

			running = true;
			std::cout << "Server started on port " << port << " and IP " << ip << std::endl;
			
			acceptConnections();
			return true;
	    }
		~ChatServer() {
			running = false;
			
			for (int client : clients) {
				#ifdef _WIN32
				    closesocket(client);
					WSACleanup();
				#else
				    close(client);
				#endif
			}
			clients.clear();

			#ifdef _WIN32
			    closesocket(Server_socket);
			    WSACleanup();
			#else
				close(Server_socket);
			#endif
		}

	private:
		void handleClient(const int &Client_socket, const std::string &ip_id) {
			char buffer[1024];
			while (running) {
				int BytesReceived = recv(Client_socket, buffer, sizeof(buffer) - 1, 0);
				if (BytesReceived <= 0) {
				    break;
				}
				buffer[BytesReceived] = '\0';
				std::cout << buffer << std::endl; // message	
				all_msgs.push_back(buffer);
				broadcastMessage(Client_socket);
			}

			clients.erase(std::remove(clients.begin(), clients.end(), Client_socket), clients.end());
			
			#ifdef _WIN32
				closesocket(Client_socket);
				WSACleanup();
			#else
				close(Client_socket);
			#endif
			std::cout << "Client disconnected: " << ip_id << std::endl;
	    }

	    void broadcastMessage(const int &SenderSocket) {
			std::string combined = "";
			for (const int &client : clients) {
				if (SenderSocket != client) { 
					for (const std::string &s : all_msgs) { // sending in one line without spaces
						combined += s;
						combined += ' ';
					}
					send(client, combined.c_str(), combined.length(), 0);
				}
			}

	    }
	    void acceptConnections() {
			sockaddr_in Client_addr {};
			#ifdef _WIN32
				int addr_len = sizeof(Client_addr);
			#else
				socklen_t addr_len = sizeof(Client_addr);
			#endif
			std::lock_guard<std::mutex> lock(mtx);
			while (running) {
				int Client_socket = accept(Server_socket, (sockaddr*)&Client_addr, &addr_len);
				if (Client_socket == INVALID_SOCKET) {
					std::this_thread::sleep_for(std::chrono::milliseconds(10));
					continue;
				}
				clients.push_back(Client_socket);

				char ClientIP[INET_ADDRSTRLEN];
				inet_ntop(AF_INET, &Client_addr.sin_addr, ClientIP, INET_ADDRSTRLEN);
				uint16_t IP_port = ntohs(Client_addr.sin_port);
				std::cout << "Client connected: " << ClientIP << ":" << IP_port << std::endl;
				std::string ip_id = std::string(ClientIP) + ":" + std::to_string(IP_port);
				std::thread Client_thread(&ChatServer::handleClient, this,  Client_socket, ip_id);
				Client_thread.detach();
			}
	    }
};

int main() {
    ChatServer server;
	int port = 0;
	std::string ip = "";
	std::cout << "Enter IP on which to start the server: ";
	std::cin >> ip;
	std::cout << "Enter port on which to start the server: ";
	std::cin >> port;
	if (!std::cin || port <= 0) {
		std::cerr << "Error: Invalid input" << std::endl;
		return -1;
	}
	std::cin.ignore(); // remove '\n' character from buffer
    bool output = server.start(ip, port); // port to start the server
	if (!output) {
		return -1;
	}
    std::cout << "Press Enter to stop server..." << std::endl;
	std::cin.get();
    return 0;
}
