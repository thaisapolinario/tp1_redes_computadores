// meu_navegador.c
#include <stdio.h>
#include "cliente.h" 

int main(int argc, char *argv[]) {
    // Verifica se o argumento (URL) foi passado
    if (argc != 2) {

        return 1;
    }

    if (download(argv[1]) != 0) {
        fprintf(stderr, "Falha no download.\n");
        return 1;
    }

    return 0;
}