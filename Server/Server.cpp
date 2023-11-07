#include <iostream>
#pragma comment(lib,"ws2_32.lib")//to get access to some functions
#include <winsock2.h>
#include <Windows.h>
#include <string>
#include <fstream>
#include <vector>
#include <nlohmann/json.hpp>
#include <sstream>
#include <chrono>
#pragma warning (disable:4996)

std::vector<SOCKET> Connections;

int Counter = 0;
//Функция ,которая выводит в лог-файл каждое событие с указанием времени
void LogEvent(const std::string& logMessage) {
    std::ofstream logFile("log.txt", std::ios_base::app);
    if (logFile.is_open()) {
        auto current_time = std::chrono::system_clock::now();
        auto time_stamp = std::chrono::system_clock::to_time_t(current_time);
        std::stringstream time_stream;
        time_stream << std::put_time(std::localtime(&time_stamp), "%Y-%m-%d %X");
        logFile << "[" << time_stream.str() << "]" << logMessage;
        logFile.close();
    }
}

//Структура нужна для чтения данных из config.json
struct UserData {
    std::string username;
    std::string password;
};

//Считываем данные из config.json
std::vector<UserData> read_data() {
    std::vector<UserData> config(0);
    std::ifstream config_file("config.json");
    nlohmann::json jsonData;

    if (config_file.is_open()) {
        config_file >> jsonData;
        size_t size = jsonData["User"].size();
        config.resize(size);
        // std::cout << config.size() << '\n';
        for (int i = 0; i < size; ++i) {
            config[i].username = jsonData["User"][i]["Username"];
            config[i].password = jsonData["User"][i]["Password"];
           // std::cout << "Username: " << config[i].username << " ,password: " << config[i].password << " msg : " << config[i].visible_msg << '\n';
        }
    }
    else {
        std::cerr << "Error: could not open config.json \n";
    }
    return config;
}


//Функция проверяет совпадает ли имя пользователя и пароль из конфигурационного файла с тем,что ввел пользователь
bool isAuthenticated(const std::string& username, const std::string& password, const std::vector<UserData>& users, size_t size) {
    //  std::cout << size << '\n';
    for (int i = 0; i < size; i++) {
        std::cout << username << ' ' << users[i].username << ' ' << password << ' ' << users[i].password << '\n';
        if (username == users[i].username && password == users[i].password) {
            return true;
        }
    }
    return false;
}

void ClientHandler(int index) {
    char msg[256]{ 0 };//создаем массив ,в котором будем хранить переданное сообщение
    SOCKET clientSocket = Connections[index];
    char username[64]{ 0 };
    char password[64]{ 0 };
    std::vector<UserData>users = read_data();
    size_t size = users.size();
    recv(clientSocket, username, sizeof(username), NULL);
    std::string enteredUsername(username);
    recv(clientSocket, password, sizeof(password), NULL);


    std::string enteredPassword(password);
    // std::cout << enteredUsername << " " << enteredPassword << '\n';
   //  std::cout << "=" << isAuthenticated(enteredUsername, enteredPassword, users, size) << '\n';
    if (isAuthenticated(enteredUsername, enteredPassword, users, size)) {
        //Если данные пользователя из файла совпадает с тем,что ввел пользователь,то помечаем это событие в лог-файле
        std::string logMessage = enteredUsername + " is authenticated.\n";
        LogEvent(logMessage);
        std::cout << "logMessage:"<<logMessage;

        //нам нужен бесконечный цикл,в котором будут приниматься и отправляться сообщения клиентов
        while (true) {
            recv(Connections[index], msg, sizeof(msg), NULL);//принимаем сообщение от нужного соединения
            std::string leave_msg(msg);
            if (leave_msg == "exit.") {
                std::string user_left = enteredUsername + " left the chat.\n";
                std::string msg_user_left = enteredUsername + " left the chat.";
                std::cout << user_left;
                LogEvent(user_left);
                for (int i = 0; i < Counter; i++) {
                    if (i == index) {
                        continue;//мы не можем отправить сообщение от соединения,который его и отправил
                    }

                    send(Connections[i],msg_user_left.c_str(), msg_user_left.size(), NULL);
                }
               // closesocket(clientSocket);
              // Connections[index] = INVALID_SOCKET;

                break;
            }
            std::string recvLogMessage = "Received from " + enteredUsername + ": " + msg + '\n';
            LogEvent(recvLogMessage);

            for (int i = 0; i < Counter; i++) {
                if (i == index) {
                    continue;//мы не можем отправить сообщение от соединения,который его и отправил
                }
                send(Connections[i], msg, sizeof(msg), NULL);
            }

        }
    }
    else {
        std::string fail_msg = "Authentication failed for a client " + enteredUsername + '\n';
        std::cout << fail_msg;
        LogEvent(fail_msg);
        closesocket(clientSocket);
        Connections[index] = INVALID_SOCKET;

    }
}


int main(int argc, char* argv[]) {
    WSAData wsaData;
    WORD DLLVersion = MAKEWORD(2, 1);

    if (WSAStartup(DLLVersion, &wsaData) != 0) {
        std::cout << "Error: Failed to initialize Winsock." << std::endl;
        return 1;
    }


    //Добавляем информацию об адресе сокета
    SOCKADDR_IN addr;
    int sizeofaddr = sizeof(addr);
    addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    addr.sin_port = htons(1111);
    addr.sin_family = AF_INET;


    SOCKET sListen = socket(AF_INET, SOCK_STREAM, NULL);
    bind(sListen, (SOCKADDR*)&addr, sizeof(addr));
    listen(sListen, SOMAXCONN);
    std::vector<UserData>users = read_data();
    int max_Connections = static_cast<int>(users.size());
    //создаем новый сокет для поддержания соединения с клиентом
    SOCKET newConnection;
    for (int i = 0; i < max_Connections; i++) {
        //возращаем новый сокет ,который можно использовать для общения с клиентами
        newConnection = accept(sListen, (SOCKADDR*)&addr, &sizeofaddr);

        if (newConnection == 0) {
            std::cout << "Error #2\n";
        }
        else 
        {
            std::cout << "Client Connected!\n";
            //Connections[i] = newConnection;
            Connections.push_back(newConnection);
            Counter++;
            //Создаум поток ,где будет выполняться функция  ClientHandler
            CreateThread(NULL, NULL, (LPTHREAD_START_ROUTINE)ClientHandler, (LPVOID)(i), NULL, NULL);
  
        }
    }
  
    system("pause");
    return 0;
}
