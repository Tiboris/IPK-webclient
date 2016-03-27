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
const regex rex("^http://(www\\.)?([\\w\\.]+)(\\:(\\d+))?(\\/.*)$");

void err_print(string msg);
bool create_message(smatch url_part);

int main(int argc, char const *argv[])
{	
	if ( ! ( (argc == REQ_ARGC) && ( regex_match(argv[1], rex) ) ) )
	{
		err_print("Wrong parameters");
		return 0;
	}
	smatch matches;
	string url = argv[1];
	regex_match(url, matches, rex);
	if ( ! create_message( matches ) ) 
	{
		return 0;
	}
	else
	{	
		return 1;
	}
}

bool create_message(smatch url_part)
{// [0]full_url [1]www [2]domain [4]port [5]path
	cout << "string object with " << url_part.size() << " matches\n";
	cout << "the matches were: " << endl;
	for (unsigned int i = 0; i < url_part.size(); ++i)
	{
		std::cout << i <<"[" << url_part[i] << "] "<< endl;
	}
	return true;
}

void err_print(string msg)
{
	cerr << msg << endl;	
}
