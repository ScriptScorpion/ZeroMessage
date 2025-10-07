#include <iostream>
#include <vector>
#include <atomic>
#include <thread>
#include <cstring>
#include <algorithm>

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
	    std::string ip_id = "";
		std::vector <std::string> all_msgs {};

	public:
	    bool start(const int port) {
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
			Server_addr.sin_addr.s_addr = INADDR_ANY; // accept any IP-address (to set only connections from specific IP, type: `inet_pton(AF_INET, IP.c_str(), &Server_addr.sin_addr)`
			Server_addr.sin_port = htons(port); // Port

			if (bind(Server_socket, (sockaddr*)&Server_addr, sizeof(Server_addr)) == SOCKET_ERROR) { // this sets server IP address
				std::cerr << "Bind failed" << std::endl;
				return false;
			}

			if (listen(Server_socket, 5) == SOCKET_ERROR) {
				std::cerr << "Listen failed" << std::endl;
				return false;
			}

			running = true;
			std::cout << "Server started on port " << port << " and IP 127.0.0.1" << std::endl;
			
			acceptConnections();
			return true;
	    }

	    void stop() {
			running = false;
			
			for (int client : clients) {
				#ifdef _WIN32
				    closesocket(client);
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
	    void acceptConnections() {
			while (running) {
				sockaddr_in Client_addr {};
				#ifdef _WIN32
				    int addr_len = sizeof(Client_addr);
				#else
				    socklen_t addr_len = sizeof(Client_addr);
				#endif

				int Client_socket = accept(Server_socket, (sockaddr*)&Client_addr, &addr_len);
				if (Client_socket == INVALID_SOCKET) {
				    std::cerr << "Accept failed" << std::endl;
				     continue;
				}

				clients.push_back(Client_socket);

				char ClientIP[INET_ADDRSTRLEN];
				inet_ntop(AF_INET, &Client_addr.sin_addr, ClientIP, INET_ADDRSTRLEN);
				uint16_t IP_port = ntohs(Client_addr.sin_port);
				std::cout << "Client connected: " << ClientIP << ":" << IP_port << std::endl;
				ip_id += (ClientIP);
				ip_id += ':'; 
				ip_id += std::to_string(IP_port);
				std::thread ClientThread(&ChatServer::handleClient, this, Client_socket);
				ClientThread.detach();
			}
	    }

	    void handleClient(const int Client_socket) {
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
			#else
				close(Client_socket);
			#endif
			std::cout << "Client disconnected: " << ip_id << std::endl;
			ip_id.clear();
	    }

	    void broadcastMessage(const int SenderSocket) {
			std::string combined = "";
			for (const int &client : clients) {
				if (SenderSocket != client) { 
					for (const std::string &s : all_msgs) {
						combined += s;
						combined += ' ';
					}
					send(SenderSocket, combined.c_str(), combined.length(), 0);
				}
			}

	    }
};

int main() {
    ChatServer server;
	int port = 0;
	std::cout << "Enter port on which to start the server: ";
	std::cin >> port;
	if (!std::cin || port <= 0) {
		std::cerr << "Input error" << std::endl;
		return -1;
	}
    bool output = server.start(port); // port to start the server
	if (!output) {
		server.stop();
		return -1;
	}
    std::cout << "Press Enter to stop server..." << std::endl;
	std::cin.get();
    
    server.stop();
    return 0;
}
