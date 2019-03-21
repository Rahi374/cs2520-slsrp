#ifndef _NAMING_H_
#define _NAMING_H_

#define NAME_SERVER_NAME "http://amanokami.net:5000/name/"
#define NAME_SERVER_ADD "http://amanokami.net:5000/add/"

//#define NAME_SERVER_NAME "http://127.0.0.1:5000/name/"
//#define NAME_SERVER_ADD "http://127.0.0.1:5000/add/"

size_t curlWriteFunction(void* ptr, size_t size, size_t nmemb, void* userdata);
int name_to_ip_port(char *name, char *ip_dest, int *port_dest);
void register_router(char *name, char *ip, int port);

#endif // _NAMING_H_
