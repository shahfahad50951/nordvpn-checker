#!/bin/sh
TEMPFILE=".tempfile.txt";

#echo "X${0}X" ;
#echo "X${1}X" ;
#echo "X${2}X" ;

nordvpn logout > /dev/null;

nordvpn login --username $1 --password $2 > $TEMPFILE;

nordvpn account >> $TEMPFILE;
