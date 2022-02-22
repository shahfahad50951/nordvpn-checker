#!/bin/sh
TEMPFILE=".tempfile.txt";

#echo "X${0}X" ;
#echo "X${1}X" ;
#echo "X${2}X" ;

nordvpn logout > /dev/null;

echo $1:$2  >> logfile;

nordvpn login --username $1 --password $2 | tee $TEMPFILE >> logfile;

nordvpn account >> $TEMPFILE;
