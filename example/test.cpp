#include <socks5.h>

int main(int argc, char *argv[])
{   
    // argv[0] for get executable path, and write log file there, if have any
    Socks5 s5proxy(1080, argv[0]);
    s5proxy.run();
    return 0;
}