#include <iostream>
#include <cstring>
#include <vector>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define MAXSIZE 1024
#define PORT 8080

class TCPClient
{
private:
    int sock_cli, recvbit;
    struct sockaddr_in servaddr;
    char buffer[MAXSIZE];

public:
    TCPClient();
    void sendData(const std::vector<int> &MIDIinput);
};

TCPClient::TCPClient()
{
    if ((sock_cli = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        std::cerr << "[!][ERR] Error in socket creation" << std::endl;
        exit(EXIT_FAILURE);
    }

    std::memset(&servaddr, 0, sizeof(servaddr));

    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(PORT);
    servaddr.sin_addr.s_addr = inet_addr("127.0.0.1");

    if (connect(sock_cli, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0)
    {
        std::cerr << "[!][ERR] Error connecting to server" << std::endl;
        exit(EXIT_FAILURE);
    }
}

void TCPClient::sendData(const std::vector<int> &MIDIInput)
{
    while (true)
    {

        char notesToSend[MIDIInput.size() * sizeof(int)];
        size_t sendsize = MIDIInput.size() * sizeof(int);

        std::cout << "Send [ " << sendsize << " ] bytes of data" << std::endl;

        memcpy(notesToSend, MIDIInput.data(), MIDIInput.size() * sizeof(int));

        send(sock_cli, notesToSend, sizeof(notesToSend), 0);

        if ((recvbit = recv(sock_cli, buffer, sizeof(buffer), 0)) < 0)
        {
            std::cerr << "[!][ERR] Error receiving data" << std::endl;
            exit(EXIT_FAILURE);
        }
        else if (recvbit == 0)
        {
            std::cout << "[!][INFO] Client disconnected" << std::endl;
            exit(EXIT_FAILURE);
        }

        std::cout << "[!][MSG] Server Response: " << buffer << std::endl;
    }
}

int main()
{
    TCPClient client1;
    int notes;
    std::vector<int> MIDIInput;

    while (true)
    {
        std::cout << "Enter the notes (end with 0)" << std::endl;
        std::cin >> notes;
        if (notes != 0)
        {
            MIDIInput.push_back(notes);
        }
        else
        {
            break;
        }
    }
    client1.sendData(MIDIInput);

    return 0;
}