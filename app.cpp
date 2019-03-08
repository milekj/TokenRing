#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>
#include <netdb.h>
#include <ctime>
#include <fcntl.h>

using namespace std;

const int bufferSize = 256;
char buffer[bufferSize];
const size_t addressSize = sizeof(sockaddr_in);
const string NEW_CLIENT = "NEW_CLIENT";
const string CHANGE_DESTINATION = "CHANGE_DESTINATION";
const string EMPTY = "EMPTY";
const string MESSAGE = "MESSAGE";

int receiveSocket;
int sendSocket;
int senderSocket;
int controlSocket;

bool hasToken;


int connectAndBindSocket(int port);
void connectSocketToIP(int socket, string ip, int port);
int acceptSocket(int socket, sockaddr_in* address);
string createNewClientMessage(int port);
string createChangeDestinationMessage(string senderIP, string senderPort, string destinationIP, string destinationPort);
string createMessage(string contents, int port);
void readMessage();
string getMyIP();
string getTime();
void atexitHandler();
void SIGINT_handler(int sig);


int main(int argc, char *argv[])
{
    atexit(atexitHandler);
    signal(SIGINT, SIGINT_handler);

    int receivePort;
    int n;

    if (argc < 6) {
        cerr << "Not enough arguments";
        return 0;
    }

    string userID = argv[1];
    receivePort = stoi(argv[2]);
    string sendIP = argv[3];
    int sendPort = stoi(argv[4]);
    hasToken = (bool) stoi(argv[5]);
    string protocol = argv[6];

    receiveSocket = connectAndBindSocket(receivePort);
    sendSocket = connectAndBindSocket(0);

    listen(receiveSocket, 2);

    sockaddr_in senderAddress;

    if (hasToken) {

    }

    else {

    }

    if (hasToken) {
        senderSocket = acceptSocket(receiveSocket, &senderAddress);
        connectSocketToIP(sendSocket, sendIP, sendPort);

        readMessage();

        char* type = strtok(buffer, " ");
        if (strcmp(type, NEW_CLIENT.c_str()) == 0 ) {
            cout << "pomijam, bo to pierwsza i jest ladnie ustawione" << endl;
            string ip = strtok(NULL, " ");
            string port = strtok(NULL, " ");
            string message = createChangeDestinationMessage(getMyIP(), to_string(receivePort), ip, port);
            cout << message << endl;
        }
        else
            perror("wolololololoo");

        if (fcntl(receiveSocket, F_SETFL, O_NONBLOCK) < 0)
            perror("ustawianie nonblockingu");

        string message = createMessage("Zaczynamy!!!", receivePort);
        send(sendSocket, message.c_str(), message.length(), 0);

        while(true) {
            controlSocket = acceptSocket(receiveSocket, &senderAddress);
            if ( controlSocket < 0) {
                cout << "Nie ma durigego placznie" << endl;
            }

            readMessage();

            printf("Here is the message: %s\n",buffer);
            string m(buffer);
            cout << m;

            char* type = strtok(buffer, " ");
            if( strcmp(type, EMPTY.c_str()) == 0) {
                sleep(1);
                string message = createMessage(getTime(), receivePort);
                send(sendSocket, message.c_str(), message.length(), 0);
            }

            else {
                string ip = strtok(NULL, " ");
                string port = strtok(NULL, " ");

                if ( ip.compare(getMyIP()) == 0 && atoi(port.c_str()) == receivePort) {
                    string message = createMessage(getTime(), receivePort);
                    sleep(1);
                    send(sendSocket, message.c_str(), message.length(), 0);
                }

                else {
                    sleep(1);
                    send(sendSocket, message.c_str(), message.length(), 0);
                }

            }
        }

    }

    else {
        connectSocketToIP(sendSocket, sendIP, sendPort);

        string message = createNewClientMessage(receivePort);
        send(sendSocket, message.c_str(), message.length(), 0);

        senderSocket = acceptSocket(receiveSocket, &senderAddress);

        while(true) {
            sleep(1);
            string message = userID + ": " + getTime();
            send(sendSocket, message.c_str(), message.length(), 0);

            bzero(buffer,256);
            n = read(senderSocket,buffer,bufferSize);
            if (n < 0)
                perror("ERROR reading from socket");

            printf("Here is the message: %s\n",buffer);
            string m(buffer);
            cout << m;
        }
    }

}

sockaddr_in newAddress(string ip, int port) {
    sockaddr_in address;
    bzero((char *) &address, addressSize);
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = (ip.empty()) ? INADDR_ANY : inet_addr(ip.c_str());
    address.sin_port = (port == 0) ? 0 : htons(port);
    return address;
}

int connectAndBindSocket(int port) {
    int newSocket =  socket(AF_INET, SOCK_STREAM, 0);
    int option = 1;
    setsockopt(newSocket, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option));
    if (port < 0)
        perror("ERROR opening");

    sockaddr_in address = newAddress("", port);
    if (bind(newSocket, (struct sockaddr *) &address, addressSize) < 0)
        perror("ERROR on binding");

    return newSocket;
}

void connectSocketToIP(int socket, string ip, int port) {
    sockaddr_in address = newAddress(ip, port);
    if (connect(socket, (struct sockaddr *) &address, addressSize) < 0)
        perror("ERROR connecting no token send");
}

void atexitHandler() {
    close(receiveSocket);
    close(sendSocket);
    close(senderSocket);
}

void SIGINT_handler(int sig) {
    atexitHandler();
    exit(0);
}

int acceptSocket(int socket, sockaddr_in *address) {;
    socklen_t senderLength = sizeof(*address);
    int senderSocket = accept(socket, (sockaddr*) address, &senderLength);
    if (senderSocket < 0)
        perror("ERROR on accept");
    printf("server: got connection from %s port %d\n", inet_ntoa(address->sin_addr), ntohs(address->sin_port));
    return senderSocket;
}

string getTime() {
    time_t timer;
    time(&timer);
    return ctime(&timer);
}

string createNewClientMessage(int port) {
    string message = NEW_CLIENT + " " + getMyIP() + " " + to_string(port);
    return message;
}

string createChangeDestinationMessage(string senderIP, string senderPort, string destinationIP, string destinationPort) {
    return CHANGE_DESTINATION + " " + senderIP + " " + senderPort + " " + destinationIP + " " + destinationPort;
}

string getMyIP() {
    sockaddr_in myAddress;
    socklen_t myAddressLength = addressSize;
    getsockname(sendSocket, (sockaddr *) &myAddress, &myAddressLength);
    return string(inet_ntoa(myAddress.sin_addr));
}

string createMessage(string contents, int port) {
    return MESSAGE + " " + getMyIP() + " " + to_string(port) + " " + contents;
}

void readMessage() {
    bzero(buffer, bufferSize);
    int n = read(senderSocket, buffer, bufferSize);
    if (n < 0)
        perror("ERROR reading from socket");
}
