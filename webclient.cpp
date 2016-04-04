#include <cstdlib>
#include <iostream>
#include <fstream>
#include <string>
#include <regex>
#include <netinet/in.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
/*
*
*/
using namespace std;
/*
*   Used constants and variables
*/
const int U_PORT = 0;
const int U_PATH = 1;
const int F_NAME = 2;
const int REQ_ARGC = 2;
const int MAX_ATTEMPTS = 5;
const size_t BUFF_SIZE = 1024;
const string DEF_PORT = "80";
const string DEF_PATH = "/";
const string DEF_NAME = "index.html";
string file_name;
string known_urls = "";
int attempts = 0; 
/*
* Used regular expressions
*/
const regex url_rex("^http://(www\\.)?([\\w\\.-]+)(\\:(\\d+))?(\\/.*)?$");
const regex head_rex("HTTP\\/1.(\\d) (\\d+) (.*)");
const regex chunk_rex("Transfer-Encoding: chunked");
const regex locattion_rex("Location: (.*)");
const regex space(" ");
const regex tilda("~");
/*
* Prototypes of functions
*/
void err_print(const char *msg);
string get_default(string str, int type);
smatch get_rex_matches(string url, regex rex);
string get_cont(int sckt, string url, string file_path, string ver);
bool connect_to(string url,string port,string path, int attempts, string ver="1");
bool save_response(string response, bool chunked);
/*
*   Main
*/
int main(int argc, char const *argv[])
{   
    if ( ! ( argc == REQ_ARGC ) ) {
        err_print("Wrong parameters, usage: ./webclient url");
        return EXIT_FAILURE;
    }
    if ( ! regex_match(argv[1], url_rex) ) {
        err_print("Bad url");
        return EXIT_FAILURE;
    }
    string url = argv[1];
    smatch url_parts;
    regex_match(url,url_parts,url_rex);// [0]full_url [1]www [2]domain [4]port [5]path
    const string path = get_default(static_cast<string>(url_parts[5]), U_PATH);
    const string port = get_default(static_cast<string>(url_parts[4]),U_PORT );
    url  = static_cast<string>(url_parts[1]) + static_cast<string>(url_parts[2]);
    if ( connect_to(url, port, path, attempts) ) {
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
/*
*   For connection handling
*/
bool connect_to(string url,string port,string path, int attempts, string ver)
{   
    size_t pos = path.find_last_of("/");
    if (attempts == 0) {
        file_name = get_default(path.substr(pos+1), F_NAME );
    }
    if (attempts > MAX_ATTEMPTS) {
        err_print("Too many redirects");
        return EXIT_FAILURE;
    } 
    string to_find = "^"+url+">>>>";
    size_t jump;
    if ( (jump = known_urls.find(to_find)) != string::npos) {
        string cut = known_urls.substr(jump);
        jump = to_find.size();
        cut=cut.substr(jump);
        jump = cut.find_first_of("|");
        string found_url="http://"+cut.substr(0,jump)+path;
        smatch url_part;
        regex_match(found_url,url_part,url_rex);
        path = get_default(static_cast<string>(url_part[5]), U_PATH);
        port = get_default(static_cast<string>(url_part[4]),U_PORT );
        url  = static_cast<string>(url_part[1]) + static_cast<string>(url_part[2]);
    }

    int adr_code, resp_code, sckt;
    struct addrinfo set;
    struct addrinfo *results, *res;

    memset(&set, 0, sizeof(struct addrinfo));
    set.ai_socktype = SOCK_STREAM; /* TCP Socket */
    set.ai_family = AF_UNSPEC;     /* Allow IPv4 or IPv6 */
    set.ai_protocol = 0;           /* Any protocol */
    set.ai_flags = 0;

    if ((adr_code = getaddrinfo( url.c_str(), port.c_str(), &set, &results) ) != 0) { 
        err_print( gai_strerror(adr_code) );
        return EXIT_FAILURE;
    }
    for (res = results; res != NULL; res = res->ai_next)  
    {   // connecting to translated addresses
        sckt = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
        if (sckt == -1) {
            continue;
        }
        if (connect(sckt, res->ai_addr, res->ai_addrlen) != -1) {
            freeaddrinfo(results);  // free no longer needed 
            break;                  // On Successfull connection
        }
        close(sckt);
    }
    if (res == NULL){
        err_print("Could not connect");
        return EXIT_FAILURE;
    }
    // Saving response
    string resp_msg = get_cont(sckt, url, path, ver);
    if (resp_msg == "ErrInMyGet") {
        return EXIT_FAILURE;
    }
    // Iterator for regex output
    smatch head;
    regex_search(resp_msg, head, head_rex);
    string res_ver = static_cast<string>(head[1]);
    string s_code = static_cast<string>(head[2]);
    resp_code = atoi(s_code.c_str());
    string err_msg = static_cast<string>(head[3]);
    bool chunked = regex_search(resp_msg, head, chunk_rex);
    close(sckt);    // closing socket
    if (resp_code == 200) { // OK
        if ( res_ver == ver ) {
            if (save_response(resp_msg, chunked)){
                return EXIT_FAILURE;
            }
            return EXIT_SUCCESS;
        }
        if ( res_ver != ver ) {
            if( connect_to(url,port, path, attempts, "0") ){
                return EXIT_FAILURE;
            }
            return EXIT_SUCCESS;
        }        
    }
    if (resp_code == 302 || resp_code == 301) { // REDIRECT
        regex_search(resp_msg, head, locattion_rex);
        string location = static_cast<string>(head[1]);
        string head_location = location;
        smatch url_parts;
        regex_match(location,url_parts,url_rex);
        path = get_default(static_cast<string>(url_parts[5]), U_PATH);
        port = get_default(static_cast<string>(url_parts[4]), U_PORT);
        location = static_cast<string>(url_parts[1]) + static_cast<string>(url_parts[2]);
        if (location == "") { // if regex matched bad url after location 
            err_print(("Bad "+s_code+" Location in HEAD response: "+head_location).c_str());
            return EXIT_FAILURE;
        }
        if (resp_code == 301 ) { // saving url to cache
            if ( known_urls.find("^"+url+">>>>"+location+"|\n") == string::npos ){
                known_urls.append("^"+url+">>>>"+location+"|\n");
            }        
        }
        if (connect_to(location,port,path, ++attempts) ) { 
            return EXIT_FAILURE;
        }
        return EXIT_SUCCESS;
    }
    if ((resp_code==400) && (res_ver != ver)) { // BAD REQUEST
        if( connect_to(url,port, path, attempts, "0") ){
            return EXIT_FAILURE;
        }
        return EXIT_SUCCESS;
    }
    if (resp_code > 399 && resp_code < 600) { // OTHER ERRORS
        err_print((s_code + " " + err_msg).c_str());
        return EXIT_FAILURE;
    }
    // OTHER NOT KNOWN ERRORS
    err_print(( "ERR : " + s_code + " " + err_msg).c_str());
    return EXIT_FAILURE;
}
/*
*   For sending head request and returning response
*/
string get_cont( int sckt, string url, string file_path, string ver){
    int res;
    char response[BUFF_SIZE];
    bzero(response, BUFF_SIZE);
    string resp_msg = "";
    string replacement[2] = { "%20","%7E"};
    file_path  = regex_replace(file_path,space,replacement[0]);
    file_path  = regex_replace(file_path,tilda,replacement[1]);
    string msg =    "GET " + file_path + " HTTP/1." + ver + "\r\n" + 
                    "Host: " + url + "\r\n" + "Connection: close\r\n\r\n";
    if ( (send(sckt, msg.c_str(),msg.size(), 0)) < 1 ) {
        err_print("Error while sending request");
        return "ErrInMyGet";
    }
    while ( (res = recv(sckt, response, BUFF_SIZE-1, 0 )) )
    {
        if (res < 1) {
            err_print("Error while getting sesponse");
            return "ErrInMyGet";
        }
        else { // response get 
            resp_msg.append(response, res);
            bzero(response, BUFF_SIZE); // buff erase
        }
    }
    return resp_msg;
}
/*
*   For writing into file
*/
bool save_response(string response, bool chunked)
{
    size_t pos = response.find("\r\n\r\n");
    response.erase(0, pos+4);
    if (chunked) {
        string chunked_resp = response;
        response = "";
        unsigned int block_size;
        while ((pos = chunked_resp.find("\r\n")) != string::npos )
        {   // real cutting of string magic
            block_size = strtoul(chunked_resp.substr(0,pos).c_str(), NULL, 16);
            chunked_resp.erase(0, pos+2);
            response += chunked_resp.substr(0,block_size);
            chunked_resp.erase(0,block_size);
        }
    }
    ofstream myfile;
    myfile.open(file_name);
    myfile << response;
    myfile.close();
    return EXIT_SUCCESS;
}
/*
*   For getting Default values
*/
string get_default(string str, int type)
{
    switch (type) 
    {
        case U_PORT : return (str.size() != 0) ? str : DEF_PORT ;
        case F_NAME : return (str.size() != 0) ? str : DEF_NAME ;
        case U_PATH : return (str.size() != 0) ? str : DEF_PATH ;
    }
    return NULL;
}
/*
*   For printing errors
*/
void err_print(const char *msg)
{
    cerr << msg << endl;    
}
