#include "key.h"
#include <stdint.h>
#include <windows.h>

void key_read_state(uint8_t* pad0, uint8_t* pad1)
{
    *pad0 = 0;
    *pad1 = 0;

    if (GetAsyncKeyState('W') & 0x8000)
    {
        *pad0 |= 0x10; // UP
    }
    if (GetAsyncKeyState('S') & 0x8000)
    {
        *pad0 |= 0x20; // DOWN
    }
    if (GetAsyncKeyState('A') & 0x8000)
    {
        *pad0 |= 0x40; // LEFT
    }
    if (GetAsyncKeyState('D') & 0x8000)
    {
        *pad0 |= 0x80; // RIGHT
    }
    if (GetAsyncKeyState(VK_LEFT) & 0x8000)
    {
        *pad0 |= 0x01; // A
    }
    if (GetAsyncKeyState(VK_RIGHT) & 0x8000)
    {
        *pad0 |= 0x02; // B
    }
    if (GetAsyncKeyState(VK_SPACE) & 0x8000)
    {
        *pad0 |= 0x08; // Start
    }
    if (GetAsyncKeyState('H') & 0x8000)
    {
        *pad0 |= 0x04; // Select
    }
}
