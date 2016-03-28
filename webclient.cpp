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

const string FILE_NAME="index.html";
const int REQ_ARGC=2;
const int MAX_ATTEMPTS=5;   
const regex url_rex("^http://(www\\.)?([\\w\\.]+)(\\:(\\d+))?(\\/.*)?$");
const regex path_rex("^(\\/.*\\/)(.*)$");

string port_no(string part);
string get_path(string path);
void err_print(const char *msg);
bool try_connection(smatch url_part);
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
    string url = argv[1];
    
    if ( try_connection( get_matches(url,url_rex) ) ) 
    {
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

bool try_connection(smatch url_part)
{   // [0]full_url [1]www [2]domain [4]port [5]path
    const string url  = "www." + static_cast<string>(url_part[2]);
    const string port = port_no(static_cast<string>(url_part[4]));
    smatch full_path = get_matches(url_part[5], path_rex); 
    const string path = static_cast<string>(full_path[1]);
    //string file = get_filename(static_cast<string>(full_path[2])); //TODO
    
    
    
    struct addrinfo set;
    struct addrinfo *results, *res;
    int code, sckt;

    cout << path << endl << url << endl << port << endl << url_part[5] << endl ;


    memset(&set, 0, sizeof(struct addrinfo));
    set.ai_socktype = SOCK_STREAM; /* TCP Socket */
    set.ai_family = AF_UNSPEC;     /* Allow IPv4 or IPv6 */
    set.ai_protocol = 0;           /* Any protocol */
    set.ai_flags = 0;

    if ((code = getaddrinfo(url.c_str(), port.c_str(), &set, &results) ) != 0)
    { 
        err_print( gai_strerror(code) );
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


    close(sckt);    // closing socket after succesfull connection
    return EXIT_SUCCESS;
}

string port_no(string port)//TODO in one function
{
    return (port.size() != 0) ? port : "80" ;
}
string get_path(string path)
{
    return (path.size() != 0) ? path : "/" ;
}
void err_print(const char *msg)
{
    cerr << msg << endl;    
}
smatch get_matches(string url, regex rex) // TODO
{
    smatch matches;
    regex_match(url, matches, rex);
    return matches;
}
