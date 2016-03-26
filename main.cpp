#include <cstdlib>

#include <iostream>
#include <string>

#include <regex>

#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

using namespace std;

const string FILE_NAME="index.html";
const int REQ_ARGC=2;
const int MAX_ATTEMPTS=5;	
const regex rex("^http://(www.)?(.+?)(:d+)?(/.*)$"); // tento regex treba TUTO opravit zerie aj :
//     tento ide ^http:\/\/(www\.)?([^\:]+?)(\:\d+)?(\/.*)$



void err_print(string msg, int err_code);
bool valid_arg(int count, char const *url[]);

int main(int argc, char const *argv[])
{	
	if ( ! valid_arg(argc,argv) )
	{
		err_print("Wrong parameters err ",9);
		return 1;
	}
	smatch matches;
	string url = argv[1];
	regex_match (url, matches, rex);
	cout << "string object with " << matches.size() << " matches\n";
	cout << "the matches were: " << endl;
	for (unsigned i=0; i<matches.size(); ++i) 
	{
		std::cout << "[" << matches[i] << "] "<< endl;
	}
	return 0;
}

void err_print(string msg, int err_code)
{
	cerr << msg << err_code << endl;	
}

bool valid_arg(int count, char const *url[])
{
	return ((count == REQ_ARGC) && (regex_match (url[1],rex))) ? true : false;
}