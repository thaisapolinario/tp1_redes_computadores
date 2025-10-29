// servidor_http.c
#include "servidor_http.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/stat.h>   
#include <dirent.h>     // Para listar diretórios
#include <fcntl.h>      


void erro(int socket_cliente, const char *status, const char *mensagem) {
    dprintf(socket_cliente, "HTTP/1.1 %s\r\n", status);
    dprintf(socket_cliente, "Content-Type: text/html\r\n");
    dprintf(socket_cliente, "\r\n"); // Fim dos cabeçalhos
    dprintf(socket_cliente, "<html><body><h1>%s</h1><p>%s</p></body></html>", status, mensagem);
}

const char* obter_tipo_mime(const char *caminho_arquivo) {
    if (strstr(caminho_arquivo, ".html")) return "text/html";
    if (strstr(caminho_arquivo, ".css")) return "text/css";
    if (strstr(caminho_arquivo, ".js")) return "application/javascript";
    if (strstr(caminho_arquivo, ".jpg")) return "image/jpeg";
    if (strstr(caminho_arquivo, ".jpeg")) return "image/jpeg";
    if (strstr(caminho_arquivo, ".png")) return "image/png";
    if (strstr(caminho_arquivo, ".txt")) return "text/plain";
    return "application/octet-stream"; 
}

void enviar_arquivo(int socket_cliente, const char *caminho_arquivo) {
    FILE *arquivo = fopen(caminho_arquivo, "rb"); 
    if (arquivo == NULL) {
        erro(socket_cliente, "403 Forbidden", "Nao foi possivel ler o arquivo.");
        return;
    }

    // Tamanho do arquivo
    struct stat info_arquivo; 
    stat(caminho_arquivo, &info_arquivo);
    long tamanho_arquivo = info_arquivo.st_size;

    // Cabeçalhos HTTP
    dprintf(socket_cliente, "HTTP/1.1 200 OK\r\n");
    dprintf(socket_cliente, "Content-Type: %s\r\n", obter_tipo_mime(caminho_arquivo));
    dprintf(socket_cliente, "Content-Length: %ld\r\n", tamanho_arquivo);
    dprintf(socket_cliente, "\r\n"); 

    // Enviar em blocos
    char buffer_arquivo[TAMANHO_BUFFER];
    size_t bytes_lidos;
    while ((bytes_lidos = fread(buffer_arquivo, 1, TAMANHO_BUFFER, arquivo)) > 0) {
        if (write(socket_cliente, buffer_arquivo, bytes_lidos) < 0) {
            perror("write (enviar_arquivo)");
            break; 
        }
    }
    fclose(arquivo);
    printf("Arquivo enviado: %s\n", caminho_arquivo);
}

void enviar_listagem_diretorio(int socket_cliente, const char *caminho_diretorio, const char *caminho_uri) {
    DIR *diretorio = opendir(caminho_diretorio);
    if (diretorio == NULL) {
        erro(socket_cliente, "500 Internal Server Error", "Nao foi possivel ler o diretorio.");
        return;
    }

    // Enviar Cabeçalhos
    dprintf(socket_cliente, "HTTP/1.1 200 OK\r\n");
    dprintf(socket_cliente, "Content-Type: text/html\r\n");
    dprintf(socket_cliente, "\r\n"); 

    // Enviar Corpo (HTML)
    dprintf(socket_cliente, "<html><head><title>Index of %s</title></head><body>", caminho_uri);
    dprintf(socket_cliente, "<h1>Index of %s</h1><hr><ul>", caminho_uri);

    struct dirent *entrada_diretorio;
    while ((entrada_diretorio = readdir(diretorio)) != NULL) {
        if (strcmp(entrada_diretorio->d_name, ".") == 0) continue; // Ignora .

        char caminho_link[PATH_MAX];
        if (caminho_uri[strlen(caminho_uri) - 1] == '/') {
            snprintf(caminho_link, sizeof(caminho_link), "%s%s", caminho_uri, entrada_diretorio->d_name);
        } else {
            snprintf(caminho_link, sizeof(caminho_link), "%s/%s", caminho_uri, entrada_diretorio->d_name);
        }

        dprintf(socket_cliente, "<li><a href=\"%s\">%s</a></li>", caminho_link, entrada_diretorio->d_name);
    }
    dprintf(socket_cliente, "</ul><hr></body></html>");

    closedir(diretorio);
    printf("Listagem de diretorio enviada: %s\n", caminho_diretorio);
}

