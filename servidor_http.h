#ifndef SERVIDOR_HTTP_H
#define SERVIDOR_HTTP_H

#include <limits.h> 

#define TAMANHO_BUFFER 4096
#define PORT 8888


extern char diretorio_raiz[PATH_MAX];


int criar_socket_servidor(int porta);
void loop_principal_servidor(int descritor_socket_servidor);

#endif 