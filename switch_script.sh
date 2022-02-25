#!/bin/bash

#echo $1
#echo $2

if [ ! -z "$2" ]; then
	sudo systemctl disable --now openvpn@$2;
fi

if [ ! -z "$1" ]; then
	sudo systemctl start openvpn@$1;
fi

