#include <windows.h>

#include "display.h"
#include "nes_cpu.h"
#include "nes_main.h"
#include "pcm.h"

int main()
{
#if !ENABLE_LOG
    StartDisplayWindow();
#endif
    // pcm_init(22050);

    // mapper0
    // nes_load("../rom/LanMAster.nes");
    nes_load("../rom/cjml.nes");
    // nes_load("../rom/tkdz.nes");
    // nes_load("../rom/dzk.nes");
    // nes_load("../rom/dkq2.nes");
    // nes_load("../rom/cdr.nes");
    // nes_load("../rom/zxd.nes");

    // mapper1
    // nes_load("../rom/tly.nes");

    // mapper2
    // nes_load("../rom/hdl.nes");

    // mapper3
    // nes_load("../rom/mxd.nes");

    // mapper4
    // nes_load("../rom/yjs.nes");

    // aduio测试
    // pcm_init(8000);
    // pcm_play_file("../rom/rssq.pcm");
    //
    // while (1)
    // {
    //     Sleep(100);
    // }
}


