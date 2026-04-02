//
// Created by Yang on 2026/4/2.
//

#include "rom.h"
#include <stdio.h>
#include "../nes/interface.h"

size_t rom_get_size(const char* file)
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


int rom_read(const char* file, uint8_t* romfile)
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
