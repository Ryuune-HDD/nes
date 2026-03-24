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

    // nes_load("../test_rom/nestest/nestest.nes");

    // === instr_misc ===
    // nes_load("../test_rom/instr_misc/instr_misc.nes");
    // nes_load("../test_rom/instr_misc/rom_singles/01-abs_x_wrap.nes");
    // nes_load("../test_rom/instr_misc/rom_singles/02-branch_wrap.nes");
    // nes_load("../test_rom/instr_misc/rom_singles/03-dummy_reads.nes");
    // nes_load("../test_rom/instr_misc/rom_singles/04-dummy_reads_apu.nes");

    // === instr_test-v5 ===
    // nes_load("../test_rom/instr_test-v5/all_instrs.nes");
    // nes_load("../test_rom/instr_test-v5/official_only.nes");
    // nes_load("../test_rom/instr_test-v5/rom_singles/01-basics.nes");
    // nes_load("../test_rom/instr_test-v5/rom_singles/02-implied.nes");
    // nes_load("../test_rom/instr_test-v5/rom_singles/03-immediate.nes");
    // nes_load("../test_rom/instr_test-v5/rom_singles/04-zero_page.nes");
    // nes_load("../test_rom/instr_test-v5/rom_singles/05-zp_xy.nes");
    // nes_load("../test_rom/instr_test-v5/rom_singles/06-absolute.nes");
    // nes_load("../test_rom/instr_test-v5/rom_singles/07-abs_xy.nes");
    // nes_load("../test_rom/instr_test-v5/rom_singles/08-ind_x.nes");
    // nes_load("../test_rom/instr_test-v5/rom_singles/09-ind_y.nes");
    // nes_load("../test_rom/instr_test-v5/rom_singles/10-branches.nes");
    // nes_load("../test_rom/instr_test-v5/rom_singles/11-stack.nes");
    // nes_load("../test_rom/instr_test-v5/rom_singles/12-jmp_jsr.nes");
    // nes_load("../test_rom/instr_test-v5/rom_singles/13-rts.nes");
    // nes_load("../test_rom/instr_test-v5/rom_singles/14-rti.nes");
    // nes_load("../test_rom/instr_test-v5/rom_singles/15-brk.nes");
    // nes_load("../test_rom/instr_test-v5/rom_singles/16-special.nes");

    // === nes_instr_test ===
    // nes_load("../test_rom/nes_instr_test/rom_singles/01-implied.nes");
    // nes_load("../test_rom/nes_instr_test/rom_singles/02-immediate.nes");
    // nes_load("../test_rom/nes_instr_test/rom_singles/03-zero_page.nes");
    // nes_load("../test_rom/nes_instr_test/rom_singles/04-zp_xy.nes");
    // nes_load("../test_rom/nes_instr_test/rom_singles/05-absolute.nes");
    // nes_load("../test_rom/nes_instr_test/rom_singles/06-abs_xy.nes");
    // nes_load("../test_rom/nes_instr_test/rom_singles/07-ind_x.nes");
    // nes_load("../test_rom/nes_instr_test/rom_singles/08-ind_y.nes");
    // nes_load("../test_rom/nes_instr_test/rom_singles/09-branches.nes");
    // nes_load("../test_rom/nes_instr_test/rom_singles/10-stack.nes");
    // nes_load("../test_rom/nes_instr_test/rom_singles/11-special.nes");

    // mapper0
    nes_load("../rom/LanMAster.nes");
    // nes_load("../rom/cjml.nes");
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
    // nes_load("../rom/elsfk.nes");

    // // mapper4
    // nes_load("../rom/yjs.nes");
    //
    // // aduio测试
    // // pcm_init(8000);
    // // pcm_play_file("../rom/rssq.pcm");
    // //
    // // while (1)
    // // {
    // //     Sleep(100);
    // // }
}


