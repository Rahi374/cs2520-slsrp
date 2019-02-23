docker: Dockerfile src/router.*
	docker build --tag=router .

network:
	docker network create --subnet=172.18.0.0/16 slsrpnet
