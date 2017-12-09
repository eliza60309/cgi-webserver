#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <signal.h>
#include <arpa/inet.h>
#include <map>
#include <unistd.h>
#include <strings.h>
#include <string.h>
#include <fstream>
#include <iostream>
#include <netdb.h>
//#include "shell.cc"

#define SERV_TCP_PORT 9487

using namespace std;

string get_file_type(string file);
string read_until(char *arr, const char *terminal, int &cursor);

int childdead = false;
void waitfor(int sig)
{
	int wstat;
	int pid = wait(&wstat);
	childdead = true;
}


int main()
{
	signal(SIGCHLD, waitfor);
	int serv_tcp_port = SERV_TCP_PORT;
	struct sockaddr_in cli_addr, serv_addr;
	int sock = socket(PF_INET, SOCK_STREAM, 0);
	if(sock < 0)
	{
		cout << "[ERR] Cannot open socket" << endl;
		return 0;
	}
	bzero((char *) &serv_addr, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	while(1)
	{
		serv_addr.sin_port = htons(serv_tcp_port);
		int bin = bind(sock, (struct sockaddr *) &serv_addr, sizeof(serv_addr));
		if(bin >= 0)break;
		serv_tcp_port++;
	}
	cout << "[LOG] Bind port: " << serv_tcp_port << endl;
	listen(sock, 5);
	while(1)
	{
		unsigned int clilen = sizeof(cli_addr);
		char hbuf[1024], sbuf[1024];
		int newsock = accept(sock, (struct sockaddr *) &cli_addr, &clilen);
		getnameinfo((struct sockaddr *)&cli_addr, clilen, hbuf, sizeof(hbuf), sbuf, sizeof(sbuf), NI_NUMERICHOST | NI_NUMERICSERV);
		cout << "[LOG] Connection accepted: " << hbuf << "/" << sbuf << endl;
		int child;
		if((child = fork()) < 0)
		{
			cout << "[ERR] Fork error" << endl;
			return 0;
		}
		else if(child == 0) 
		{
			signal(SIGCHLD, SIG_DFL);
			close(sock);
			fd_set fds;
			FD_ZERO(&fds);
			FD_SET(newsock, &fds);
			while(1)
			{
				while((select(newsock + 1, &fds, 0, 0, 0) < 0))
				{
					if(errno == EINTR);
					else 
					{
						cout << "[ERR] Select error" << endl;
						return 0;
					}
				}
				char packet[1024 * 1024] = {};
				int cursor = 0;
				map<string, string>env;
				int receive = read(newsock, packet, 1024 * 1024);
				cout << "[LOG] Received packet:" << receive << endl;
				string tok = read_until(packet, " ", cursor);
				env["REQUEST_METHOD"] = tok;
				tok = read_until(packet, "? ", cursor);
				env["SCRIPT_NAME"] = tok;
				env["HTTP_CONNECTION"] = "keep-alive";
				env["REQUEST_SCHEME"] = "http";
				env["AUTH_TYPE"] = "I'm the boss!!";
				env["REMOTE_USER"] = "??";
				env["REMOTE_IDENT"] = "??";
				if(packet[cursor - 1] == '?')
				{
					tok = read_until(packet, " ", cursor);
					env["QUERY_STRING"] = tok;
				}
				else if(packet[cursor - 1] == ' ')
				{
					env["QUERY_STRING"] = "";
				}
				tok = read_until(packet, "\r\n", cursor);
				env["SERVER_PROTOCOL"] = tok;
				env["CONTENT_LENGTH"] = "0";
				env["SERVER_SOFTWARE"] = "fucker";
				while(packet[cursor] == '\r' || packet[cursor] == '\n')cursor++;
				getnameinfo((struct sockaddr *)&cli_addr, clilen, hbuf, sizeof(hbuf), sbuf, sizeof(sbuf), NI_NUMERICHOST | NI_NUMERICSERV);
				env["REMOTE_ADDR"] = hbuf;
				env["REMOTE_PORT"] = sbuf;
				getnameinfo((struct sockaddr *)&serv_addr, sizeof(serv_addr), hbuf, sizeof(hbuf), sbuf, sizeof(sbuf), NI_NUMERICHOST | NI_NUMERICSERV);
				env["SERVER_ADDR"] = hbuf;
				env["SERVER_PORT"] = sbuf;
				env["REDIRECT_STATUS"] = "200";
				env["GATEWAY_INTERFACE"] = "CGI/1.1";
				map<string, string>field;
				chdir("root");
				while(1)
				{
					string f = read_until(packet, ":\r\n", cursor);
					if(f == "")break;
					cursor++;
					string v = read_until(packet, "\r\n", cursor);
					if(packet[cursor] == '\n')cursor++;
					field[f] = v;
				}
				env["SERVER_NAME"] = env["HTTP_HOST"] = field["Host"];
				if(field["Cookie"] != "")env["HTTP_COOKIE"] = field["Cookie"];
				if(env["QUERY_STRING"] == "")env["REQUEST_URI"] = env["SCRIPT_NAME"];
				else env["REQUEST_URI"] = env["SCRIPT_NAME"] + '?' + env["QUERY_STRING"];
				if(field["User-Agent"] != "")env["HTTP_USER_AGENT"] = field["User-Agent"];
				if(field["Accept"] != "")env["HTTP_ACCEPT"] = field["Accept"];
				if(field["Accept-Encoding"] != "")env["HTTP_ACCEPT_ENCODING"] = field["Accept-Encoding"];
				if(field["Accept-Language"] != "")env["HTTP_ACCEPT_LANGUAGE"] = field["Accept-Language"];
				if(field["Upgrade-Insecure-Requests"] != "")env["HTTP_UPGRADE_INSECURE_REQUESTS"] = field["Upgrade-Insecure-Requests"];
				int p[2];
				pipe(p);
				string file = "";
				for(int i = 0; i + 1 < env["SCRIPT_NAME"].size(); i++)file += env["SCRIPT_NAME"][i + 1];
				char pwd[1024];
				getcwd(pwd, 1024);
				env["SCRIPT_FILENAME"] = string(pwd) + '/' + file;
				env["DOCUMENT_ROOT"] = string(pwd);
				int exist = true;
				struct stat st;
				string response;
				if(stat(file.c_str(), &st) != 0)
				{
					cout << "[LOG] 404 FUCK OFF" << endl;
					exist = false;
					response += "HTTP/1.1 404 FUCK OFF\r\n";
					response += "Server: fucker\r\n";
					response += "Date: now\r\n";
					response += "Connection: keep-alive\r\n";
					response += "Keep-Alive: timeout=20\r\n";
					response += "Content-type: text/html\r\n\r\n<img src=\"http://skyhertz.me/404NF.JPG\">";
					write(newsock, response.c_str(), response.size());
				}
				else 
				{
					cout << "[LOG] 200 FUCK YEAH" << endl;
					response += "HTTP/1.1 200 FUCK YEAH\r\n";
					response += "Server: fucker\r\n";
					response += "Date: now\r\n";
					response += "Connection: keep-alive\r\n";
					response += "Keep-Alive: timeout=20\r\n";
					string type = get_file_type(file);
					if(type == "cgi")
					{
						childdead = false;
						signal(SIGCHLD, waitfor);
						if(!fork())
						{
						//	clearenv();
							for(auto iter = env.begin();; iter++)
							{
								if(iter == env.end())break;
								setenv((*iter).first.c_str(), (*iter).second.c_str(), 1);
							}
							close(p[0]);
							dup2(p[1], 1);
							execl(file.c_str(), file.c_str(), NULL);
							exit(0);
						}
						else 
						{
							close(p[1]);
							//wait(NULL);
							write(newsock, response.c_str(), response.size());
							cout << "[LOG] Sent packet: " << response.size() << endl;
							while(!childdead)
							{
								char ret[1024 * 1024] = {};
								int size = read(p[0], ret, 1024 * 1024);
								string s = ret;
								write(newsock, s.c_str(), s.size());
								cout << "[LOG] Sent packet: " << s.size() << endl;
							}
							close(p[0]);
						}
					}
					else
					{
						response += "Content-type: text/html\r\n\r\n";
						fstream fin(file.c_str());
						string input;
						input.assign(istreambuf_iterator<char>(fin), istreambuf_iterator<char>());
						response += input;
						write(newsock, response.c_str(), response.size());
						cout << "[LOG] Sent packet: " << response.size() << endl;

					}
				}
				//string s;
				//s += packet_header;
				//if(exist)s += ret;
				//s += "\r\n";
				//for(int i = 0; i < 5; i++)write(newsock, s.c_str(), s.size());
				//cout << "[LOG] Sent packet: " << s.size() << endl;

				close(newsock);
				return 0;
			}
		//	close(newsock);
			exit(0);
		}
		close(newsock);
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

string read_until(char *arr, const char *terminal, int &cursor)
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
