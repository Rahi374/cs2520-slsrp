#include <curl/curl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
/*
#define NAME_SERVER_NAME "http://amanokami.net:5000/name/"
#define NAME_SERVER_ADD "http://amanokami.net:5000/add/"
*/
#define NAME_SERVER_NAME "http://127.0.0.1:5000/name/"
#define NAME_SERVER_ADD "http://127.0.0.1:5000/add/"

size_t dataSize=0;
size_t curlWriteFunction(void* ptr, size_t size, size_t nmemb, void* userdata)
{
	char** stringToWrite=(char**)userdata;
	const char* input=(const char*)ptr;
	if(nmemb==0) return 0;
	if(!*stringToWrite)
		*stringToWrite=malloc(nmemb+1);
	else
		*stringToWrite=realloc(*stringToWrite, dataSize+nmemb+1);
	memcpy(*stringToWrite+dataSize, input, nmemb);
	dataSize+=nmemb;
	(*stringToWrite)[dataSize]='\0';
	return nmemb;
}

int name_to_ip_port(char *name, char *ip_dest, int *port_dest)
{
	char url[100];
	strcpy(url, NAME_SERVER_NAME);
	strcat(url, name);
	CURL *curl;
	CURLcode res;
	char *str = (char *)malloc(100);
	curl = curl_easy_init();
	if(curl) {
		curl_easy_setopt(curl, CURLOPT_URL, url);
		curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, &curlWriteFunction);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, &str);
		res = curl_easy_perform(curl);
		if(res != CURLE_OK)
			fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));

		curl_easy_cleanup(curl);
	}
	char *error_str = "\"0\"\n";
	if(strcmp(str, error_str)==0){
		free(str);
		return -1;
	}
	char *delim_p = strchr(str, ':');
	*delim_p = '\0';
	str++;//delete opening quote
	strcpy(ip_dest, str);
	delim_p++;
	*port_dest = atoi(delim_p);//this stips the closing quote
	free(--str);
	return 0;
}

void register_router(char *name, char *ip, int port)
{
	CURL *curl;
	CURLcode res;
	char url[100];
	char *str = (char *)malloc(100);
	strcpy(url, NAME_SERVER_ADD);
	strcat(url, name);
	strcat(url, "/");
	strcat(url, ip);
	strcat(url, "/");
	int len = snprintf(NULL, 0, "%d", port);
	char *port_str = malloc(len+1);
	snprintf(port_str, len+1, "%d", port);
	strcat(url, port_str);
	free(port_str);
	curl_global_init(CURL_GLOBAL_ALL);
	curl = curl_easy_init();
	if(curl){
		curl_easy_setopt(curl, CURLOPT_URL, url);
		curl_easy_setopt(curl, CURLOPT_POSTFIELDS, "");
		//curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, NULL);
		//curl_easy_setopt(curl, CURLOPT_WRITEDATA, &str);
		res = curl_easy_perform(curl);
		curl_easy_cleanup(curl);
	}
	curl_global_cleanup();
	free(str);
}

int main(int argc, char *argv[])
{	
	char *r_name = "r2";
	char *ip_addr = malloc(100);
	int port;
	register_router("r2", "172.18.0.3", 445);
	int res = name_to_ip_port(r_name, ip_addr, &port);
	if(res == 0){
		printf("ip addr final: %s\n", ip_addr);
		printf("port final: %d\n", port);
	}else{
		printf("router not found for that name\n");
	}
	return 0;
}
