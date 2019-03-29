docker: Dockerfile src/router.*
	docker build --tag=router .
	make -C ./src ui

debug: Dockerfile src/router.*
	docker build -f Dockerfile.debug --tag=router .
	make -C ./src ui

network:
	docker network create --subnet=172.18.0.0/16 cs2520slsrp_default
