#include <windows.h>

#include "display.h"
#include "nes_cpu.h"
#include "nes_main.h"
#include "pcm.h"
#include "aa.h"
#include "nes_apu.h"

int main()
{
#if !ENABLE_LOG
    StartDisplayWindow();
#endif
    pcm_init(APU_SAMPLE_RATE / 2);

    // === CPU基础测试 ===
    // nes_load("../test_rom/nestest/nestest.nes");

    // === instr_test-v5 ===
    // 1. 测试所有指令包括非法指令
    // nes_load("../test_rom/instr_test-v5/all_instrs.nes");
    // 2. 只测试官方指令
    // nes_load("../test_rom/instr_test-v5/official_only.nes");

    // === 所有官方/非官方指令的周期计数测试 ===
    // nes_load("../test_rom/cpu_timing_test6/cpu_timing_test.nes");

    // === 所有指令的时序测试 ===
    // nes_load("../test_rom/instr_timing/instr_timing.nes");
    // nes_load("../test_rom/instr_timing/rom_singles/1-instr_timing.nes");
    // nes_load("../test_rom/instr_timing/rom_singles/2-branch_timing.nes");

    // === APU基础测试 ===
    // nes_load("../test_rom/apu_test/apu_test.nes");
    // 1. 长度计数器测试
    // nes_load("../test_rom/apu_test/rom_singles/1-len_ctr.nes");
    // 2. 长度计数器查找表测试
    // nes_load("../test_rom/apu_test/rom_singles/2-len_table.nes");
    // 3. IRQ 标志测试
    // nes_load("../test_rom/apu_test/rom_singles/3-irq_flag.nes");
    // 4. 抖动测试 (jitter)
    // nes_load("../test_rom/apu_test/rom_singles/4-jitter.nes");
    // 5. 长度计数器时序测试
    // nes_load("../test_rom/apu_test/rom_singles/5-len_timing.nes");
    // 6. IRQ 标志时序测试
    // nes_load("../test_rom/apu_test/rom_singles/6-irq_flag_timing.nes");
    // 7. DMC 基础测试
    // nes_load("../test_rom/apu_test/rom_singles/7-dmc_basics.nes");
    // 8. DMC 速率测试
    // nes_load("../test_rom/apu_test/rom_singles/8-dmc_rates.nes");

    // mapper0
    // nes_load("../rom/LanMaster.nes");
    nes_load("../rom/cjml.nes");
    // nes_load("../rom/tkdz.nes");
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

    pcm_cleanup();
}


