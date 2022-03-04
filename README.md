# NORDVPN ACCOUNT CHECKER (BASED ON C++) #

## IMPORTANT NOTE ##
This program is for education purposes ONLY.
Author shall not be responsible for any illegal use of this program.
Use this only on accounts you own.



### DOWNLOAD ###
1. Clone this git directory.\
	Command: git clone "https://github.com/shahfahad50951/nordvpn-checker.git"


### COMPILE ###
1. Change current working directory to nordvpn-checker directory.\
	cd ./nordvpn-checker

2. Compile the program on you unix System.\
	g++ ./nordvpn-checker.cpp -o ./nordvpn-checker
	

### USE ###
1. Run the program.\
	./nordvpn-checker -i {inputfile_path} -o {outputfile_path}



### INFO ###
1. This program accepts as input a inputfile_path. File must be a unix text file and not a windows textfile (i.e it must not contain any carriage return characters)
2. All the entries in inputfile should be of the format: username:password and each seperate entry should be on a newline.
3. All the Successful Entries along with their Expiry Details will be appended to the file given by outputfile_path as well as written to stdout.
4. Make sure that nordvpn daemon i.e nordvpnd is already running.
5. THIS PROGRAM WORKS ONLY ON UNIX LIKE SYSTEMS
6. This branch uses linux pipes for Interprocess Communication instead of file based IPC, so this program should be faster.
7. This program does not require bash.


### PROGRAM CONFIGURATION ###
You can edit the nordvpn-checker.cpp file to change the following parameters defined as preprocessor directive, to your liking:
1. LIMIT: Number of Accounts to check in a single run. Program will quit after checking LIMIT number of accounts. Make sure to keep it low otherwise your ip address will be blocked by nordvpn servers.
2. __DEBUG: If defined, some debugging information related to program will be outputed to stderr.

Make sure to recompile nordvpn-checker.cpp after changing any preprocessor directive.
