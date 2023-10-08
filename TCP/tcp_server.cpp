#include <iostream>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <vector>
#include <unistd.h>
#include <fluidsynth.h>
#include <SDL2/SDL.h>

#define MAXSIZE 1024
#define PORT 8080

class MIDIPlayback
{
private:
    int channel = 0;
    fluid_settings_t *fluidSettings = nullptr;
    fluid_synth_t *synth = nullptr;
    fluid_audio_driver_t *audioDriver = nullptr;

public:
    MIDIPlayback()
    {
        // initialize the SDL
        SDL_Init(SDL_INIT_AUDIO);

        // fluid synth pointers for settings, the required driver, loading the soundfont
        fluidSettings = new_fluid_settings();
        synth = new_fluid_synth(fluidSettings);
        fluid_settings_setstr(fluidSettings, "audio.driver", "alsa");
        audioDriver = new_fluid_audio_driver(fluidSettings, synth);
        fluid_synth_sfload(synth, "/home/devuser/5_Linux/TCP/soundfont/fluidR3_gm.sf2", 1);
        fluid_synth_set_gain(synth, 2.0);
    }
    auto playNote(std::vector<int> notes, int noteVelocity);
    auto cleanPlayer();
};

auto MIDIPlayback::playNote(std::vector<int> notes, int noteVelocity)
{

    // loop through the array and play the notes
    for (int &size : notes)
    {
        fluid_synth_noteon(synth, channel, size, noteVelocity);
        // make the system play the note for 1 second(s)
        unsigned int microSecond = 1000000;
        usleep(1 * microSecond);
        fluid_synth_noteoff(synth, channel, size);
    }
}

auto MIDIPlayback::cleanPlayer()
{
    // release the drivers.
    delete_fluid_audio_driver(audioDriver);
    delete_fluid_synth(synth);
    delete_fluid_settings(fluidSettings);
    SDL_Quit();
}

class TCPServer : public MIDIPlayback
{
private:
    int sock_serv, sock_cli, recvbit;
    struct sockaddr_in servaddr, cliaddr;
    char buffer[MAXSIZE];

public:
    TCPServer();
    void serverListen(MIDIPlayback m_player);
};

TCPServer::TCPServer()
{
    if ((sock_serv = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        std::cerr << "[!][ERR] Error creating socket" << std::endl;
        exit(EXIT_FAILURE);
    }

    std::memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(PORT);
    servaddr.sin_addr.s_addr = INADDR_ANY;

    if (bind(sock_serv, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0)
    {
        std::cerr << "[!][ERR] Error in binding" << std::endl;
        exit(EXIT_FAILURE);
    }
}

void TCPServer::serverListen(MIDIPlayback m_player)
{
    listen(sock_serv, 5);

    std::cout << "[!][INFO] Server is listening on port " << PORT << std::endl;

    socklen_t client_len = sizeof(cliaddr);

    if ((sock_cli = accept(sock_serv, (struct sockaddr *)&cliaddr, &client_len)) < 0)
    {
        std::cerr << "[!][ERR] Error accepting connections" << std::endl;
    }

    std::vector<int> MIDIInput;

    while (true)
    {
        std::memset(buffer, 0, sizeof(buffer));
        if ((recvbit = recv(sock_cli, buffer, sizeof(buffer), 0)) < 0)
        {
            std::cerr << "[!][ERR] Error receiving data" << std::endl;
            exit(EXIT_FAILURE);
        }
        else if (recvbit == 0)
        {
            std::cout << "[!][INFO] Client disconnected" << std::endl;
        }

        int numInts = recvbit / sizeof(int);
        MIDIInput.resize(numInts);

        size_t recvsize = MIDIInput.size() * sizeof(int);

        std::cout << "[!][INFO] Received [ " << recvsize << " ] bytes of data" << std::endl;

        std::cout << "[!][MSG] Message received: " << buffer << std::endl;
        send(sock_cli, buffer, recvbit, 0);

        m_player.playNote(MIDIInput, 127);
    }
}

int main()
{
    TCPServer server;
    MIDIPlayback m_player;
    server.serverListen(m_player);
}