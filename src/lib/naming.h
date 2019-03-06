#ifndef _NAMING_H
#define _NAMING_H

#define NAME_SERVER "http://127.0.0.1:5000/name/"

void name_to_ip_port(char *name, char *ip_dest, int *port_dest);
void register_router(char *name, char *ip, int port);

#endif // _NAMING_H
