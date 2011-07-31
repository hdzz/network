#include "proxy/Socks4Client.h"
#include "proxy/HTTPClient.h"

#include "proxy/Socks4Server.h"
#include "proxy/HTTPServer.h"
#include "proxy/Forwarder.h"

#include <iostream>
#include <string>
#include <sstream>

void help(std::ostream &out)
{
    out << "proxychain - simple implementation of several proxy "
        "clients/servers.\n"
        "This program allows the use of libproxy to chain several proxy "
        "servers. To do\nthat, specify the different proxies to use (in the "
        "right order) on the\ncommandline and finish with the local server to "
        "create on the local machine.\n"
        "Recognized commands are:\n"
        "  -hc, --httpclient <host> <port>: indicates a HTTP proxy server.\n"
        "  -s4c, --socks4-client <host> <port> <auth>: indicates a SOCKS4 "
        "proxy server.\nThe <auth> string will be sent to the server for "
        "authentification.\n"
        "  -s4s, --socks4-server <port>: creates a local SOCKS4 server, which "
        "will relay\nconnections through all the specified proxies.\n"
        "  -hs, --http-server <port>: creates a local HTTP server.\n"
        "  -fw, --forward <port> <target> <tport>: creates a forwarding TCP "
        "server, which\nwill connect to <target>:<tport> and redirect the "
        "received data there (a new\nconnection is made for each incoming "
        "connection).\n"
        "  -h, --help: displays this help screen and exists.\n";
}

template<typename T>
T fromString(const std::string &str, const T& def)
{
    std::istringstream iss(str);
    T t;
    if(iss.eof() || ((iss>>t), false) || iss.fail()
     || !iss.eof())
        return def;
    else
        return t;
}

int main(int argc, char **argv)
{
    try {
        Socket::Init();

        int i;
        Proxy *client = NULL;
        ProxyServer *server = NULL;
        for(i = 1; i < argc; i++)
        {
            std::string arg = argv[i];
            // Clients: the following commands add a proxy to the end of the
            // chain (i.e. it will be reached through all the previous ones)
            // HTTP
            if(arg == "-hc" || arg == "--http-client")
            {
                if(i+2 >= argc)
                {
                    std::cerr << "--http-client: missing parameters\n";
                    return 1;
                }
                int port = fromString<int>(argv[i+2], -1);
                if(port <= 0 || port > 65535)
                {
                    std::cerr << "--http-client: invalid port number\n";
                    return 1;
                }
                client = new HTTPClient(argv[i+1], port, client);
                i += 2;
            }
            // SOCKS4
            else if(arg == "-s4c" || arg == "--socks4-client")
            {
                if(i+3 >= argc)
                {
                    std::cerr << "--socks4-client: missing parameters\n";
                    return 1;
                }
                int port = fromString<int>(argv[i+2], -1);
                if(port <= 0 || port > 65535)
                {
                    std::cerr << "--socks4-client: invalid port number\n";
                    return 1;
                }
                client = new Socks4Client(argv[i+1], port, argv[i+3], client);
                i += 3;
            }
            // Help screen
            else if(arg == "-h" || arg == "--help")
            {
                help(std::cout);
                return 0;
            }
            // Servers: the following commands, to be placed at the end of the
            // commandline, indicate which server to create locally
            // SOCKS4
            else if(arg == "-s4s" || arg == "--socks4-server")
            {
                if(i+1 >= argc)
                {
                    std::cerr << "--socks4-server: missing parameters\n";
                    return 1;
                }
                else if(i+2 < argc)
                {
                    std::cerr << "error: --socks4-server must be the last "
                            "command\n";
                    return 1;
                }
                int port = fromString<int>(argv[i+1], -1);
                if(port <= 0 || port > 65535)
                {
                    std::cerr << "--socks4-server: invalid port number\n";
                    return 1;
                }
                server = new Socks4Server(port, client);
                i++;
            }
            // HTTP-CONNECT
            else if(arg == "-hs" || arg == "--http-server")
            {
                if(i+1 >= argc)
                {
                    std::cerr << "--http-server: missing parameters\n";
                    return 1;
                }
                else if(i+2 < argc)
                {
                    std::cerr << "erreur: --http-server must be the last "
                            "command\n";
                    return 1;
                }
                int port = fromString<int>(argv[i+1], -1);
                if(port <= 0 || port > 65535)
                {
                    std::cerr << "--http-server: invalid port number\n";
                    return 1;
                }
                server = new HTTPServer(port, client);
                i++;
            }
            // TCP forward
            else if(arg == "-fw" || arg == "--forward")
            {
                if(i+3 >= argc)
                {
                    std::cerr << "--forward: missing parameters\n";
                    return 1;
                }
                else if(i+4 < argc)
                {
                    std::cerr << "erreur: --forward must be the last command\n";
                    return 1;
                }
                int port = fromString<int>(argv[i+1], -1);
                int target_port = fromString<int>(argv[i+3], -1);
                if(port <= 0 || port > 65535
                 || target_port <= 8 || target_port > 65535)
                {
                    std::cerr << "--forward: invalid port number\n";
                    return 1;
                }
                server = new Forwarder(port, argv[i+2], target_port, client);
                i += 3;
            }
            else
            {
                std::cerr << "error: unrecognized command\n";
                help(std::cerr);
                return 1;
            }
        }

        if(server == NULL)
        {
            std::cerr << "error: you must specified a server\n";
            help(std::cerr);
            return 1;
        }

        for(;;)
            server->update(true);
    }
    catch(std::exception &e)
    {
        std::cerr << "fatal error: " << e.what() << std::endl;
        return 2;
    }
    
    return 0;
}