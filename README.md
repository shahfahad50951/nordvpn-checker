# NORDVPN ACCOUNT CHECKER (BASED ON C++) #

## IMPORTANT NOTE ##
This program is for education purposes ONLY.
Author shall not be responsible for any illegal use of this program.
Use this only on accounts you own.



### DOWNLOAD ###
1. Clone this git directory.\
	Command: git clone "https://github.com/shahfahad50951/nordvpn-checker.git"



### PRECOMPILATION STEPS ###
1. First thing first, install openvpn package from your distribution, system must be booted with systemd.\
	This program will switch your openvpn server automatically when banned.
2. Download the openvpn config files (different file for different nordvpn servers) from the nordvpn website.  
3. Rename your openvpn config files downloaded from nordvpn website as 1.conf, 2.conf and so on.
4. Now place all these openvpn config files in the directory /etc/openvpn/
5. Now place the login credentials for nordvpn openvpn servers, downloaded from your nordvpn accounts, into files named auth1.txt, auth2.txt and so on.\
	There should be 2 lines in each authN.txt file. First line should contain the username and second line should contain password.
6. Now place all these files i.e auth1.txt, auth2.txt and so on, into the directory /etc/openvpn/.
7. Now edit each openvpn config file in the /etc/openvpn directory i.e 1.conf, 2.conf and so on, and search for a line containing "auth-user-pass".
8. Edit this line, and append the path of the file containing the nordvpn openvpn credentials, like "auth-user-pass /etc/openvpn/auth1.txt".
9. Repeat step 8 for all openvpn config files and add the path of file containing the credentials, in the line containing "auth-user-pass".


1. Edit the nordvpn-checker.cpp file and edit the preprocessor directive "#define TOTAL_SERVERS N", and set the value of N to the number of config files present in /etc/openvpn/ directory.


### COMPILE ###
1. Change current working directory to nordvpn-checker directory.\
	cd ./nordvpn-checker

2. Compile the program on you unix System.\
	g++ ./nordvpn-checker.cpp -o ./nordvpn-checker


### USE ###
1. Run the program.\
	./nordvpn-checker -i {inputfile_path} -o {outputfile_path} [-c N]


### INFO ###
1. This program accepts as input a inputfile_path. File must be a unix text file and not a windows textfile (i.e it must not contain any carriage return characters)
2. All the entries in inputfile should be of the format: username:password and each seperate entry should be on a newline.
3. All the Successful Entries along with their Expiry Details will be appended to the file given by outputfile_path as well as written to stdout.
4. It also accepts a -c flag which specifies the number of the current nordvpn server config file being used before nordvpn-checker program is run. By default it assumes that no nordvpn config file is being used.
5. Make sure that nordvpn daemon i.e nordvpnd is already running.
6. THIS PROGRAM WORKS ONLY ON UNIX LIKE SYSTEMS


### PROGRAM CONFIGURATION ###
You can edit the nordvpn-checker.cpp file to change the following parameters defined as preprocessor directive, to your liking:
1. LIMIT: Number of Accounts to check in a single run. Program will quit after checking LIMIT number of accounts. Make sure to keep it low otherwise your ip address will be blocked by nordvpn servers.
2. __DEBUG: If defined, some debugging information related to program will be outputed to stderr.

Make sure to recompile nordvpn-checker.cpp after changing any preprocessor directive.
