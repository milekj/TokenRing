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

const string NEW_CLIENT = "NEW_CLIENT";
const string CHANGE_DESTINATION = "CHANGE_DESTINATION";
const string EMPTY = "EMPTY";
const string MESSAGE = "MESSAGE";
const int SECONDS_TO_WAIT = 1;
const size_t ADDRESS_SIZE = sizeof(sockaddr_in);
const int BUFFER_SIZE = 256;

char buffer[BUFFER_SIZE];
string userID;
int receiveSocket;
int sendSocket;
int senderSocket;
int controlSocket;
int newSenderSocket;

int connectAndBindSocket(int port);
void connectSocketToIP(int socket, string ip, int port);
int acceptSocket(int socket, sockaddr_in* address);
string createNewClientMessage(int port);
string createChangeDestinationMessage(string senderIP, string senderPort, string destinationIP, string destinationPort);
string createMessage(string contents, int port);
string createEmptyMessage(int port);
void readMessage(int socket);
string getMyIP();
string getTime();
void sendMessageAfterSleep(string message);
void atexitHandler();
void SIGINT_handler(int sig);


int main(int argc, char *argv[])
{
    atexit(atexitHandler);
    signal(SIGINT, SIGINT_handler);

    int receivePort;

    if (argc < 6) {
        cerr << "Not enough arguments";
        return 0;
    }

    userID = argv[1];
    receivePort = stoi(argv[2]);
    string sendIP = argv[3];
    int sendPort = stoi(argv[4]);
    bool hasToken = (bool) stoi(argv[5]);
    string protocol = argv[6];

    receiveSocket = connectAndBindSocket(receivePort);
    sendSocket = connectAndBindSocket(0);

    listen(receiveSocket, 2);

    sockaddr_in senderAddress;


    if (hasToken) {
        senderSocket = acceptSocket(receiveSocket, &senderAddress);
        connectSocketToIP(sendSocket, sendIP, sendPort);

        readMessage(senderSocket);

        char* type = strtok(buffer, " ");
        if (strcmp(type, NEW_CLIENT.c_str()) == 0); //first message for first client discarded
        else {
            cerr << "First message was not of NEW_CLIENT type";
            exit(1);
        }

        string message = createMessage("Let's go!", receivePort);
        sendMessageAfterSleep(message);
    }

    else {
        connectSocketToIP(sendSocket, sendIP, sendPort);

        string message = createNewClientMessage(receivePort);
        send(sendSocket, message.c_str(), message.length(), 0);
        //cerr << "meludje wyslanie" << message;

        senderSocket = acceptSocket(receiveSocket, &senderAddress);
    }

    if (fcntl(receiveSocket, F_SETFL, O_NONBLOCK) < 0)
        perror("ustawianie nonblockingu");

    bool sentLastTime = false;
    bool needToChangeDestination = false;
    string changeDestinationMessage;
    string newSendIP;
    string newSendPort;

    while(true) {
        controlSocket = acceptSocket(receiveSocket, &senderAddress);
        if (controlSocket > 0) {
            readMessage(controlSocket);
            string message (buffer);

            char* type = strtok(buffer, " ");
            cerr << "\t" << message << endl;
            if (strcmp(type, NEW_CLIENT.c_str()) == 0 ) {
                string ip = strtok(NULL, " ");
                string port = strtok(NULL, " ");
                changeDestinationMessage = createChangeDestinationMessage(getMyIP(), to_string(receivePort), ip, port);
                needToChangeDestination = true;
                newSenderSocket = controlSocket;
            } else {
                perror("noo cos nie tak inny typ piersza od ocbeg");
            }


            //MOZE JAKIES CONTINUE
        }

        readMessage(senderSocket);
        string message (buffer);

        cerr << message << endl;
        //printf("Here is the message: %s\n",buffer);

        char* type = strtok(buffer, " ");
        string ip = strtok(NULL, " ");
        string port = strtok(NULL, " ");

        if( strcmp(type, EMPTY.c_str()) == 0) {
            if (needToChangeDestination) {
                senderSocket = newSenderSocket;
                sleep(1);
                send(sendSocket, changeDestinationMessage.c_str(), changeDestinationMessage.length(), 0);
                needToChangeDestination = false;
            }
            else if (!sentLastTime) {
                message = createMessage(getTime(), receivePort);
                sentLastTime = true;
                sleep(1);
                send(sendSocket, message.c_str(), message.length(), 0);
            } else {
                message = createEmptyMessage(receivePort);
                sentLastTime = false;
                sleep(1);
                send(sendSocket, message.c_str(), message.length(), 0);
            }

        }

        else if ( ip.compare(getMyIP()) == 0 && atoi(port.c_str()) == receivePort) {
            if (needToChangeDestination) {
                senderSocket = newSenderSocket;
                sleep(1);
                send(sendSocket, changeDestinationMessage.c_str(), changeDestinationMessage.length(), 0);
                needToChangeDestination = false;
            }
            else if (!sentLastTime) {
                message = createMessage(getTime(), receivePort);
                sentLastTime = true;
                sleep(1);
                send(sendSocket, message.c_str(), message.length(), 0);
            } else {
                message = createEmptyMessage(receivePort);
                sentLastTime = false;
                sleep(1);
                send(sendSocket, message.c_str(), message.length(), 0);
            }
        }

        else if (strcmp(type, CHANGE_DESTINATION.c_str()) == 0) {
            string newIP = strtok(NULL, " ");
            string newPort = strtok(NULL, " ");
            if ( ip == sendIP && port == (to_string(sendPort))) {
                sendIP = newIP;
                sendPort = stoi(newPort);
                close(sendSocket);
                sendSocket = connectAndBindSocket(0);
                connectSocketToIP(sendSocket, newIP, atoi(newPort.c_str()));
                sleep(1);
                message = createMessage(getTime(), receivePort);
                send(sendSocket, message.c_str(), message.length(), 0);
            }
            else {
                sleep(1);
                send(sendSocket, message.c_str(), message.length(), 0);
            }
        }

        else {
            sleep(1);
            send(sendSocket, message.c_str(), message.length(), 0);
        }
    }

}

