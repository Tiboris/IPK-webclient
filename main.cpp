#include <cstdlib>

#include <iostream>
#include <string>

#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

using namespace std;

const string FILE_NAME="index.html";
const int REQ_ARGC=2;
const int MAX_ATTEMPTS=5;
//const string "^http:\/\/(www\.)?(.+?(\:\d+)?\/)(.*)$"



void err_print(string msg, int err_code);
bool valid_arg(int argc, char const *argv[]);

int main(int argc, char const *argv[])
{
	if ( ! valid_arg(argc,argv) )
	{
		err_print("",9);
		return 1;
	}
	return 0;
}

void err_print(string msg, int err_code)
{
	cerr << msg << err_code << endl;	
}

bool valid_arg(int argc, char const *argv[])
{
	if ((argc == REQ_ARGC) && (argv[1])==0)
		return true;
	else
		return false;
}