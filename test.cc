#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <strings.h>
#include <string.h>
#include <fstream>
#include <iostream>
#include <map>

using namespace std;

string get_file_type(string file);
string nslookup(string s);

int main()
{
	chdir("root");	
	string s;
	cin >> s;
	cout << nslookup(s) << endl;
}

string nslookup(string s)
{
	int p[0];
	pipe(p);
	if(!fork())
	{
		close(p[0]);
		dup2(p[1], 1);//1 for stdout
		if(!execl("nslookup.cgi", "nslookup.cgi", s.c_str(), NULL))exit(0);
		else exit(1);
	}
	else
	{
		close(p[1]);
		int wstat;
		wait(&wstat);
		cout << "[LOG] nslookup status: " << WEXITSTATUS(wstat) << endl;
		char ret[1024];
		read(p[0], ret, 1024);
		close(p[0]);
		return string(ret);
	}
}

string get_file_type(string file)
{
	bool flag = false;
	for(int i = file.size() - 1; i >= 0; i--)
	{
		if(file[i] == '.')return string(file.c_str() + i + 1);
	}
	return string("");
}
