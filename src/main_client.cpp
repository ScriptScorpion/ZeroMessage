#include <iostream>
#include <cstdlib>
#include <atomic>
#include <ctime>
#include <thread>
#include <vector>
#include <chrono>

#define SUCCESS 0

#ifdef __unix__
    #define INVALID_SOCKET -1
    #define SOCKET_ERROR -1
    #include <sys/socket.h> // for creating socket, for binding address, for acceptance, for connecting to the server
    #include <arpa/inet.h> // convertation from string to struct
    #include <unistd.h> // system calls: close socket, send data, receive data.
    const char *clear = "clear";
#else
    #include <winsock2.h>
    #include <ws2tcpip.h>
    const char *clear = "cls";
#endif
std::string string_id() {
    srand(time(NULL));
    const int length = rand() % (25 - 5 + 1) + 5;
    const char chars[53] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz"; // 52 characters + null ter
    std::string result = ""; 
    for (int i = 0; i < length; ++i) {
        result += chars[rand() % 51]; // 51 - it is amount of characters - 1 because index starts with 0
    }   
    return result;
}

class App {
    private:
        const std::string str_id = string_id();
        std::string msg = "";
        int sock_m = 0;
        std::atomic <bool> allowed_to_send = false;
        std::thread receive_thread;
        sockaddr_in Server_addr {};
    public:
        void end() {
            allowed_to_send = false;
            if (receive_thread.joinable()) {
                receive_thread.detach(); // i'm saying close this thread separately from main thread
            }
        }
        void send_to_server(const std::string &message) {
            if (allowed_to_send) {
                if (send(sock_m, message.c_str(), message.length(), 0) == SOCKET_ERROR) {
                    std::cerr << "\nError: failed to send a message\n";
                    return;
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }
            else {
                std::cerr << "\nError: failed to connect to the server\n";
                #ifdef _WIN32
                    closesocket(sock_m);
                    WSACleanup();
                #else
                    close(sock_m);
                #endif
                std::exit(-1);
            }
        }
        void init (const std::string &ip, const int &port) {
            #ifdef _WIN32
                WSADATA wsaData;
                if (WSAStartup(MAKEWORD(2, 2), &wsaData) != SUCCESS) return;
            #endif

            int sock = socket(AF_INET, SOCK_STREAM, 0);
            if (sock == INVALID_SOCKET) {
                return;
            }

            Server_addr.sin_family = AF_INET;
            Server_addr.sin_port = htons(port);
            inet_pton(AF_INET, ip.c_str(), &Server_addr.sin_addr);
            sock_m = sock;
            if (connect(sock, (sockaddr*)&Server_addr, sizeof(Server_addr)) == SUCCESS)  {
                allowed_to_send = true;
            }
        }
        void create_msg(const std::string &message) {
            msg = message;
            std::string full_msg = "[" + str_id + "]: " + message;
            send_to_server(full_msg);
        }
        void get_msgs() {
            receive_thread = std::thread([this]() { // [this] because to capture all fileds of class
                while (allowed_to_send) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(100)); 
                    std::string all_msgs = "";
                    char chunk[1024];
                    int bytes_received = recv(sock_m, chunk, sizeof(chunk), 0);
                    if (bytes_received > 0) {
                        all_msgs += chunk;
                        for (const char &c : all_msgs) {
                            if ((c) == '[') {
                                std::cout << std::endl;
                            }
                            std::cout << c;
                        }
                        std::cout << std::endl;
                    }
                }
            });
        }
        int get_sock() {
           return sock_m;
        }
        std::string get_str_id() {
            return str_id;
        }
};

bool is_only_space(std::string str) {
    bool res = true;
    for (char x : str) {
        if (std::isalnum(x)) {
            res = false;
        }
    }
    return res;
}

int main() {
    std::string input = "";
    std::string ip = "";
    int port = 0;
    App instance;
    std::cout << "Enter IP of the server: ";
    std::cin >> ip;
    std::cout << "Enter Port of the server: ";
    std::cin >> port;
    if (!std::cin || port <= 0) {
        std::cerr << "Error: Invalid input" << std::endl;
        return -1;
    }
    std::cin.ignore(); // remove '\n' character from buffer 
    instance.init(ip, port);
    system(clear);
    instance.get_msgs();
    while (true) {
        std::cout << instance.get_str_id() << ": ";
        std::getline(std::cin, input);
        if (input == "q" || input.empty() || is_only_space(input)) {
            break;
        }
        instance.create_msg(input);

    }
    instance.end();
    #ifdef _WIN32
        closesocket(instance.get_sock());
        WSACleanup();
    #else
        close(instance.get_sock());
    #endif
    return 0;
}
