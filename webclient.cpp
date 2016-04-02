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
*/
using namespace std;
/*
*/
const int U_PORT = 0;
const int U_PATH = 1;
const int F_NAME = 2;
const int REQ_ARGC = 2;
const int MAX_ATTEMPTS = 5;
const size_t BUFF_SIZE = 2048;
const string DEF_PORT = "80";
const string DEF_PATH = "/";
const string DEF_NAME = "index.html";
string file_name;
string known_urls = "";
int attempts = 0; 
/*
*/
const regex url_rex("^http://(www\\.)?([\\w\\.]+)(\\:(\\d+))?(\\/.*)?$");
const regex head_rex("HTTP\\/1.(\\d) (\\d+) (.*)");
const regex chunk_rex("Transfer-Encoding: chunked");
const regex locattion_rex("Location: (.*)");
/*
*/
void err_print(const char *msg);
string get_default(string str, int type);
smatch get_rex_matches(string url, regex rex);
bool send_req(int sckt, string url, string file_path, int cnt, string ver="1");
string get_cont(int sckt, string url, string file_path, string ver);
bool connect_to(smatch url_parts, int attempts);
bool save_response(string response, bool chunked);
string url_encode(const string input);
/*
*
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
    if ( connect_to(get_rex_matches(url,url_rex), ++attempts) ) {
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
/*
*
*/
bool connect_to(smatch url_parts, int attempts)
{   // [0]full_url [1]www [2]domain [4]port [5]path
    const string path = get_default(static_cast<string>(url_parts[5]), U_PATH);
    size_t pos = path.find_last_of("/");
    if (attempts == 1)
    {
        file_name = get_default(path.substr(pos+1), F_NAME );
    }
    if (attempts > MAX_ATTEMPTS) {
        err_print("Too many redirects");
        return EXIT_FAILURE;
    }
    const string port = get_default(static_cast<string>(url_parts[4]),U_PORT );
    const string url  = static_cast<string>(url_parts[1]) + static_cast<string>(url_parts[2]);
    
    bool send_err;
    int code, sckt;
    struct addrinfo set;
    struct addrinfo *results, *res;

    memset(&set, 0, sizeof(struct addrinfo));
    set.ai_socktype = SOCK_STREAM; /* TCP Socket */
    set.ai_family = AF_UNSPEC;     /* Allow IPv4 or IPv6 */
    set.ai_protocol = 0;           /* Any protocol */
    set.ai_flags = 0;

    if ((code = getaddrinfo( url.c_str(), port.c_str(), &set, &results) ) != 0) { 
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
    int s_att = 1;
    send_err = send_req(sckt, url, path, s_att); // send is closing socket
    return (send_err) ? EXIT_FAILURE : EXIT_SUCCESS;
}
/*
*
*/
bool send_req(int sckt, string url, string file_path, int cnt, string ver)
{
    string resp_msg = get_cont(sckt, url, url_encode(file_path), ver);
    close(sckt);
    if (resp_msg == "ErrInMyGet")
    {
        return EXIT_FAILURE;
    }
    smatch head;
    regex_search(resp_msg, head, head_rex);
    string res_ver = static_cast<string>(head[1]);
    string s_code = static_cast<string>(head[2]);
    int code = atoi(s_code.c_str());
    string err_msg = static_cast<string>(head[3]);
    bool chunked = regex_search(resp_msg, head, chunk_rex);

    if (code == 302 || code == 301) { 
        regex_search(resp_msg, head, locattion_rex);
        string url = static_cast<string>(head[1]);
        if (code == 301) {
            if ( known_urls.find(url) != string::npos ){
                attempts--;
            }
            else {
                known_urls.append(url, url.size());
            }
        }
        if (connect_to(get_rex_matches(url,url_rex), ++attempts) ) { 
            return EXIT_FAILURE;
        }
        else {
            return EXIT_SUCCESS;            
        }
    }
    if (code > 399 && code < 600) {
        err_print((s_code + " " + err_msg).c_str());
        return EXIT_FAILURE;
    }
    if (code == 200) {
        if ((cnt == 1) && (res_ver=="1")) {
            if (save_response(resp_msg, chunked)){
                return EXIT_FAILURE;
            }
        }
        if ((cnt == 1) && (res_ver == "0")) {
            if( send_req(sckt, url, file_path, ++cnt, "0") ){
                return EXIT_FAILURE;
            }
        }
        if ( (cnt == 2) && (res_ver == "0")) {
            if (save_response(resp_msg, chunked)) {
                return EXIT_FAILURE;
            }
        }
        if ((cnt == 2) && (res_ver != "0")) {
            err_print("Unexpected error");
            return EXIT_FAILURE;
        }
        return EXIT_SUCCESS;
    }
    else{
        err_print((s_code + " " + err_msg).c_str());
        return EXIT_FAILURE;
    }
}
/*
*
*/
string get_cont( int sckt, string url, string file_path, string ver){
    int res;
    char response[BUFF_SIZE];
    bzero(response, BUFF_SIZE);
    string resp_msg = "";
    string msg =    "GET " + file_path + " HTTP/1." + ver + "\r\n" + 
                    "Host: " + url + "\r\n" + "Connection: close\r\n\r\n";
    if ( send(sckt, msg.c_str(),msg.size(), 0) == -1 ) {
        err_print("Error while sending request");
        return "ErrInMyGet";
    }
    while ( (res = recv(sckt, response, BUFF_SIZE-1, 0 )) )
    {
        if (res < 0) {
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
*
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
        {
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
*
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
*
*/
string url_encode(string input)
{
    regex space(" ");
    regex tilda("~");
    string replacement[] = { "%20","%7E"};
    input = regex_replace (input,space,replacement[0]);
    input = regex_replace (input,tilda,replacement[1]);
    return input;
}
/*
*
*/
smatch get_rex_matches(string input, regex rex)
{
    smatch output;
    regex_match(input, output, rex);
    return output;
}
/*
*
*/
void err_print(const char *msg)
{
    cerr << msg << endl;    
}