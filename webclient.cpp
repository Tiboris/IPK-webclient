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
const regex rex("^http://(www\\.)?([\\w\\.]+)(\\:(\\d+))?(\\/.*)?$");

void err_print(const char *msg);
string port_no(string part);
string get_path(string path);
bool create_message(smatch url_part);

int main(int argc, char const *argv[])
{   
    if ( ! ( argc == REQ_ARGC ) )
    {
        err_print("Wrong parameters, usage: ./webclient url");
        return 1;
    }
    if ( ! regex_match(argv[1], rex) )
    {
        err_print("Bad url");
        return 1;
    }
    smatch matches;
    string url = argv[1];
    regex_match(url, matches, rex);
    if ( ! create_message(matches) ) 
    {
        return 1;
    }
    return 0;
}

bool create_message(smatch url_part)
{   // [0]full_url [1]www [2]domain [4]port [5]path
    const string url  = "www." + static_cast<string>(url_part[2]);
    const string path = get_path(static_cast<string>(url_part[5]));
    const string port = port_no(static_cast<string>(url_part[4]));
    
    struct addrinfo hints;
    struct addrinfo *result;
    int code;

    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_UNSPEC;    /* Allow IPv4 or IPv6 */
    hints.ai_socktype = SOCK_DGRAM; /* Datagram socket */
    hints.ai_flags = 0;
    hints.ai_protocol = 0;          /* Any protocol */

    if ((code = getaddrinfo(url.c_str(), port.c_str(), &hints, &result) )!= 0)
    { 
        err_print( gai_strerror(code) );
        return false;
    }
    cout << code << "=?0" << endl;

    return true;
}

string port_no(string port)
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
