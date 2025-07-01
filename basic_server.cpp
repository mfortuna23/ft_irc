// basic_server.cpp

#include <iostream>
#include <map>
#include <vector>
#include <string>
#include <cstring>
#include <cstdlib>
#include <unistd.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <poll.h>

#define BUFFER_SIZE 512

void set_nonblocking(int fd) {
    fcntl(fd, F_SETFL, O_NONBLOCK);
}

int send_all(int sockfd, const char *buf, size_t len)
{
	size_t total_sent = 0;

	while (total_sent < len)
	{
		int sent = send(sockfd, buf + total_sent, len - total_sent, 0);
        if (sent <= 0)
            return -1;
        total_sent += sent;
    }
	return 0;
}

int create_listener_socket(int port) {
    int sockfd = socket(PF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
        throw std::runtime_error("socket() failed");

    int yes = 1;
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));// allows to reuse the port without having to wait.

    struct sockaddr_in addr;
    std::memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);

    if (bind(sockfd, (sockaddr*)&addr, sizeof(addr)) < 0)
        throw std::runtime_error("bind() failed");
    if (listen(sockfd, SOMAXCONN) < 0)
        throw std::runtime_error("listen() failed");

    set_nonblocking(sockfd);
    return sockfd;
}

int main(int argc, char** argv) {
    if (argc != 3) {
        std::cerr << "Usage: " << argv[0] << " <port> <password>\n";
        return 1;
    }

    int port = std::atoi(argv[1]);
    std::string password = argv[2];

    int listen_fd;
    try {
        listen_fd = create_listener_socket(port);
    } catch (std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }

    std::vector<struct pollfd> fds;

    struct pollfd pfd; // estrutura para o fd servidor
    pfd.fd = listen_fd;
    pfd.events = POLLIN;
    fds.push_back(pfd);

    std::cout << "Server listening on port " << port << "...\n";

    while (true) {
        if (poll(&fds.at(0), fds.size(), -1) < 0) {
            std::cerr << "poll() failed\n";
            break;
        }

        for (size_t i = 0; i < fds.size(); ++i) {
            if ((fds[i].revents & POLLIN) == false) // operacao de comparacao bit a bit. "O bit POLLIN está ativado em revents?" → Se sim, resultado é não zero (true).
                continue;                   // se tem dados para ler (true) nao entra no continue, que avancaria para o proximo fd.
                                                      
            if (fds[i].fd == listen_fd) // se for o socket do servidor, um novo cliente esta querendo se conectar
            {
                // Novo cliente
                int client_fd = accept(listen_fd, NULL, NULL);
                if (client_fd >= 0) {
                    set_nonblocking(client_fd);
                    struct pollfd client_pfd; // criamos a estrutura para o fd cliente
                    client_pfd.fd = client_fd; // nomeamos
                    client_pfd.events = POLLIN; // tipo de evento que queremos esperar
                    fds.push_back(client_pfd); // colocamos no fim do nosso vetor de estruturas
                    std::cout << "New client connected: fd " << client_fd << "\n";
                }
            }
            else
            {
                // Cliente existente enviando dados
                int fd = fds[i].fd; // salvo o descritor do cliente
                char buffer[BUFFER_SIZE]; // crio o buffer temporario
                ssize_t bytes = recv(fd, buffer, BUFFER_SIZE - 1, 0); // armazeno o numero de bytes de facto lidos

                if (bytes <= 0) // verifico se o cliente se desconectou ou deu erro.
                {
                    std::cout << "Client disconnected: fd " << fd << "\n";
                    close(fd);
                    fds.erase(fds.begin() + i); // removo este client do vetor de fds
                    --i; // ajusto o indice ja que o vetor agora esta menor.
                    continue; // passo para a proxima iteracao do loop.
                }
                // se recebi os dados normalmente:
                buffer[bytes] = '\0'; // o buffer vira uma string valida.
                std::string msg(buffer); // crio um objeto do tipo string.
                std::cout << "[fd " << fd << "] " << msg; // escrevo a mensagem recebida.
                std::string response = ":server PONG :" + msg; // crio um outro objeto tipo string estilo protocolo irc.
                //send(fd, response.c_str(), response.size(), 0); // nao usar assim.
                send_all(fd, response.c_str(), response.size());  // o cliente recebe a confirmacao da mensagem que enviou.
            }
        }
    }

    close(listen_fd);
    return 0;
} 
