#!/bin/bash

if [ -z ${1+x} ]; then
	docker-compose up --scale router=1
else
	docker-compose up --scale router=$1
fi
