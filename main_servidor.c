// meu_servidor.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>     
#include <sys/stat.h>   
#include "servidor_http.h" 

#define PORTA_PADRAO 8888

char diretorio_raiz[PATH_MAX];


int main(int argc, char *argv[]) {

    if (argc != 2) {

        return 1;
    }

    struct stat info_diretorio;

    if (stat(argv[1], &info_diretorio) != 0 || !S_ISDIR(info_diretorio.st_mode)) {
        fprintf(stderr, "Erro: Diretório '%s' não encontrado ou não é um diretório.\n", argv[1]);
        return 1;
    }


    if (realpath(argv[1], diretorio_raiz) == NULL) {
        perror("Erro ao resolver o caminho do diretorio");
        return 1;
    }

    size_t len = strlen(diretorio_raiz);
    if (len > 1 && diretorio_raiz[len - 1] == '/') {
        diretorio_raiz[len - 1] = '\0';
    }


    int descritor_socket_servidor = criar_socket_servidor(PORTA_PADRAO);
    if (descritor_socket_servidor < 0) {
        fprintf(stderr, "Falha fatal ao criar o socket do servidor.\n");
        return 1;
    }

    printf("Servidor HTTP iniciado em http://localhost:%d/\n", PORTA_PADRAO);
    printf("Diretório: %s\n", diretorio_raiz);


    loop_principal_servidor(descritor_socket_servidor);

    return 0;
}