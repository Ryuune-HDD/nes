//
// Created by Yang on 2025/12/4.
//

#include "interface.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "nes_cpu.h"

// 获取文件大小（单位：字节）
size_t nes_get_size(const char* file)
{
    if (!file)
    {
        LOG("file not found\n");
        return 0;
    }

    FILE* fp = fopen(file, "rb");
    if (!fp)
    {
        LOG("file not open\n");
        return 0;
    }

    if (fseek(fp, 0, SEEK_END) != 0)
    {
        LOG("file not seek end\n");
        fclose(fp);
        return 0;
    }

    size_t size = ftell(fp);
    fclose(fp);
    LOG("File size: %d\n", size);
    return size;
}

// 从文件读取 ROM 数据到指定缓冲区 romfile
// 调用者需确保 romfile 有足够空间（至少 nes_get_size(file) 字节）
int nes_read_rom(const char* file, uint8_t* romfile)
{
    if (!file || !romfile)
    {
        return -1;
    }

    FILE* fp = fopen(file, "rb");
    if (!fp)
    {
        return -2; // 打开失败
    }

    if (fseek(fp, 0, SEEK_SET) != 0)
    {
        fclose(fp);
        return -3;
    }

    // 获取文件大小
    if (fseek(fp, 0, SEEK_END) != 0)
    {
        fclose(fp);
        return -4;
    }
    long size = ftell(fp);
    if (size <= 0)
    {
        fclose(fp);
        return -5;
    }

    if (fseek(fp, 0, SEEK_SET) != 0)
    {
        fclose(fp);
        return -6;
    }

    size_t bytes_read = fread(romfile, 1, (size_t)size, fp);
    fclose(fp);

    if (bytes_read != (size_t)size)
    {
        return -7; // 读取不完整
    }

    LOG("File bytes_read: %d\n", bytes_read);
    return bytes_read; // 成功
}
