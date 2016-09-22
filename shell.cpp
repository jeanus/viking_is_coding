#include<iostream>
#include<sys/types.h>
#include<sys/wait.h>
#include<fcntl.h>
#include<unistd.h>
#include<stdlib.h>
#include<cstdio>
#include<string>
#include<string.h>
#include<vector>
using namespace std;

vector<string> split(string str, const char * d) {
	char * writable = new char[str.size() + 1];
	std::copy(str.begin(), str.end(), writable);
	writable[str.size()] = '\0';
	vector<string> v;
	char *p;
	p = strtok(writable, d);
	while (p)
	{
		string s(p);
		v.push_back(s);
		p = strtok(NULL, d);
	}
	delete[] writable;
	return v;
}


bool start_with(const string &mainstr, const string &substr){
	return mainstr.find(substr) == 0 ? 1 : 0;
}

bool check_redirect(vector<string>& vec){
	bool find = false;
	int i = 0;
	for (vector<string>::iterator a = vec.begin(); a < vec.end(); a++, i++){
		string str = *a;
		if (str.compare(">") == 0){
			vec.erase(a);
			string filename = *a;
			vec.erase(a);
			a--;
			freopen(filename.c_str(), "w", stdout);
			find = true;
		}
		else if (str.compare("<") == 0){
			vec.erase(a);
			string filename = *a;
			vec.erase(a);
			a--;
			freopen(filename.c_str(), "r", stdin);
			find = true;
		}
		else if (str.compare(">>") == 0){
			vec.erase(a);
			string filename = *a;
			vec.erase(a);
			a--;
			freopen(filename.c_str(), "a", stdout);
			find = true;
		}
		else if (start_with(str, ">>")){
			string filename = (*a).substr(2);
			vec.erase(a);
			a--;
			freopen(filename.c_str(), "a", stdout);
			find = true;
		}
		else if (start_with(str, ">")){
			string filename = (*a).substr(1);
			vec.erase(a);
			a--;
			freopen(filename.c_str(), "w", stdout);
			find = true;
		}
		else if (start_with(str, "<")){
			string filename = (*a).substr(1);
			vec.erase(a);
			a--;
			freopen(filename.c_str(), "r", stdin);
			find = true;
		}
		else if (str.find(">>") != std::string::npos){
			int index = str.find(">>");
			string filename = (*a).substr(index + 2);
			vec[i] = (*a).substr(0, index);
			freopen(filename.c_str(), "a", stdout);
			find = true;
		}
		else if (str.find(">") != std::string::npos){
			int index = str.find(">");
			string filename = (*a).substr(index + 1);
			vec[i] = (*a).substr(0, index);
			freopen(filename.c_str(), "w", stdout);
			find = true;
		}
		else if (str.find("<") != std::string::npos){
			int index = str.find("<");
			string filename = (*a).substr(index + 1);
			vec[i] = (*a).substr(0, index);
			freopen(filename.c_str(), "r", stdin);
			find = true;
		}
	}
	return find;

}

static void run(vector<string> vec, int in, int out) {
	char** argv = new char*[vec.size() + 1];
	int i = 0;
	for (vector<string>::iterator a = vec.begin(); a<vec.end(); a++){
		argv[i] = (char*)((*a).data());
		i++;
	}
	argv[vec.size()] = NULL;

	dup2(in, 0);   /* <&in  : child reads from in */
	dup2(out, 1); /* >&out : child writes to out */
	execvp(argv[0], argv);
	perror("Unable to execute the command");
	exit(EXIT_FAILURE);
}

void run_pipeline(vector< vector<string> > command){
	int n = command.size();
	if(n==1){
		vector<string> vec = command[0];
		if (vec[0].compare("cd") == 0){
			if (vec.size()>1){
				if (vec[1].compare("~") == 0)
					chdir(getenv("HOME"));
				else
					chdir(vec[1].c_str());
			}
			return;
		}
		if (vec[0].compare("echo") == 0){
			for (vector<string>::iterator a = vec.begin() + 1; a < vec.end(); a++){
				cout << (*a) << " ";
			}
			cout << endl;
			return;
		}
		
	}
	



	int i, in = 0; /* the first command reads from stdin */
	for (i = 0; i < n; ++i) {
		int status;
		int fd[2]; /* in/out pipe ends */
		pid_t pid; /* child's pid */
		vector<string> vec = command[i];
		if (vec[0].compare("cd") != 0 ){
			pipe(fd);
			pid = fork();
			if (pid == 0) {
				/* run command[i] in the child process */
				if (i != 0)
					close(fd[0]); /* close unused read end of the pipe */
				if (i == (n - 1))
					run(command[i], in, 1); /* $ command < in */
				else
					run(command[i], in, fd[1]); /* $ command < in > fd[1] */
			}
			else { /* parent */			
				close(fd[1]); /* close unused write end of the pipe */
				if (i != 0)
					close(in);    /* close unused read end of the previous pipe */
				in = fd[0]; /* the next command reads from here */
			}
			wait(&status);
		}
		else{
			if (vec.size()>1){
				if (vec[1].compare("~") == 0)
					chdir(getenv("HOME"));
				else
					chdir(vec[1].c_str());
			}
		}
	}
}


int main(int argc, char* argv[]){
	int pid, status;
	cout << "$ ";
	string command;
	getline(cin, command);

	while (!cin.eof()){
		int oldin = dup(0);
		int oldout = dup(1);
		vector<string> vec = split(command, " ");;
		if (command.size() > 0){
			if (vec[0].compare("exit") == 0)
				break;
			bool find = check_redirect(vec);
			command = "";
			for (vector<string>::iterator b = vec.begin(); b < vec.end(); b++){
				command = command + " " + (*b);
			}
			vec = split(command, "|");
			vector< vector<string> > command_vec;
			for (vector<string>::iterator a = vec.begin(); a < vec.end(); a++){
				command_vec.push_back(split((*a), " "));
			}
			run_pipeline(command_vec);
		}
		dup2(oldin, 0);
		dup2(oldout, 1);
		cout << "$ ";
		getline(cin, command);
	}
	return 0;
}