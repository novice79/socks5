#include <socks5.h>

int main(int argc, char *argv[])
{   
    Socks5 s5proxy(1080);
    s5proxy.run();
    return 0;
}