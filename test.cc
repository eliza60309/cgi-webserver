#include <bits/stdc++.h>
#include <sys/types.h>
#include <sys/stat.h>

using namespace std;

string get_file_type(string file);

int main()
{
	string s;
	cin >> s;
	struct stat ss;
	cout << stat(s.c_str(), &ss) << endl;
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
