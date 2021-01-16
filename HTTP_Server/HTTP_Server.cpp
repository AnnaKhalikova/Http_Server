#include <iostream>
#include <sstream>
#include <string>
#include <fstream>
#include <sdkddkver.h>
#include <conio.h>
#include <stdio.h>
#include <iostream>

 //Для корректной работы freeaddrinfo в MinGW
 //Подробнее: http://stackoverflow.com/a/20306451
#define _WIN32_WINNT 0x501


#include <WinSock2.h>
#include <WS2tcpip.h>

 //Необходимо, чтобы линковка происходила с DLL-библиотекой
 //Для работы с сокетам
#pragma comment(lib, "Ws2_32.lib")
using namespace std;
using std::cerr;

int calculatePercent(char buffer[]);
//Parse functions
void parseRequest(char request[], char text[]);
unsigned int charToInt(char chr);
char hexToDec(char* hex, int size);
int pow_(int num, int exp);

int main()
{
     

    WSADATA wsaData; // служебная структура для хранение информации
    /* о реализации Windows Sockets
     старт использования библиотеки сокетов процессом
     (подгружается Ws2_32.dll)*/
    int result = WSAStartup(MAKEWORD(2, 2), &wsaData);

     /*Если произошла ошибка подгрузки библиотеки*/
    if (result != 0) {
        cerr << "WSAStartup failed: " << result << "\n";
        return result;
    }

    struct addrinfo* addr = NULL; // структура, хранящая информацию
     /*об IP-адресе  слущающего сокета

     Шаблон для инициализации структуры адреса*/
    struct addrinfo hints;
    ZeroMemory(&hints, sizeof(hints));

    hints.ai_family = AF_INET; // AF_INET определяет, что будет
     //использоваться сеть для работы с сокетом
    hints.ai_socktype = SOCK_STREAM; // Задаем потоковый тип сокета
    hints.ai_protocol = IPPROTO_TCP; // Используем протокол TCP
    hints.ai_flags = AI_PASSIVE; // Сокет будет биндиться на адрес,
     /*чтобы принимать входящие соединения

     Инициализируем структуру, хранящую адрес сокета - addr
     Наш HTTP-сервер будет висеть на 8000-м порту локалхоста*/
    result = getaddrinfo("127.0.0.1", "8000", &hints, &addr);

     /*Если инициализация структуры адреса завершилась с ошибкой,
     выведем сообщением об этом и завершим выполнение программы*/
    if (result != 0) {
        cerr << "getaddrinfo failed: " << result << "\n";
        WSACleanup(); // выгрузка библиотеки Ws2_32.dll
        return 1;
    }

     //Создание сокета
    int listen_socket = socket(addr->ai_family, addr->ai_socktype,
        addr->ai_protocol);
     /*Если создание сокета завершилось с ошибкой, выводим сообщение,
     освобождаем память, выделенную под структуру addr,
     выгружаем dll-библиотеку и закрываем программу*/
    if (listen_socket == INVALID_SOCKET) {
        cerr << "Error at socket: " << WSAGetLastError() << "\n";
        freeaddrinfo(addr);
        WSACleanup();
        return 1;
    }

    /* Привязываем сокет к IP-адресу*/
    result = bind(listen_socket, addr->ai_addr, (int)addr->ai_addrlen);

     /*Если привязать адрес к сокету не удалось, то выводим сообщение
     об ошибке, освобождаем память, выделенную под структуру addr.
     и закрываем открытый сокет.
     Выгружаем DLL-библиотеку из памяти и закрываем программу.*/
    if (result == SOCKET_ERROR) {
        cerr << "bind failed with error: " << WSAGetLastError() << "\n";
        freeaddrinfo(addr);
        closesocket(listen_socket);
        WSACleanup();
        return 1;
    }

     //Инициализируем слушающий сокет
    if (listen(listen_socket, SOMAXCONN) == SOCKET_ERROR) {
        cerr << "listen failed with error: " << WSAGetLastError() << "\n";
        closesocket(listen_socket);
        WSACleanup();
        return 1;
    }


    const int max_client_buffer_size = 1024;
    char buf[max_client_buffer_size];
    char text[max_client_buffer_size];
    int client_socket = INVALID_SOCKET;

    for (;;) {
        std::stringstream response; // сюда будет записываться ответ клиенту
        std::stringstream response_body; // тело ответа
         //Принимаем входящие соединения
        client_socket = accept(listen_socket, NULL, NULL);
        if (client_socket == INVALID_SOCKET) {
            cerr << "accept failed: " << WSAGetLastError() << "\n";
            closesocket(listen_socket);
            WSACleanup();
            return 1;
        }

        result = recv(client_socket, buf, max_client_buffer_size, 0);



        if (result == SOCKET_ERROR) {
             //ошибка получения данных
            cerr << "recv failed: " << result << "\n";
            closesocket(client_socket);
        }
        else if (result == 0) {
             //соединение закрыто клиентом
            cerr << "connection closed...\n";
        }
        else if (result > 0) {
            /* Мы знаем фактический размер полученных данных, поэтому ставим метку конца строки
             В буфере запроса.*/
            //char indexPage[max_client_buffer_size];
            std::ifstream in("C:\\Users\\aikha\\Downloads\\HTTP_Server\\Debug\\index.html"); // окрываем файл для чтения


            string indexPage = "", s1;
            while (getline(in, s1))
                indexPage += s1;
  
            in.close();
            
            buf[result] = '\0';
            
             /*Данные успешно получены
             формируем тело ответа (HTML)*/
            parseRequest(buf, text);
             response_body << indexPage << "<h2>Percent of plagiarism:   " <<  calculatePercent(text) << " % </h2>";
             //TEMP COMMAND FOR TEST
             //std::cout << indexPage;
           

             /*Формируем весь ответ вместе с заголовками*/
            response << "HTTP/1.1 200 OK\r\n"
                << "Version: HTTP/1.1\r\n"
                << "Content-Type: text/html; charset=utf-8\r\n"
                << "Content-Length: " << response_body.str().length()
                << "\r\n\r\n"
                << response_body.str();

             //Отправляем ответ клиенту с помощью функции send
            result = send(client_socket, response.str().c_str(),
                response.str().length(), 0);

            if (result == SOCKET_ERROR) {
                 //произошла ошибка при отправле данных
                cerr << "send failed: " << WSAGetLastError() << "\n";
            }
             //Закрываем соединение к клиентом
            closesocket(client_socket);
        }
    }

     //Убираем за собой
    closesocket(listen_socket);
    freeaddrinfo(addr);
    WSACleanup();
    return 0;
}
int calculatePercent(char buffer[]) {
    cout << "Current info in buffer: " << buffer << endl;
    int length = 0;

    while (buffer[length] != '\0') {
        length++;
    }
    return length;
}
void parseRequest(char request[], char text[]) {
    int startIndex = 0, endIndex = 0, i = 0, j, k = 0;

    while (request[i] != '=') {
        startIndex = i;
        endIndex = i;
        i++;
    }
    startIndex = ++i;
    endIndex = i;
    while (request[i] != ' ') {
        endIndex = i;
        i++;
    }

    for (j = 0, k = startIndex; k <= endIndex; k++) {

        if (request[k] == '+') {
            text[j] = ' ';
        }
        else if (request[k] == '%') {
            int tempIndex = ++k, length = 0;
            char* tempHexNumber = new char[length];
            while ((request[tempIndex] >= '0' and request[tempIndex] <= '9')
                or (request[tempIndex] >= 'A' and request[tempIndex] <= 'F')) {
                tempHexNumber[length] = request[tempIndex];
                length++;
                tempIndex++;
            }
            char decNumber = hexToDec(tempHexNumber, length);
            text[j] = (char)decNumber;
            k += length;
        }
        else {
            text[j] = request[k];
        }
        j++;

    }
    text[j] = '\0';
}
unsigned int charToInt(char chr)
{
    if (chr >= '0' && chr <= '9')
        return chr - '0';
    else if (chr >= 'A' && chr <= 'F')
        return chr - 'A' + 10;
    else if (chr >= 'a' && chr <= 'f')
        return chr - 'a' + 10;
    return -1;
}
char hexToDec(char* hex, int size)
{
    // Изначальное десятичное значение
    char dec = 0;
    // Непосредственно преобразование шестнадцатеричного числа
    // в десятичное
    for (int j = 0, i = size - 1; j < size; ++j, --i) {
        dec += charToInt(hex[j]) * pow_(16, i);
    }
    // Возвращаем десятичное число
    return dec;
}
int pow_(int num, int exp)
{
    // Если показатель степени является нулем
    if (exp == 0) {
        // Любое число в нулевой степени является единицей
        return 1;
    }
    // Если показатель степени положителен
    if (exp > 0) {
        int result = 1;
        // Умножаем число само на себя exp раз
        for (int i = 0; i < exp; ++i) {
            result *= num;
        }
        // Возвращаем результат
        return result;
    }
    // Нам не нужно считать отрицательные степени
    return -1;
}
