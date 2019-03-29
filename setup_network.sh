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
	echo "starting router $1 in docker with config $2"
	echo "docker run --network=cs2520slsrp_default router ./router $1 $2 &"
	docker run --network=cs2520slsrp_default router ./router $1 $2 &
}

setup_docker() {
	while read line; do
		if [[ -n $(echo $line | grep -P "\[.*\]") ]]; then
			router=$(echo $line | sed -e 's/\(\[\|\]\)//g')
			if [[ -n "$1" ]]; then
				start_docker_image $router configs/config
			else
				start_docker_image $router configs/$router.config
			fi
		fi
	done < $config_fn
}


if [[ -z "$1" ]]; then
	echo "specify config file, and optionally a second arg to spawn dockers,"
	echo "and a third to use same config for all docker containers"
	exit 1
fi
config_fn=$1

if [[ -n "$2" ]]; then
	if [[ -n "$3" ]]; then
		# start docker containers with global config
		setup_docker 1
	else
		setup_docker
	fi
	sleep 30
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
