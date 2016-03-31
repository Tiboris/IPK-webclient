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
const size_t BUFF_SIZE = 1024;

const string DEF_PORT = "80";
const string DEF_PATH = "/";
const string DEF_NAME = "index.html";
string filename;


const regex url_rex("^http://(www\\.)?([\\w\\.]+)(\\:(\\d+))?(\\/.*)?$");

void err_print(const char *msg);
string get_default(string str, int type);
smatch get_matches(string url, regex rex);
bool send_req(int sckt, string url, string file_path);
bool connect_to(smatch url_parts);
bool get_content();

int main(int argc, char const *argv[])
{   
    string filename;
    if ( ! ( argc == REQ_ARGC ) ) {
        err_print("Wrong parameters, usage: ./webclient url");
        return EXIT_FAILURE;
    }
    if ( ! regex_match(argv[1], url_rex) ) {
        err_print("Bad url");
        return EXIT_FAILURE;
    }
    string url = argv[1]; // 
    if ( connect_to(get_matches(url,url_rex)) ) {
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
    // 1.1 chunk
    // 1.0 no chunk
}

bool connect_to(smatch url_parts)
{   // [0]full_url [1]www [2]domain [4]port [5]path
    const string sub  = static_cast<string>(url_parts[1]);
    const string url  = static_cast<string>(url_parts[2]);
    const string port = get_default(static_cast<string>(url_parts[4]),U_PORT );
    const string full_path = static_cast<string>(url_parts[5]);
    const size_t found = full_path.find_last_of("/");
    const string path = get_default(full_path.substr(0,found+1),U_PATH );
    filename = get_default(full_path.substr(found+1), F_NAME );

    int code, sckt;
    bool send_err, req_err;
    struct addrinfo set;
    struct addrinfo *results, *res;

    memset(&set, 0, sizeof(struct addrinfo));
    set.ai_socktype = SOCK_STREAM; /* TCP Socket */
    set.ai_family = AF_UNSPEC;     /* Allow IPv4 or IPv6 */
    set.ai_protocol = 0;           /* Any protocol */
    set.ai_flags = 0;

    if ((code = getaddrinfo( (sub + url).c_str(), port.c_str(), &set, &results) ) != 0) { 
        err_print( gai_strerror(code) );
        return EXIT_FAILURE;
    }
    for (res = results; res != NULL; res = res->ai_next)  
    {   // connecting to translated addresses
        sckt = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
        if (sckt == -1){
            continue;
        }
        if (connect(sckt, res->ai_addr, res->ai_addrlen) != -1){
            freeaddrinfo(results);  // free no longer needed 
            break;                  // On Successfull connection
        }
        close(sckt);
    }
    if (res == NULL){
        err_print("Could not connect");
        return EXIT_FAILURE;
    }
    send_err = send_req(sckt, (sub+url), full_path);
    req_err = get_content();
    close(sckt); 
    if (send_err){
        return EXIT_FAILURE;
    }
    else if (req_err){
        return EXIT_FAILURE;
    }
    else{
        return EXIT_SUCCESS;
    }
}
bool send_req(int sckt, string url, string file_path)
{
    int res;
    char response[BUFF_SIZE];
    string resp_msg = "";
    string msg =    "HEAD " + file_path + " HTTP/1.1\r\n" + 
                    "Host: " + url + "\r\n" + "Connection: close\r\n\r\n";
    cout << msg << endl << endl;
    if ( send(sckt, msg.c_str(),msg.size(), 0) == -1 ) {
        err_print("Error while sending request");
        return EXIT_FAILURE;
    } 
    while ( (res = recv(sckt, response, BUFF_SIZE, 0 )) )
    {
        if (res < 0) {
            err_print("Get ERR");
            return EXIT_FAILURE;
        }
        else { // OK
            resp_msg += response;
        }
    }
    cout << resp_msg << endl << resp_msg.length()<< endl;

    return EXIT_SUCCESS;
}
bool get_content ()
{
    for (int i = 1; i <= MAX_ATTEMPTS; ++i)
    {
        //cout << i << endl;
        if (i==4)
        {
            return EXIT_SUCCESS;   
        }
    }
    return EXIT_FAILURE;
}
string get_default(string str, int type)
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
smatch get_matches(string url, regex rex)
{
    smatch matches;
    regex_match(url, matches, rex);
    return matches;
}
