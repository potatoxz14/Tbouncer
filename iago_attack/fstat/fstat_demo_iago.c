#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdint.h>
#include <string.h>

// 模拟的文件结构体
struct cache_file {
    uint32_t nlibs;  // 假设有库的数量字段
    char libs[0];    // 假设是动态大小的库信息
};
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
// 宏定义用于验证指针合法性
#define VERIFY_PTR(ptr, cache_data_size) ((ptr) > (cache_data_size))

int main() {

    const char *filename = "testfile";
    int fd = open(filename, O_RDONLY);
    if (fd < 0) {
        perror("open");
        return EXIT_FAILURE;
    }

    struct stat st;
    write_coordination_file("iago_attack_fstat");
    sleep(5);
    if (fstat(fd, &st) < 0) {

        perror("fstat");
        close(fd);
        return EXIT_FAILURE;
    }
    write_coordination_file("0");
    size_t sizep = st.st_size; // 文件大小
    printf("File size reported by fstat: %zu\n", sizep);

    // 内存映射文件
    void *result = mmap(NULL, sizep, PROT_READ, MAP_PRIVATE, fd, 0);
    if (result == MAP_FAILED) {
        perror("mmap");
        close(fd);
        return EXIT_FAILURE;
    }

    close(fd);

    // 假设文件内容是特定格式，并进行强制类型转换
    struct cache_file *cache = (struct cache_file *)result;

    // 模拟计算 cache_data 和 cache_data_size
    void *cache_data = &cache->libs[cache->nlibs];
    uint32_t cache_data_size = (uint32_t)((const char *)cache + sizep - (const char *)cache_data);

    printf("cache_data_size: %u\n", cache_data_size);

    // 检查指针是否合法
    if (!VERIFY_PTR((uintptr_t)cache_data, cache_data_size)) {
        fprintf(stderr, "Pointer verification failed! Invalid access detected.\n");
        munmap(result, sizep);
        return EXIT_FAILURE;
    }
    if (cache_data_size > 50) {
        printf("attack success\n");
        return 0;
    }
    // 模拟对 cache_data 的使用
    printf("Accessing cache_data: %p\n", cache_data);
    munmap(result, sizep);

    return EXIT_SUCCESS;
}
