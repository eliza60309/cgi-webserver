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

string read_until(const char *arr, const char *terminal, int &cursor);
string nslookup(string s);

int main()
{
	cout << "Content-Type: text/html\r\n\r\n";
	string query = getenv("QUERY_STRING");
	map<string, string> args;
	cout << query << "<br>" << endl;
	int cursor = 0;
	while(1)
	{
		string name = read_until(query.c_str(), "=", cursor);
		if(name == "")break;
		string cont = read_until(query.c_str(), "&", cursor);
		args[name] = cont;
	}
	for(auto i = args.begin(); i != args.end(); i++)
	{
		if((*i).first[0] == 'h')
		{
			string name = nslookup((*i).second);
			cout << (*i).first << " nslookup " << name << "<br>" << endl;
		}
		else cout << (*i).first << " " << (*i).second << "<br>" << endl;
		
	}
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
//		cout << "[LOG] nslookup status: " << WEXITSTATUS(wstat) << endl;
		char ret[1024];
		read(p[0], ret, 1024);
		close(p[0]);
		return string(ret);
	}
}

string read_until(const char *arr, const char *terminal, int &cursor)
{
	char str[1024] = {};
	for(int i = 0; i < 1024; i++)
	{
		if(arr[cursor] == '\0')return string(str);
		for(int j = 0;terminal[j] != '\0'; j++)
		{
			if(arr[cursor] == terminal[j])
			{
				cursor++;
				return string(str);
			}
		}
		str[i] = arr[cursor];
		cursor++;
	}
}
/*
string nslookup(string s)
{
	int p[0];
	pipe(p);
	if(!fork())
	{
		close(p[0]);
		dup2(p[1], 1);//1 for stdout
		if(!execl("nslookup.cgi", "nslookup.cgi", NULL))exit(0);
		else exit(1);
	}
	else
	{
		close(p[1]);
		wait(NULL);
		char ret[1024];
		read(p[0], ret, 1024);
		close
	}
}*/
