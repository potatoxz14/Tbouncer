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
#define ARRAY_SIZE 10

// 全局数组，模拟 Memcached 的 `conns`
char *conns[ARRAY_SIZE];

// 用于线程间通信的管道
int pipe_fds[2];
#define SHARED_FILE "/switch_file" // 注意：shm_open 的路径不需要 "/dev/shm"

// 写入共享文件
void write_coordination_file(const char *data) {
    // 使用 shm_open 创建或打开共享内存文件
    int fd = shm_open(SHARED_FILE, O_CREAT | O_RDWR, 0644);
    printf("write_coordination_file fd = %d\n", fd);
    if (fd < 0) {
        perror("Failed to open shared memory file");
        return;
    }

    // 调整共享内存文件大小，确保能够写入数据
    size_t data_len = strlen(data);
    if (ftruncate(fd, data_len) < 0) {
        perror("Failed to set size of shared memory file");
        close(fd);
        shm_unlink(SHARED_FILE);
        return;
    }

    // 映射共享内存
    char *shared_mem = mmap(NULL, data_len, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (shared_mem == MAP_FAILED) {
        perror("Failed to map shared memory");
        close(fd);
        shm_unlink(SHARED_FILE);
        return;
    }

    // 写入数据到共享内存
    memcpy(shared_mem, data, data_len);
    printf("Data written to shared memory: %s\n", shared_mem);

    // 解除映射和关闭文件描述符
    munmap(shared_mem, data_len);
    close(fd);
}


// 模拟 `conn_close_idle` 函数
void conn_close_idle(const char *conn) {
    if (conn) {
        printf("关闭连接: %s\n", conn);
    } else {
        printf("检测到非法内存访问！\n");
        exit(1);
    }
}

// 模拟“恶意操作系统”的线程，修改 `timeout_fd`
void *another_thread(void *arg) {
    sleep(1); // 确保主线程开始读取前运行
    int malicious_fd =  6; // 一个越界的数组下标
    write(pipe_fds[1], &malicious_fd, sizeof(malicious_fd));
    return NULL;
}

// 主函数，模拟传统应用的行为
int main() {
    pthread_t thread_id;

    // 初始化连接
    for (int i = 0; i < ARRAY_SIZE; i++) {
        asprintf(&conns[i], "连接 %d", i);
    }

    // 创建管道，用于模拟线程间通信
    if (pipe(pipe_fds) < 0) {
        perror("管道创建失败");
        return 1;
    }

    // 启动恶意线程
    pthread_create(&thread_id, NULL, another_thread, NULL);

    // 模拟从“可信”来源读取 `timeout_fd`
    int timeout_fd;
    printf("%d\n", pipe_fds[0]);
    write_coordination_file("iago_attack_read");
    sleep(3);
    ssize_t ret = read(pipe_fds[0], &timeout_fd, sizeof(timeout_fd));
    write_coordination_file("0");
    sleep(3);
    if (ret < 0) {
        perror("读取失败");
        return 1;
    }

    printf("读取到 timeout_fd: %d\n", timeout_fd);
    if (timeout_fd == 11) {
        printf("attack success\n");
    }
    else {
        printf("attack failed\n");
    }

    
    // 清理资源
    pthread_join(thread_id, NULL);
    for (int i = 0; i < ARRAY_SIZE; i++) {
        free(conns[i]);
    }
    close(pipe_fds[0]);
    close(pipe_fds[1]);

    return 0;
}
