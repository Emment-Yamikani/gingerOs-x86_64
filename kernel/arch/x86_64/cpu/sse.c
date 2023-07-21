#include <bits/errno.h>
#include <lib/printk.h>
#include <sync/spinlock.h>
#include <sys/thread.h>
#include <arch/cpu.h>
#include <arch/x86_64/system.h>

#define AVX512_BIT BS(16)

static size_t cpu_simd_region_size;
static void (*cpu_save_simd)(void *);
static void (*cpu_restore_simd)(void *);

int sse_init(void) {
    // printk("init SSE as this is the baseline for x86_64 long mode math\n");

    cr0mask(CR0_EM);
    cr0set(CR0_MP);

    if (cpu_has(CPU_FXSR))
        cr4set(CR4_OSFXRS);
    else
        panic("CPU has no OFXSR\n");

    if (cpu_has(CPU_SSE))
        cr4set(CR4_OSXMMEXCPT);

    if (cpu_has(CPU_XSAVE)) {
        cr4set(CR4_OSXSAVE);

        uint64_t xcr0 = BS(1) | BS(0);
        if (cpu_has(CPU_AVX))
            xcr0 |= BS(2);

        uint32_t a, b, c, d;

        cpuid(7, 0, &a, &b, &c, &d);

        if((b & AVX512_BIT)) {
                xcr0 |= (1 << 5); // Enable AVX-512
                xcr0 |= (1 << 6); // Enable management of ZMM{0 -> 15}
                xcr0 |= (1 << 7); // Enable management of ZMM{16 -> 31}
        }

        wrxcr(0, xcr0);

        cpu_save_simd = xsave;
        cpu_restore_simd = xrstor;
        panic("CPU support XSAVE\n");
    } else {
        cpu_simd_region_size = 512; // Legacy size for fxsave
        cpu_save_simd = fxsave;
        cpu_restore_simd = fxrstor;
    }

    return 0;
}

void coprocessor_except(void) {
    panic("CPU%d executed %s()\n", cpu_id, __func__);
}

void simd_fp_except(void) {
    panic("CPU%d executed %s()\n", cpu_id, __func__);
}