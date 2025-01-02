#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <limits.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#define TEST_PATH "/proc/self"
#define BUFFER_SIZE 20
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

int main() {
    char buf[BUFFER_SIZE];
    ssize_t len;

    // 创建符号链接
    // if (symlink("./etc_passwd", TEST_PATH) != 0) {
    //     perror("symlink");
    //     exit(EXIT_FAILURE);
    // }

    printf("Symbolic link created: %s -> /etc_passwd\n", TEST_PATH);
    write_coordination_file("iago_attack_readlink");
    sleep(2);
    // 读取符号链接内容
    len = readlink(TEST_PATH, buf, BUFFER_SIZE - 1); // 留出一个位置用于添加终止符
    if (len == -1) {
        perror("readlink");
        // unlink(TEST_PATH);
    }
    printf("len = %d\n", len);
    // 检查返回值是否超出缓冲区限制
    if (len >= BUFFER_SIZE) {
        write_coordination_file("0");
        printf("attack successed\n");
        // unlink(TEST_PATH);  
        return 0;
    }

    // 手动添加零终止符
    buf[len] = '\0';

    printf("Symbolic link content: %s\n", buf);
    write_coordination_file("0");
    // 清理测试文件
    // unlink(TEST_PATH);

    return 0;
}
