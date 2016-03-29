#include <cstdlib>

#include <iostream>
#include <string>

#include <regex>

#include <netinet/in.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

using namespace std;

const int U_PORT = 0;
const int U_PATH = 1;
const int F_NAME = 2;

const int REQ_ARGC = 2;
const int MAX_ATTEMPTS = 5;

const string DEF_PORT = "80";
const string DEF_PATH = "/";
const string DEF_NAME = "index.html";

const regex url_rex("^http://(www\\.)?([\\w\\.]+)(\\:(\\d+))?(\\/.*)?$");

void err_print(const char *msg);
bool try_connection(smatch url_parts);
bool send_req(int sckt);
string get_default(string str, int type);
smatch get_matches(string url, regex rex);

int main(int argc, char const *argv[])
{   
    if ( ! ( argc == REQ_ARGC ) )
    {
        err_print("Wrong parameters, usage: ./webclient url");
        return EXIT_FAILURE;
    }
    if ( ! regex_match(argv[1], url_rex) )
    {
        err_print("Bad url");
        return EXIT_FAILURE;
    }
    string url = argv[1]; // 
    if ( try_connection( get_matches(url,url_rex) ) ) 
    {
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

bool try_connection(smatch url_parts)
{   // [0]full_url [1]www [2]domain [4]port [5]path
    const string url  = "www." + static_cast<string>(url_parts[2]);
    const string port = get_default(static_cast<string>(url_parts[4]),U_PORT );
    string full_path = static_cast<string>(url_parts[5]);
    size_t found = full_path.find_last_of("/");
    const string path = get_default(full_path.substr(0,found+1),U_PATH );
    const string file = get_default(full_path.substr(found+1), F_NAME );

    bool get_err;
    int code, sckt;
    struct addrinfo set;
    struct addrinfo *results, *res;

    memset(&set, 0, sizeof(struct addrinfo));
    set.ai_socktype = SOCK_STREAM; /* TCP Socket */
    set.ai_family = AF_UNSPEC;     /* Allow IPv4 or IPv6 */
    set.ai_protocol = 0;           /* Any protocol */
    set.ai_flags = 0;

    if ((code = getaddrinfo(url.c_str(), port.c_str(), &set, &results) ) != 0)
    { 
        err_print( gai_strerror(code) );
        freeaddrinfo(results);
        return EXIT_FAILURE;
    }
    for (res = results; res != NULL; res = res->ai_next) // pripajam sa na prelozene adresy 
    {
        sckt = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
        if (sckt == -1)
        {
            continue;
        }
        if (connect(sckt, res->ai_addr, res->ai_addrlen) != -1)
        {
            freeaddrinfo(results);  // free no longer needed 
            break;                  // On Successfull connection
        }
        close(sckt);
    }
    if (res == NULL)
    {
        err_print("Could not connect");
        return EXIT_FAILURE;
    }
    get_err = send_req(sckt);
    close(sckt); 
    if (get_err)
    {
        return EXIT_FAILURE;
    }
    else
    {
        return EXIT_SUCCESS;
    }
}
bool send_req(int sckt)
{
    cout << sckt << endl;
    return EXIT_SUCCESS;
}

string get_default(string str, int type)//TODO in one function
{
    switch (type) 
    {
        case U_PORT : return (str.size() != 0) ? str : DEF_PORT ;
        case U_PATH : return (str.size() != 0) ? str : DEF_PATH ;
        case F_NAME : return (str.size() != 0) ? str : DEF_NAME ;
    }
    return NULL;
}
void err_print(const char *msg)
{
    cerr << msg << endl;    
}
smatch get_matches(string url, regex rex) // TODO
{
    smatch matches;
    regex_match(url, matches, rex);
    /*cout << "----------------------\n";
    for (unsigned int i = 0; i < matches.size(); ++i)
    {
        cout << "[" << i<< "]" << " = " << matches[i] << endl;
    }
    cout << "----------------------\n";*/
    return matches;
}