sockaddr_in newAddress(string ip, int port) {
    sockaddr_in address;
    bzero((char *) &address, ADDRESS_SIZE);
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
    if (bind(newSocket, (struct sockaddr *) &address, ADDRESS_SIZE) < 0)
        perror("ERROR on binding");

    return newSocket;
}

void connectSocketToIP(int socket, string ip, int port) {
    sockaddr_in address = newAddress(ip, port);
    if (connect(socket, (struct sockaddr *) &address, ADDRESS_SIZE) < 0)
        perror("ERROR connecting no token send");
}

void atexitHandler() {
    close(receiveSocket);
    close(sendSocket);
    close(senderSocket);
    close(controlSocket);
    close(newSenderSocket);
}

void SIGINT_handler(int sig) {
    atexitHandler();
    exit(0);
}

int acceptSocket(int socket, sockaddr_in *address) {;
    socklen_t senderLength = sizeof(*address);
    int senderSocket = accept(socket, (sockaddr*) address, &senderLength);
    if (senderSocket < 0);
        //perror("ERROR on accept");
    //printf("server: got connection from %s port %d\n", inet_ntoa(address->sin_addr), ntohs(address->sin_port));
    return senderSocket;
}

string getTime() {
    time_t timer;
    time(&timer);
    string t = ctime(&timer);
    return t.substr(0, t.length() - 1);
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
    socklen_t myAddressLength = ADDRESS_SIZE;
    getsockname(sendSocket, (sockaddr *) &myAddress, &myAddressLength);
    return string(inet_ntoa(myAddress.sin_addr));
}

string createMessage(string contents, int port) {
    return MESSAGE + " " + getMyIP() + " " + to_string(port) + " " + userID + ": " + contents;
}

void readMessage(int socket) {
    bzero(buffer, BUFFER_SIZE);
    int n = read(socket, buffer, BUFFER_SIZE);
    //cerr << "\t!!!!!!!!!!!!!!!!!!!! " << n << endl;
    if (n < 0)
        perror("ERROR reading from socket");
    if (n == 0) {
        cerr << "Cannot connect to the neighbour - exiting";
        exit(1);
    }

}

string createEmptyMessage(int port) {
    return EMPTY + " " + getMyIP() + " " + to_string(port);
}

void sendMessageAfterSleep(string message) {
    sleep(SECONDS_TO_WAIT);
    send(sendSocket, message.c_str(), message.length(), 0);
}