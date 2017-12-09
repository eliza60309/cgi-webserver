#include <iostream>
#include <fcntl.h>
#include <fstream>
#include <string.h>
#include <unistd.h>
#include <string>
#include <sstream>
#include <vector>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#define F_CONNECTING 0
#define F_READING 1
#define F_WRITING 2
#define F_DONE 3
using namespace std;

string output;
struct data
{
int fd;
int status;
int mID;
string IP;
string port;
fstream file;
};

void change(string *linetmp)
{
for(int i = 0; i < linetmp->size(); ++i)
{
if((*linetmp)[i] == '<') linetmp->replace(i, 1, "&lt");
else if((*linetmp)[i] == '>') linetmp->replace(i, 1, "&gt");
else if((*linetmp)[i] == '"') linetmp->replace(i, 1, "&quot");
else if((*linetmp)[i] == '&') linetmp->replace(i, 1, "&amp");
else if((*linetmp)[i] == 'r') linetmp->erase(i--, 1);
else if((*linetmp)[i] == 'n') linetmp->erase(i--, 1);
}
}
void display(int i, string text)
{
if( output.empty() )
{
output += "<script>document.all['m";
output += to_string(i);
output += "'].innerHTML += \"";
}
output += text;
if(text != "% ")
{
output += "<br>\";</script>n";
cout << output << endl;
output = "";
}
}
int main(int argc, char* argv[], char* envp[])
{
cout << "Content-type: text/htmlnn";
// parse string
string query_string = getenv("QUERY_STRING");

stringstream parse(query_string);
vector<data> info(5);
string part, tmp, filename;
int index = 0;
while( getline(parse, part, '&') )
{
if(part[0] == 'h') info[index].IP = part.substr(3);
if(part[0] == 'p') info[index].port = part.substr(3);
if(part[0] == 'f')
{
filename = part.substr(3);
info[index].file.open(filename.c_str(), ios::in);
++index;
}
}
// initialize socket
int conn = 0;
struct sockaddr_in serv_addr[5];
for(int i = 0; i < 5; ++i)
{
if(info[i].IP == "" || info[i].port == "")
{
info[i].fd = -1;
info[i].mID = -1;
continue;
}

info[i].mID = conn++;
info[i].fd = socket(AF_INET, SOCK_STREAM, 0);
memset(&serv_addr[i], 0, sizeof(serv_addr[i]));
serv_addr[i].sin_family = AF_INET;
serv_addr[i].sin_addr = *( (struct in_addr *) gethostbyname( info[i].IP.c_str() )->h_addr );
serv_addr[i].sin_port = htons( atoi(info[i].port.c_str()) );
}


cout << "<html>" << endl
<< "<head>" << endl
<< "<meta http-equiv=\"Content-Type\" content=\"text/html; charset=big5\" />" << endl
<< "<title>Network Programming Homework 3</title>" << endl
<< "</head>" << endl
<< "<body bgcolor=#336699>" << endl
<< "<font face=\"Courier New\" size=2 color=#FFFF99>" << endl
<< "<table width=\"1000\" border=\"1\">" << endl
<< "<tr>";


for (int i = 0; i < 5; ++i)
if (info[i].mID == -1);
else cout << "<td valign=\"top\" id =\"m" << i << "\"></td>";

cout << "</tr></table>" << endl;


fd_set rfds;
fd_set rs;
FD_ZERO(&rfds);
FD_ZERO(&rs);
fd_set wfds;
fd_set ws;
FD_ZERO(&wfds);
FD_ZERO(&ws);
int nfds = FD_SETSIZE;


for (int i = 0; i < 5; i++)
{
if (info[i].fd != -1)
{

int flag;
if( fcntl(info[i].fd, F_GETFL, 0) == -1 ) flag = 0;

fcntl(info[i].fd, F_SETFL, flag | O_NONBLOCK);

connect(info[i].fd, (struct sockaddr *)&serv_addr[i], sizeof(serv_addr[i]));
info[i].status = F_CONNECTING;
FD_SET(info[i].fd, &rs);
FD_SET(info[i].fd, &ws);
}
}
rfds = rs;
wfds = ws;

while (conn > 0)
{
memcpy(&rfds, &rs, sizeof(rfds));
memcpy(&wfds, &ws, sizeof(wfds));
if ( select(nfds, &rfds, &wfds, (fd_set*)0, (struct timeval*)0) < 0 ) return(-1);

for (int i = 0; i < 5; i++)
{
if (info[i].fd == -1) continue;
else if(info[i].status == F_CONNECTING)
{
FD_CLR(info[i].fd, &ws);
info[i].status = F_READING;
}
else if( FD_ISSET(info[i].fd, &rfds) )
{
char buffer[5001];
memset(buffer, 0, 5001 * sizeof(char) );

if ( read(info[i].fd, buffer, 5000) > 0)
{
stringstream ss((string)buffer);
string linetmp;
while (getline(ss, linetmp, 'n'))
{
if(linetmp[0] == '%' && linetmp[1] == ' ') FD_SET(info[i].fd, &ws);
//                       else if(linetmp[0] == '*' && linetmp[1] == '*' && linetmp[2] == '*' && linetmp[3] != '*') FD_SET(info[i].fd, &ws);

change(&linetmp);
display(info[i].mID, linetmp);
}
if(info[i].status == F_DONE)
{
close(info[i].fd);
FD_CLR(info[i].fd, &rs);
FD_CLR(info[i].fd, &ws);
info[i].fd = -1;
--conn;
}
}
}
else if( FD_ISSET(info[i].fd, &wfds))
{
string buffer;
getline(info[i].file, buffer);

buffer += "n";
send(info[i].fd, buffer.c_str(), buffer.size(), 0);
buffer.pop_back();

change(&buffer);
string command = (string)"<b>" + buffer + (string)"</b>";
display(info[i].mID, command);

if (buffer == "exit") info[i].status = F_DONE;
FD_CLR(info[i].fd, &ws);
}
}
}
cout << "</font>"
<< "</body>"
<< "</html>" << endl;
return 0;
}
