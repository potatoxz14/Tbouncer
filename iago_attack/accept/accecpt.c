#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/mman.h>

#define MAX_FD 1024
#define PORT 8080
#define SHARED_FILE "/dev/shm/switch_file"

// 写入共享文件
void write_coordination_file(const char *data) {
    int fd = open(SHARED_FILE, O_CREAT | O_WRONLY | O_TRUNC, 0644);
    if (fd < 0) {
        perror("Failed to create file");
        return;
    }

    if (write(fd, data, strlen(data)) < 0) {
        perror("Failed to write to file");
    }

    close(fd);
}

// 服务器线程函数
void *server_thread(void *arg) {
    int fd_array[MAX_FD];
    int listen_fd = socket(AF_INET, SOCK_STREAM, 0); // 创建监听 socket
    if (listen_fd < 0) {
        perror("Socket creation failed");
        return NULL;
    }

    // 配置地址和端口
    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(PORT);
    if (bind(listen_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("Bind failed");
        close(listen_fd);
        return NULL;
    }

    if (listen(listen_fd, 5) < 0) {
        perror("Listen failed");
        close(listen_fd);
        return NULL;
    }

    printf("Server listening on port %d...\n", PORT);
    write_coordination_file("iago_attack_accept");

    while (1) {
        struct sockaddr_in client_addr;
        socklen_t addr_len = sizeof(client_addr);

        printf("Waiting for client...\n");
        int client_fd = accept(listen_fd, (struct sockaddr *)&client_addr, &addr_len);

        // if (client_fd < 0) {
        //     perror("Accept failed");
        //     continue;
        // }

        if (client_fd >= 0 && client_fd < MAX_FD) {
            printf("Client connected, fd=%d\n", client_fd);
            // 简单处理客户端连接
            char buffer[1024] = {0};
            recv(client_fd, buffer, sizeof(buffer), 0);
            printf("Received message: %s\n", buffer);

            // 关闭连接
            close(client_fd);
            write_coordination_file("0");
            return 0;
        } else {
            close(listen_fd);
            printf("Invalid client fd: %d\n", client_fd);
            write_coordination_file("0");
            return 0;
        }
    }

    close(listen_fd);
    return NULL;
}

// 客户端线程函数
void *client_thread(void *arg) {
    sleep(1); // 确保服务器先启动
    int sock;
    struct sockaddr_in server_addr;
    char *message = "Hello from client!";
    char buffer[1024] = {0};

    // 创建 socket
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("socket");
        return NULL;
    }

    // 配置服务器地址
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    if (inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr) <= 0) {
        perror("inet_pton");
        close(sock);
        return NULL;
    }

    // 连接服务器
    if (connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("connect");
        close(sock);
        return NULL;
    }

    printf("Connected to server.\n");

    // 发送消息
    send(sock, message, strlen(message), 0);
    printf("Message sent: %s\n", message);

    close(sock);
    return NULL;
}

// 主函数
int main() {
    pthread_t server_tid, client_tid;

    // 创建服务器线程
    if (pthread_create(&server_tid, NULL, server_thread, NULL) != 0) {
        perror("Failed to create server thread");
        exit(EXIT_FAILURE);
    }

    // 创建客户端线程
    if (pthread_create(&client_tid, NULL, client_thread, NULL) != 0) {
        perror("Failed to create client thread");
        exit(EXIT_FAILURE);
    }

    // 等待线程结束
    pthread_join(server_tid, NULL);
    pthread_join(client_tid, NULL);

    return 0;
}
