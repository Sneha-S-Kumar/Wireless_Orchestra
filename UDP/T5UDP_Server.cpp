// Required Libraries
#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <cstring>
#include <fluidsynth.h>
#include <map>
#include <string>
#include <sstream>

// Macro definitions
#define PORT 8080
#define MAX_SIZE 1024
void displayBanner()
{
    system("clear");
    system("figlet HIB-Server | lolcat");
    sleep(2);
}

class UDPServer
{
private:
    int serverSocket;
    sockaddr_in serverAddress, clientAddress;
    socklen_t clientAddressLength;
    fluid_settings_t *settings;
    fluid_synth_t *synth;
    fluid_audio_driver_t *adriver;

public:
    UDPServer();
    ~UDPServer();
    bool checkInstrumentChange(int insID);
    bool initServer();
    void loadSoundFonts();
    void startServer();
};

UDPServer::UDPServer()
{
    displayBanner();
    bool isConnected = initServer();
    if (isConnected)
    {
        system("clear");
        system("toilet Connected | lolcat");
        loadSoundFonts();
        sleep(2);
        system("echo Starting server... | lolcat");
        clientAddressLength = sizeof(clientAddress);
    }
}

UDPServer::~UDPServer()
{
    delete_fluid_audio_driver(adriver);
    delete_fluid_synth(synth);
    delete_fluid_settings(settings);
    close(serverSocket);
}

bool UDPServer::checkInstrumentChange(int insID)
{
    static bool pianoActive = true;
    static bool fluteActive = false;
    static bool drumsActive = false;

    if (insID == 0 && !pianoActive)
    {
        pianoActive = true;
        drumsActive = false;
        fluteActive = false;
        return true;
    }
    else if (insID == 1 && !fluteActive)
    {
        pianoActive = false;
        drumsActive = false;
        fluteActive = true;
        return true;
    }
    else if (insID == 2 && !drumsActive)
    {
        drumsActive = true;
        pianoActive = false;
        fluteActive = false;
        return true;
    }
    else
    {
        return false;
    }
}

bool UDPServer::initServer()
{
    bool isConnected = false;

    serverSocket = socket(AF_INET, SOCK_DGRAM, 0);
    if (serverSocket < 1)
    {
        system("echo Socket Creation Error | lolcat");
        exit(EXIT_FAILURE);
        isConnected = false;
    }
    else
    {
        isConnected = true;
    }
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_addr.s_addr = INADDR_ANY;
    serverAddress.sin_port = htons(PORT);

    if (bind(serverSocket, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) == -1)
    {
        system("echo Binding Error | lolcat");
        exit(EXIT_FAILURE);
        isConnected = false;
    }
    else
    {
        isConnected = true;
    }

    return isConnected;
}

void UDPServer::loadSoundFonts()
{
    // create fluidsynth object

    settings = new_fluid_settings();
    fluid_settings_setstr(settings, "audio.driver", "alsa");
    synth = new_fluid_synth(settings);
    adriver = new_fluid_audio_driver(settings, synth);
    fluid_synth_set_gain(synth, 2.0);

    // load FS soundfonts
    if (fluid_synth_sfload(synth, "House_Piano_4th.sf2", 1) == FLUID_FAILED)
    {
        system("echo Error Loading Piano Sound font :(| lolcat");
        exit(1);
    }
    else
    {
        system("echo Piano SoundFile loaded (^_^) | lolcat");
    }
    if (fluid_synth_sfload(synth, "FLUTE2.SF2", 2) == FLUID_FAILED)
    {
        system("echo Error Loading Flute Sound font :(| lolcat");
        exit(1);
    }
    else
    {
        system("echo Flute SoundFile loaded (^_^) | lolcat");
    }
    if (fluid_synth_sfload(synth, "fluidR3_gm.sf2", 3) == FLUID_FAILED)
    {
        system("echo Error Loading Drums Sound font :(| lolcat");
        exit(1);
    }
    else
    {
        system("echo Drums SoundFile loaded (^_^) | lolcat");
    }
    fluid_synth_program_select(synth, 0, 1, 0, 0);
    fluid_synth_program_select(synth, 1, 2, 0, 0);
    fluid_synth_program_select(synth, 2, 3, 128, 5);
}

