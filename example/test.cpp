#include <socks5.h>

int main(int argc, char *argv[])
{   
    int port{1080};
    if(argc >= 2 && std::atoi(argv[1]) )
        port = std::atoi(argv[1]);
    // argv[0] for get executable path, and write log file there, if have any
    Socks5 s5proxy(port, argv[0]);
    s5proxy.run();
    return 0;
}