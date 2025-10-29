
CC = gcc
CFLAGS = -g -Wall -Wextra -std=gnu11
LDFLAGS = 

all: main_servidor main_cliente


TARGET_SERVER = main_servidor
SRCS_SERVER = main_servidor.c servidor_http.c
OBJS_SERVER = $(SRCS_SERVER:.c=.o)
HDRS_SERVER = servidor_http.h

$(TARGET_SERVER): $(OBJS_SERVER)
	@echo "Linkando o Servidor: $@"
	$(CC) $(LDFLAGS) -o $@ $(OBJS_SERVER)
	@echo "Programa '$@' compilado com sucesso!"

TARGET_CLIENT = main_cliente

SRCS_CLIENT = main_cliente.c cliente.c
OBJS_CLIENT = $(SRCS_CLIENT:.c=.o)
HDRS_CLIENT = cliente.h

$(TARGET_CLIENT): $(OBJS_CLIENT)
	@echo "Linkando o Cliente: $@"
	$(CC) $(LDFLAGS) -o $@ $(OBJS_CLIENT)
	@echo "Programa '$@' compilado com sucesso!"


main_servidor.o: main_servidor.c $(HDRS_SERVER)
	@echo "Compilando (Servidor): $<"
	$(CC) $(CFLAGS) -c $< -o $@

servidor_http.o: servidor_http.c $(HDRS_SERVER)
	@echo "Compilando (Servidor): $<"
	$(CC) $(CFLAGS) -c $< -o $@

main_cliente.o: main_cliente.c $(HDRS_CLIENT)
	@echo "Compilando (Cliente): $<"
	$(CC) $(CFLAGS) -c $< -o $@

cliente.o: cliente.c $(HDRS_CLIENT)
	@echo "Compilando (Cliente): $<"
	$(CC) $(CFLAGS) -c $< -o $@


clean:
	@echo "Limpando arquivos gerados..."
	rm -f $(OBJS_SERVER) $(OBJS_CLIENT) $(TARGET_SERVER) $(TARGET_CLIENT) *.o

.PHONY: all clean