void UDPServer::startServer()
{
    char frame[MAX_SIZE];
    system("echo Server is listening on PORT 8080... | lolcat");
    sleep(2);
    system("clear");
    system("figlet HIB-Server | lolcat");
    system("echo version 1 | lolcat");
    system("echo ============================ | lolcat");
    system("echo [------------LOG-----------] | lolcat");
    system("echo ============================ | lolcat");

    while (1)
    {
        ssize_t bytesReceived = recvfrom(serverSocket, frame, sizeof(frame), 0, (struct sockaddr *)&clientAddress, &clientAddressLength);

        if (bytesReceived == -1)
        {
            perror("Receiving Error");
            exit(EXIT_FAILURE);
        }

        frame[bytesReceived] = '\0';
        std::istringstream iss(frame);
        std::string cnote;
        int octave, instrument, pitch;
        bool insChange = false;

        if (iss >> cnote >> octave >> instrument >> pitch)
        {
            if (checkInstrumentChange(instrument))
            {
                if (instrument == 0)
                {
                    system("figlet -w 3000 Piano | lolcat");
                }
                else if (instrument == 1)
                {
                    system("figlet -w 3000 Flute | lolcat");
                }
                else
                {
                    system("figlet -w 3000 Drums | lolcat");
                }
            }
            system("echo NOTES RECEIVED FROM CLIENT | lolcat");
            system("echo ---------------------------| lolcat");
            // Printing the parsed values
            std::cout << "NOTE: " << cnote << std::endl;
            std::cout << "OCTAVE: " << octave << std::endl;
            std::cout << "INSTRUMENT: " << instrument << std::endl;
        }
        else
        {
            std::cerr << "Parsing failed." << std::endl;
        }

        // Parse the note from the received message
        std::map<std::string, int> maps;
        maps["A"] = 33;
        maps["W"] = 34; // a#
        maps["B"] = 35;
        maps["C"] = 24;
        maps["V"] = 25; // c#
        maps["D"] = 26;
        maps["R"] = 27; // d#
        maps["E"] = 28;
        maps["F"] = 29;
        maps["T"] = 30; // f#
        maps["G"] = 31;
        maps["K"] = 57;
        maps["L"] = 40;
        maps["J"] = 60;
        maps["H"] = 54;

        int note = maps[cnote];
        int velocity;
        // std::cout << "Received note: " << note << std::endl;

        if (instrument == 2)
        {
            if ((pitch < -15) && (pitch > -25))
            {
                velocity = 40;
            }
            else if ((pitch < -25) && (pitch > -65))
            {
                velocity = 60;
            }
            else if ((pitch < -35) && (pitch > -80))
            {
                velocity = 80;
            }
            else if ((pitch < -80) && (pitch > -100))
            {
                velocity = 100;
            }
            else
            {
                velocity = 100;
            }
        }
        else
        {
            velocity = 100;
        }
        // Play the note for a short duration
        note += octave * 12;
        fluid_synth_noteon(synth, instrument, note, velocity);
        usleep(500000); // Sleep for 500 ms
        fluid_synth_noteoff(synth, instrument, note);
    }
}

void makeServer()
{
    UDPServer udps;
    udps.startServer();
}

int main()
{
    short int opt;

    while (1)
    {
        system("clear");
        system("echo Fusion510 Server Program | lolcat");
        system("echo Point the device NW to get started | lolcat");
        system("echo ======================================== | lolcat");
        system("echo 1. Start Server");
        system("echo 2. See Instrument List");
        system("echo 3. Exit");
        system("echo ======================================== | lolcat");

        std::cin >> opt;

        switch (opt)
        {
        case 1:
            makeServer();
            break;
        case 2:
            std::cout << "Piano [String Instrument]" << std::endl;
            std::cout << "Flute [Wind Instrument]" << std::endl;
            std::cout << "Drums [Percussion Instrument]" << std::endl;
            sleep(3);
            break;
        case 3:
            system("echo Exitting... | lolcat");
            exit(0);
            break;

        default:
            std::cout << "Wrong Option" << std::endl;
        }
    }

    return 0;
}
