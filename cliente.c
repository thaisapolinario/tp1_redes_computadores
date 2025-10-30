#include "cliente.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#define PORTA_PADRAO 80
#define BUFFER 4096

// pega a URL e divide em partes lógicas
static int verifica_url(const char *url_completa, char *buffer_host, size_t tamanho_buffer_host,char *buffer_caminho, size_t tamanho_buffer_caminho, int *porta) {
    char armaza_host[256];
    char caminho_temp[512];
    
    if (strncmp(url_completa, "http://", 7) != 0) {
        fprintf(stderr, "Erro: Apenas o protocolo HTTP é suportado.\n");
        return -1;
    }
    if (sscanf(url_completa, "http://%255[^/]%511[^\n]", armaza_host, caminho_temp) == 2) {
        strncpy(buffer_caminho, caminho_temp, tamanho_buffer_caminho - 1);
        buffer_caminho[tamanho_buffer_caminho - 1] = '\0';
    
    } else if (sscanf(url_completa, "http://%255[^\n]", armaza_host) == 1) {
        strcpy(buffer_caminho, "/"); 
    
    } else {
        fprintf(stderr, "URL mal formatada.\n");
        return -1;
    }

    // separar host e porta
    char *porta_str = strrchr(armaza_host, ':');
    
    if (porta_str != NULL) {
        int tamanho_host = porta_str - armaza_host;
        
        if (tamanho_host >= tamanho_buffer_host) {
             fprintf(stderr, "Erro: Nome do host (em URL) muito longo.\n");
             return -1;
        }
        strncpy(buffer_host, armaza_host, tamanho_host);
        buffer_host[tamanho_host] = '\0';
        
        *porta = atoi(porta_str + 1); 
    
    } else {
        if (strlen(armaza_host) >= tamanho_buffer_host) {
             fprintf(stderr, "Erro: Nome do host (em URL) muito longo.\n");
             return -1;
        }
        strcpy(buffer_host, armaza_host); // A armaza_host inteira é o host
        *porta = PORTA_PADRAO;  // Usa a porta padrão 8888
    }

    if (*porta <= 0 || *porta > 65535) {
         fprintf(stderr, "Erro: Número de porta inválido (%d).\n", *porta);
         return -1;
    }

    return 0; 
}

// Extrai o nome que deverá ser salvo no disco
static const char* extrai_nome(const char *caminho_recurso) {
    const char *nome_arquivo = strrchr(caminho_recurso, '/');
    if (nome_arquivo == NULL || *(nome_arquivo + 1) == '\0') {
        return "index.html"; 
    }
    return nome_arquivo + 1; 
}

// salva e baixa 
int download(const char *url_completa) {
    char nome_host[256];
    char caminho_recurso[512];
    int porta;
    
    int descreve_socket; 
    struct hostent *info_servidor; // resultado do DNS
    struct sockaddr_in endereco_servidor; 

    if (verifica_url(url_completa, nome_host, sizeof(nome_host), caminho_recurso, sizeof(caminho_recurso), &porta) != 0) {
        return -1;
    }
    printf("Conectando a: %s (Porta: %d, Caminho: %s)\n", nome_host, porta, caminho_recurso);

    // Cria Socket
    descreve_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (descreve_socket < 0) {
        perror("Erro ao criar socket");
        return -1;
    }

    info_servidor = gethostbyname(nome_host);
    if (info_servidor == NULL) {
        fprintf(stderr, "Erro: Host não encontrado (%s)\n", nome_host);
        close(descreve_socket);
        return -1;
    }

    memset(&endereco_servidor, 0, sizeof(endereco_servidor));
    endereco_servidor.sin_family = AF_INET;
    endereco_servidor.sin_port = htons(porta); 
    memcpy(&endereco_servidor.sin_addr.s_addr, info_servidor->h_addr_list[0], info_servidor->h_length);

    if (connect(descreve_socket, (struct sockaddr *)&endereco_servidor, sizeof(endereco_servidor)) < 0) {
        perror("Erro ao conectar");
        close(descreve_socket);
        return -1;
    }
    printf("Conectado!\n");

    // Envia a requisição HTTP
    char requisicao_http[1024];
    int tamanho_requisicao = snprintf(requisicao_http, sizeof(requisicao_http), 
                           "GET %s HTTP/1.1\r\nHost: %s\r\nConnection: close\r\n\r\n", 
                           caminho_recurso, nome_host);
                           
    if (send(descreve_socket, requisicao_http, tamanho_requisicao, 0) < 0) {
        perror("Erro ao enviar requisição");
        close(descreve_socket);
        return -1;
    }

    const char *nome_arquivo_saida = extrai_nome(caminho_recurso);
    FILE *arquivo_saida = fopen(nome_arquivo_saida, "wb"); 
    if (arquivo_saida == NULL) {
        perror("Erro ao abrir arquivo para escrita");
        close(descreve_socket);
        return -1;
    }

    char buffer_leitura[BUFFER];
    int bytes_recebidos;
    int cabecalho_processado = 0;
    long codigo_status = 0;

    printf("Recebendo arquivo... ");
    fflush(stdout); 

    while ((bytes_recebidos = recv(descreve_socket, buffer_leitura, BUFFER, 0)) > 0) {
        
        if (!cabecalho_processado) {
            
            sscanf(buffer_leitura, "HTTP/1.1 %ld", &codigo_status);

            char *fim_cabecalho = strstr(buffer_leitura, "\r\n\r\n");
            
            if (fim_cabecalho != NULL) {
                cabecalho_processado = 1;
                
                if (codigo_status != 200) {
                    fprintf(stderr, "\nErro: Servidor respondeu com código %ld\n", codigo_status);
                    fclose(arquivo_saida);
                    remove(nome_arquivo_saida); 
                    close(descreve_socket);
                    return -1;
                }
                
                size_t tamanho_cabecalho = (fim_cabecalho + 4) - buffer_leitura;
                size_t tamanho_corpo = bytes_recebidos - tamanho_cabecalho;
                
                if (tamanho_corpo > 0) {
                    fwrite(fim_cabecalho + 4, 1, tamanho_corpo, arquivo_saida);
                }
            }
        } else {
            fwrite(buffer_leitura, 1, bytes_recebidos, arquivo_saida);
        }
    }

    if (bytes_recebidos < 0) {
        perror("\nErro de recebimento de dados");
        remove(nome_arquivo_saida); 
    } else {
        printf("Salvo com sucesso: '%s'\n", nome_arquivo_saida);
    }

    fclose(arquivo_saida);
    close(descreve_socket);
    
    if (codigo_status != 200) {
        fprintf(stderr, "Erro: Arquivo não encontrado ou resposta inválida (Status: %ld)\n", codigo_status);
        return -1;
    }

    return 0; 
}