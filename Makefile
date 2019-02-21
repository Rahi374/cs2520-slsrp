docker_all: docker run_docker

docker: Dockerfile
	docker build --tag=router .

network:
	docker network create --subnet=172.18.0.0/16 slsrpnet

run_docker:
	docker-compose up --scale router=5
	#docker run --net mynet123 --ip 172.18.0.22 -it ubuntu bash
