# SOCKS5 Proxy Server

## Description

This is simple realization of socks5 proxy server according to the [RFC 1928](https://www.ietf.org/rfc/rfc1928.txt) using `boost::asio` library. The server uses async socket calls for data forwarding and io_service for message processing. Currently, this socks5 proxy server works only in `NO AUTHENTICATION REQUIRED` mode.

## Dependence

[boost](https://www.boost.org/)

*build tools:*   
g++ or clang++ and cmake+ninja

## Build

On *nix OS just run ./build.sh   
example app & lib/include file will be installed in $PWD/<u>dist</u> dir

## Test

in one screen window start socks5 proxy server

    dist/bin/s5

or show trace log like this

    LOG=trace dist/bin/s5

in another window, test it via curl

    curl -x socks5h://127.0.0.1:1080 baidu.com

