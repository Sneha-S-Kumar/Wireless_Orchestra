#include <stdlib.h>
#include <unistd.h>
#include <iostream>
int main()
{
    int hi = 1;
    system("echo HELLO | lolcat");
    sleep(1);
    std::cout << "Hello";
    system("echo HELLO | lolcat");

    // // Execute the "ls -l" command in the terminal
    // system("figlet -w 3000 FUSION 510 | lolcat");
    // sleep(3);
    // system("clear");
    // // system("toilet -w 3000 Starting... | lolcat");
    // system("echo hi | lolcat");

    return 0;
}