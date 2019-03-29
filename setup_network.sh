#!/bin/bash

ui=./src/ui
log_path=./logs
config_fn=""

ui_add_neighbour() {
	$ui $1 << EOF
1
$2
exit
EOF
}

start_docker_image() {
	echo "starting router $1 in docker"
	echo "docker run --network=cs2520slsrp_default router ./router $1 > $log_path/$1 &"
	docker run --network=cs2520slsrp_default router valgrind ./router $1 &
}

setup_docker() {
	while read line; do
		if [[ -n $(echo $line | grep -P "\[.*\]") ]]; then
			router=$(echo $line | sed -e 's/\(\[\|\]\)//g')
			start_docker_image $router
		fi
	done < $config_fn
}


if [[ -z "$1" ]]; then
	echo "specify config file, and optionally a second arg to spawn dockers"
	exit 1
fi
config_fn=$1

if [[ -n "$2" ]]; then
	setup_docker
	sleep 10
fi

echo "reading config from $config_fn"
section=""

while read line; do
	if [[ -n $(echo $line | grep -P "\[.*\]") ]]; then
		section=$(echo $line | sed -e 's/\(\[\|\]\)//g')
	elif [[ -n $line ]]; then
		echo -e "$section - $line" 
		ui_add_neighbour $section $line
	fi
done < $config_fn
