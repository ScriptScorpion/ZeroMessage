#include <iostream>
#include <string>
#include <cstring>

#ifdef _WIN32
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #pragma comment(lib, "ws2_32.lib")
	WSADATA wsaData;
    bool winsockInitialized = false;
#else
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <netdb.h>
    #include <unistd.h>
    #include <errno.h>
#endif

class NetworkChecker {
	private:
		void closeSocket(const int sock) {
		    #ifdef _WIN32
		        closesocket(sock);
		    #else
		        close(sock);
		    #endif
		}
	public:
		NetworkChecker() {
		    #ifdef _WIN32
		        int output = WSAStartup(MAKEWORD(2, 2), &wsaData);
		        winsockInitialized = (output == 0);
		    #endif
		}

		~NetworkChecker() {
		    #ifdef _WIN32
		    if (winsockInitialized) {
		        WSACleanup();
		    }
		    #endif
		}

		bool checkInternetConnection(const std::string& host = "example.com", const int port = 80, const int timeout_seconds = 5) {
		    
		    int sock = socket(AF_INET, SOCK_STREAM, 0);
		    if (sock <= 0) {
		        return false;
		    }

		    // Host IP(not normal variantion(not 127.0.0.1 kind of variantion))_
		    struct hostent* Server = gethostbyname(host.c_str());
		    if (Server == NULL) {
		        closeSocket(sock);
		        return false;
		    }

		    sockaddr_in Server_addr;
		    Server_addr.sin_family = AF_INET; // IPV4 
		    Server_addr.sin_port = htons(port); // Port 
		    
		    memcpy(&Server_addr.sin_addr.s_addr, Server->h_addr, Server->h_length);

		    #ifdef _WIN32
		        DWORD timeout = timeout_seconds * 1000; // Windows uses milliseconds
		        setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof(timeout));
		        setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, (const char*)&timeout, sizeof(timeout));
		    #else
		        timeval timeout;
		        timeout.tv_sec = timeout_seconds;
		        setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
		        setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout));
		    #endif

		    int connection_result = connect(sock, (sockaddr*)&Server_addr, sizeof(Server_addr));

		    bool result = (connection_result == 0);
		    
		    if (result) {
		        
		        std::string http_request = "GET / HTTP/1.1\r\nHost: example.com\r\nConnection: close\r\n\r\n";
		        
		        int send_result = send(sock, http_request.c_str(), http_request.length(), 0);
		        
		        if (send_result > 0) {
		            char buffer[17];
		            int receive_result = recv(sock, buffer, sizeof(buffer) - 1, 0);
		            
		            if (receive_result > 0) {
		                buffer[receive_result] = '\0';
		                std::cout << "Response: " << buffer << std::endl;
		            }
		        }
		    }
		    closeSocket(sock);
		    return result;
		}
};

int main() {
    NetworkChecker checker;
    
    if (checker.checkInternetConnection()) {
        std::cout << "[online]: You are connected to the internet" << std::endl;
    } else {
        std::cout << "[offline]: You are not connected to the internet" << std::endl;
    }
    
    return 0;
}
