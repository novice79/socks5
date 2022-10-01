# BOOST_SOCKS5 Proxy Server

## Description

This is simple realization of socks5 proxy server according to the [RFC 1928](https://www.ietf.org/rfc/rfc1928.txt) using `boost::asio` library. The server uses async socket calls for data forwarding and io_service for message processing. Currently, this socks5 proxy server works only in `NO AUTHENTICATION REQUIRED` mode.

## Build

On *nix OS just run ./build.sh, example app & lib/include file will be installed in $PWD/dist dir

## test

in one screen window start socks5 proxy server

    dist/bin/test

in another window, test it via curl

    curl -x socks5://127.0.0.1:1080 baidu.com

