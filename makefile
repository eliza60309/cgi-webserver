all:
	g++ server.cc
	g++ hw3.cc -o root/hw3.cgi
	g++ nslookup.cc -o root/nslookup.cgi
server:
	g++ server.cc
cgi:
	g++ hw3.cc -o root/hw3.cgi
nslookup:
	g++ nslookup.cc -o root/nslookup.cgi
	
