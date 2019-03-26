// Oliver Vazquez
// CS 3377 Unix S19
// 24 March 2019

// Random Comment
#include <iostream>
#include <string>
#include <sstream>
#include <vector>
#include <cstdlib>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>

using namespace std;

/*
 * Function prototypes
 */
vector<string> readCommand(string &line);
string readSpecialChar(string &line);

/*
 * Main Function
 */
int main() {
	//Read from user one line at a time
	string line;
	bool loop = true;
	while (loop) {
		//Get input
		cout << "$ ";
		getline(cin, line);

		//Input descriptor that can change
		pid_t savedInput = 0;
		
		//Repeat until all commands have run
		bool commandLoop = true;
		while (commandLoop) {
			//Get the next command
			vector<string> command = readCommand(line);

			//Format it to char ** for exec function arguments
			char *commandName;
			char **execArgs = new char*[command.size()+1];
			for (int i=0; i<command.size(); i++) {
				execArgs[i] = (char*)command[i].c_str();
			}
			execArgs[command.size()] = nullptr;
			commandName = execArgs[0];

			//If the command name is "exit" we stop the program
			if (command[0] == "exit")
				return 0;

			/*
			 * Do special characters (like | ; < > etc)
			 */
			int pipes[2];
			int redirectDescriptor;
			bool isPiped = false;
			bool isRedirected = false;
			bool redirectIn = false;
			string specialChar = readSpecialChar(line);
			if (specialChar == "STOP") {
				//Stops the loop after the next command runs
				commandLoop = false;
			} else if (specialChar == "|") {
				//Opens a pipe for the next command
				isPiped = true;
				pipe(pipes);
			} else if (specialChar == "<" || specialChar == ">" || specialChar == ">>") {
				//Read the file redirect
				vector<string> file = readCommand(line);
				string fileName = file[0];
				isRedirected = true;
				
				//Open the descriptor for the file
				if (specialChar == "<") {
					redirectDescriptor = open(fileName.c_str(), O_RDONLY);
					redirectIn = true;
				} else if (specialChar == ">") {
					redirectDescriptor = open(fileName.c_str(), O_WRONLY | O_CREAT, 0777);
				} else {
					redirectDescriptor = open(fileName.c_str(), O_WRONLY | O_CREAT | O_APPEND, 0777);
				}
				if (redirectDescriptor < 0) {
					cout << "Error opening file!" << endl;
				}
			}

			//Fork the program to run the command
			pid_t forkPID = fork();
			if (forkPID == 0) {
				//If we are the child then do redirects and execute:
				//Get the current input source
				dup2(savedInput, 0);

				//Get pipe/file redirection sources
				if (isPiped == true) {
					//Set output to writing side of pipe
					dup2(pipes[1], 1);
				} else if (isRedirected == true) {
					//Set input/output based off redirectIn flag
					if (redirectIn == true) { 
						dup2(redirectDescriptor, 0);
					} else {
						dup2(redirectDescriptor, 1);
					}
				}

				//Execute the command
				execvp(commandName, execArgs);
			} else if (forkPID < 0) {
				//If the PID is less than 0 there is an error
				cout << "Error forking!" << endl;
			} else {
				//If we are the parent then just wait for the child to finish running
				waitpid(forkPID, nullptr, 0);
			}

			//If there was a piping, then update the input source to the reading side and close the writing side
			if (isPiped == true) {
				close(pipes[1]);
				savedInput = pipes[0];
			}

			//If there is a semicolon, refresh saved input source to standard input for the next command to run
			if (specialChar == ";") {
				savedInput = 0;
			}

			//If the string is empty, end the loop
			if (line.size() == 0) {
				commandLoop = false;
			}

			//Clean the arg array
			delete execArgs;
		}
	}
	return 0;
}

/*
 * Reads a command line into a vector of strings
 */
vector<string> readCommand(string &line) {
	//Find the command boundary
	int i;
	for (i = 0; i<line.size(); i++) {
		bool stop = false;
		switch (line[i]) {
		case ';':
		case '<':
		case '>':
		case '|':
		case '\0':
			i -= 1;
			stop = true;
			break;
		}
		if (stop) break;
	}

	//Get the command as a stream
	stringstream commandStream(line.substr(0, i));
	line.erase(0, i);

	//Read command parts into a vector
	string part;
	vector<string> command;
	while (commandStream >> part) {
		command.push_back(part);
	}

	//Return the vector
	return command;
}
/*
 * Reads a special symbol from the line
 */
string readSpecialChar(string &line) {
	//Find the special word
	int i;
	string word;
	for (i = 0; i<line.size(); i++) {
		if (line[i] == ';') {
			word = ";";
			break;
		} else if (line[i] == '<') {
			word = "<";
			break;
		} else if (line[i] == '>') {
			if (line[i+1] == '>') {
				word = ">>";
			} else {
				word = ">";
			}
			break;
		} else if (line[i] == '|') {
			word = "|";
			break;
		} else if (line[i] == '\0' || line[i] == '\n') {
			word = "STOP";
			break;
		} else if (!isspace(line[i])) {
			//Return if the next thing isnt a special character
			return "";
		}
	}
	if (i == 0) return "STOP";
	line.erase(0, i+1);
	return word;
}
