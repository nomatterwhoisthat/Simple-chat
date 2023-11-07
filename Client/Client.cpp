#include <iostream>
#pragma comment(lib,"ws2_32.lib")//to get access to some functions
#include <winsock2.h>
#include <Windows.h>
#include <string>
#include <fstream>
#include <vector>
#include <nlohmann/json.hpp>
#pragma warning (disable:4996)

SOCKET Connection;

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

//создаем функцию для принятия сообщения,которое было отправлено сервером
void ClientHandler() {
    char msg[256];
    //создадим бесконечный цикл,в котором будем смотреть сообщение от сервера и выводить его на экран
    while (true) {
        recv(Connection, msg, sizeof(msg), NULL);

        std::cout << msg << '\n';
    }
}

//Функция проверяет совпадает ли имя пользователя и пароль из конфигурационного файла с тем,что ввел пользователь
bool isAuthenticated(const std::string& username, const std::string& password, const std::vector<UserData>& users, size_t size) {
    for (int i = 0; i < size; i++) {
        //  std::cout << username << ' ' << config[i].username << ' ' << password << ' ' << config[i].password << '\n';
        if (username == users[i].username && password == users[i].password) {
            return true;
        }
    }
    return false;
}

//Функция,проверяющая корректность сообщения
bool IsValidMessage(const char* message) {
    //Длина сообщения не больше 64 символов 
    if (strlen(message) > 64) {
        return false;
    }

    // Код символов ы сообщении имеют код ASCII >= 32.
    for (size_t i = 0; i < strlen(message); i++) {
        if (message[i] < 32) {
            return false;
        }
    }

    //Сообщение должно заканчиваться символами: '.', '!', or '?'.
    char lastChar = message[strlen(message) - 1];
    if (lastChar != '.' && lastChar != '!' && lastChar != '?') {
        return false;
    }


    return true;
}

int main(int argc, char* argv[]) {
    WSAData wsaData;
    WORD DLLVersion = MAKEWORD(2, 1);
    if (WSAStartup(DLLVersion, &wsaData) != 0) {
        std::cout << "Error: Failed to initialize Winsock." << std::endl;
        return 1;
    }

    SOCKADDR_IN addr;
    int sizeofaddr = sizeof(addr);
    addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    addr.sin_port = htons(1111);
    addr.sin_family = AF_INET;

    Connection = socket(AF_INET, SOCK_STREAM, NULL);
    if (connect(Connection, (SOCKADDR*)&addr, sizeofaddr) != 0) {
        std::cout << "Error: Failed to connect to the server." << std::endl;
        return 1;
    }


    std::vector<UserData> user = read_data();
    size_t size = user.size();
    std::string username;
    std::cout << "Enter your username: ";
    std::cin >> username;

    std::string password;
    std::cout << "Enter your password: ";
    std::cin >> password;
    if (!isAuthenticated(username, password, user, size)) {
        std::cout << "Authentication failed. Invalid username or password.";
        return -1;
    }

    if (isAuthenticated(username, password, user, size)) {
        std::cout << username << " entered to chat.\n";

    }

    send(Connection, username.c_str(), (int)username.size(), NULL);
    Sleep(100);
    send(Connection, password.c_str(), (int)password.size(), NULL);

    // std::cout << isAuthenticated(username, password, user) << '\n';
    CreateThread(NULL, NULL, (LPTHREAD_START_ROUTINE)ClientHandler, NULL, NULL, NULL);
    char msg[256]{ 0 };
    bool istrue = true;
    while (true) {
        std::cin.getline(msg, sizeof(msg));
        char new_[256]{ 0 };

        if (std::string(msg) == "exit.") {
            std::cout << "You left the chat.\n";
            send(Connection, msg, sizeof(msg), NULL);
            // recv(Connection, msg, sizeof(msg), NULL);
            return -2;
        }

        if (strlen(msg) == 0) {
            continue;
        }

        if (strlen(msg) != 0 && IsValidMessage(msg)) {
            std::string msg_with_username = username + ": " + msg;
            send(Connection, msg_with_username.c_str(), int(msg_with_username.size()), NULL);
        }
        else {
            std::cout << "Invalid message. Message must be:\n";
            std::cout << "less than 64 characters;\n";
            std::cout << "contain only printable ASCII characters;\n";
            std::cout << "end with '.', '!', or '?'.\n";
        }
    }
    closesocket(Connection);
    return 0;
}
