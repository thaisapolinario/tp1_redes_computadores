# Trabalho Prático 1 de Redes de Computadores

1.  main_servidor: Um servidor web de arquivos.
2.  main_cliente: Um cliente web para baixar arquivos.

main_servidor:
    - Serve arquivos de um diretório-raiz especificado.

main_cliente:
    - Baixa um arquivo de uma URL completa
    -Analisa a URL para extrair o host, porta e caminho.


Como Compilar:
    Com 2 terminais aberto (1 para Cliente e outro para o Servidor)

1 - Terminal Servidor 
    make
    ./main_servidor meusite (irá iniciar o servidor)
    
2- Terminal Cliete
    ./main_cliente http://localhost:8888/ (Listagem)
    ./main_cliente http://localhost:8888/imagens/d.jpg (Erro 404)
    ./main_cliente http://localhost:8888/imagens/dog.jpg (download)
    