static void tratar_conexao(int socket_cliente) {
    char buffer_leitura[TAMANHO_BUFFER] = {0};
    char caminho_uri[PATH_MAX] = {0}; // O caminho da URL

    // Leitura requisição
    if (read(socket_cliente, buffer_leitura, TAMANHO_BUFFER - 1) < 0) {
        perror("read");
        close(socket_cliente);
        return;
    }
    
    // Verifica a requisição 
    if (sscanf(buffer_leitura, "GET %s HTTP", caminho_uri) != 1) {
        erro(socket_cliente, "400 Bad Request", "Requisicao mal formatada.");
        close(socket_cliente);
        return;
    }

    printf("Requisicao recebida para: %s\n", caminho_uri);

    char caminho_no_sistema[PATH_MAX];
    snprintf(caminho_no_sistema, sizeof(caminho_no_sistema), "%s%s", diretorio_raiz, caminho_uri);


    char caminho_resolvido[PATH_MAX];
    if (realpath(caminho_no_sistema, caminho_resolvido) == NULL) {
        erro(socket_cliente, "404 Not Found", "O recurso solicitado nao foi encontrado.");
        close(socket_cliente);
        return;
    }

    if (strncmp(caminho_resolvido, diretorio_raiz, strlen(diretorio_raiz)) != 0) {
        erro(socket_cliente, "403 Forbidden", "Acesso negado.");
        close(socket_cliente);
        return;
    }

    // Verificar se é um arquivo ou diretório
    struct stat info_arquivo;
    if (stat(caminho_resolvido, &info_arquivo) < 0) {
        erro(socket_cliente, "404 Not Found", "Nao foi possivel acessar o recurso.");
        close(socket_cliente);
        return;
    }

    if (S_ISDIR(info_arquivo.st_mode)) {
        // Diretorio 
        char caminho_index[PATH_MAX];
        snprintf(caminho_index, sizeof(caminho_index), "%s/index.html", caminho_resolvido);
        
        if (access(caminho_index, R_OK) == 0) {
            enviar_arquivo(socket_cliente, caminho_index);
        } else {
            enviar_listagem_diretorio(socket_cliente, caminho_resolvido, caminho_uri);
        }
    } 
    else if (S_ISREG(info_arquivo.st_mode)) {
        // Arquivo
        enviar_arquivo(socket_cliente, caminho_resolvido);
    } 
    else {
        erro(socket_cliente, "403 Forbidden", "Tipo de arquivo nao suportado.");
    }

    close(socket_cliente);
}


int criar_socket_servidor(int porta) {
    int descritor_socket_servidor; 
    struct sockaddr_in endereco;
    int opcao = 1;

    // 1. Criar Socket
    if ((descritor_socket_servidor = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket falhou");
        return -1;
    }
    if (setsockopt(descritor_socket_servidor, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opcao, sizeof(opcao))) {
        perror("setsockopt");
        close(descritor_socket_servidor);
        return -1;
    }
    endereco.sin_family = AF_INET;
    endereco.sin_addr.s_addr = INADDR_ANY; 
    endereco.sin_port = htons(porta); 
    
    if (bind(descritor_socket_servidor, (struct sockaddr *)&endereco, sizeof(endereco)) < 0) {
        perror("bind falhou");
        close(descritor_socket_servidor);
        return -1;
    }
    if (listen(descritor_socket_servidor, 10) < 0) { 
        perror("listen");
        close(descritor_socket_servidor);
        return -1;
    }
    return descritor_socket_servidor;
}

void loop_principal_servidor(int descritor_socket_servidor) {
    int novo_socket_cliente;
    struct sockaddr_in endereco_cliente;
    int tamanho_endereco = sizeof(endereco_cliente);

    while(1) {
        printf("\nAguardando nova conexao...\n");

        if ((novo_socket_cliente = accept(descritor_socket_servidor, (struct sockaddr *)&endereco_cliente, (socklen_t*)&tamanho_endereco)) < 0) {
            perror("accept");
            continue; 
        }

        printf("Conexao aceita.\n");
        tratar_conexao(novo_socket_cliente);
    }
}