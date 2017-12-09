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
#include <vector>
#include <map>
#include <fcntl.h>
#include <netinet/in.h>

#define F_CONNECTING 0
#define F_READING 1
#define F_WRITING 2
#define F_DONE 3

using namespace std;

string read_until(const char *arr, const char *terminal, int &cursor);
string nslookup(string s);
int getint(const char *c);
string readline(int sock);

class request
{
	public:
	string host;
	string port;
	string file;
	int state;
	int fd;
	string input;
	int cursor;
};

int main()
{
	cout << "Content-Type: text/html\r\n\r\n";
	string query = getenv("QUERY_STRING");
	//string query = "h1=nplinux3.cs.nctu.edu.tw&p1=9487&f1=t1.txt&h2=&p2=&f2=&h3=&p3=&f3=&h4=&p4=&f4=&h5=&p5=&f5=";
	map<string, string> args;
	cout << query << "<br>";
	int cursor = 0;
	while(1)
	{
		string name = read_until(query.c_str(), "=", cursor);
		if(name == "")break;
		string cont = read_until(query.c_str(), "&", cursor);
		args[name] = cont;
	}
	vector<request> request_vector;
	vector<string> number;
	number.push_back("1");
	number.push_back("2");
	number.push_back("3");
	number.push_back("4");
	number.push_back("5");
	for(int i = 0; i < number.size(); i++)
	{
		if(args["h" + number[i]].size() != 0 && args["p" + number[i]].size() != 0 && args["f" + number[i]].size() != 0)
		{
			request request_instance;
			request_instance.host = nslookup(args["h" + number[i]]);
			request_instance.file = args["f" + number[i]];
			request_instance.port = args["p" + number[i]];
			request_instance.state = 0;
			int sock = socket(PF_INET, SOCK_STREAM, 0);
			fstream in(request_instance.file.c_str());
			string input;
			input.assign(istreambuf_iterator<char>(in), istreambuf_iterator<char>());
			request_instance.input = input;
			request_instance.cursor = 0;
			request_instance.fd = sock;
			request_instance.state = F_CONNECTING;
			request_vector.push_back(request_instance);
			int flag = fcntl(sock, F_GETFL, 0);
			fcntl(sock, F_SETFL, flag | O_NONBLOCK);
		}
	}
	fd_set rfds;
	fd_set wfds;
	fd_set rs;
	fd_set ws;
	FD_ZERO(&rfds);
	FD_ZERO(&wfds);
	FD_ZERO(&rs);
	FD_ZERO(&ws);
	for(int i = 0; i < request_vector.size(); i++)
	{
		struct sockaddr_in cli_addr, serv_addr;
		bzero((char *) &serv_addr, sizeof(serv_addr));
		serv_addr.sin_family = AF_INET;
		serv_addr.sin_addr.s_addr = inet_addr(request_vector[i].host.c_str());
		serv_addr.sin_port = htons(getint(request_vector[i].port.c_str()));
		if(connect(request_vector[i].fd, (sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
		{
			if(errno != EINPROGRESS)
			{
				cout << "[ERR] connect error" << endl;
				return -1;
			}
		}
		FD_SET(request_vector[i].fd, &rs);
		FD_SET(request_vector[i].fd, &ws);
	}
	rfds = rs;
	wfds = ws;
	int f = false;
	int nfds = FD_SETSIZE;
	int connection_cnt = request_vector.size();
	while(connection_cnt > 0)
	{
		memcpy(&rfds, &rs, sizeof(rfds));
		memcpy(&wfds, &ws, sizeof(wfds));
		if(select(nfds, &rfds, &wfds, NULL, NULL) < 0)
		{
			if(errno == EINTR)continue;
			else 
			{
				cout << "[ERR] select error" << endl;
				return 0; 
			}
		}
		for(int i = 0; i < request_vector.size(); i++)
		{
			//cout << "STATE " << request_vector[i].state << FD_ISSET(request_vector[i].fd, &wfds) << FD_ISSET(request_vector[i].fd, &rfds) << endl;
			if(request_vector[i].state == F_CONNECTING)
			{
//				cout << "connecting...<br>" << endl;
				int error, k;
				if(getsockopt(request_vector[i].fd, SOL_SOCKET, SO_ERROR, &error, (unsigned int *)&k) < 0 || error != 0)
				{
					cout << "[ERR] socket non blocking error" << endl;
					return 0;
				}
				FD_CLR(request_vector[i].fd, &ws);
				request_vector[i].state = F_READING;
			}
			else if(request_vector[i].state == F_READING && FD_ISSET(request_vector[i].fd, &rfds))
			{
				//cout << "reading...<br>" << endl;
				char c[1024 * 1024] = {};
				int flag = true;
				int ans = read(request_vector[i].fd, c, 1024 * 1024);
				string inst = c;
				if(ans)
				{
					int cursor = 0;
					while(cursor != inst.size())
					{
						cout << read_until(inst.c_str(), "\r\n", cursor);
					//	cout << "inst:" << inst[inst.size() - 1] << inst[inst.size() - 2] << endl;
					}
				}
				if(inst[inst.size() - 1] == ' ' && inst[inst.size() - 2] == '%')
				{
					request_vector[i].state = F_WRITING;
					FD_CLR(request_vector[i].fd, &rs);
					FD_SET(request_vector[i].fd, &ws);
					if(f)
					{
						request_vector[i].state = F_DONE;
						connection_cnt--;
						FD_CLR(request_vector[i].fd, &ws);
						cout << "DONE";
					}
				}
				else cout << "<br>" << endl;

			}
			else if(request_vector[i].state == F_WRITING && FD_ISSET(request_vector[i].fd, &wfds))
			{
//				cout << "writing...<br>" << endl;
				string input = read_until(request_vector[i].input.c_str(), "\n", request_vector[i].cursor);
				input += '\n';
				cout << input << "<br>" << endl;
				int n = write(request_vector[i].fd, input.c_str(), input.size());
				FD_CLR(request_vector[i].fd, &ws);
				FD_SET(request_vector[i].fd, &rs);
				if(request_vector[i].cursor == request_vector[i].input.size())f = true;
				else request_vector[i].state = F_READING;
			}
		}
	}
	return 0;
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

int getint(const char *c)
{
	int num = 0;
	for(int i = 0; i < 20; i++)
	{
		if(c[i] == 0)return num;
		num *= 10;
		num += c[i] - '0';
	}
	return num;
}

string readline(int sock)
{
	int sum = 0;
	string s;
	while(1)
	{
		char c;
		int ret = read(sock, &c, 1);
		if(ret)
		{
			if(c == '\n')
			{
				s += c;
				return s;
			}
			else if(c == '\0')return s;
		}
		else if(ret == -1 && errno != EINTR)return "ERROR IN STRING";
	}
}
