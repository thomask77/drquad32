#include "fault_handler.h"
#include "board.h"
#include "stm32f4xx.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/**
 * \todo Use memcpy in HardFault_Handler_C
 * \todo Support for FPU stack frames
 * \todo Interrupt handler for FPU errors
 * \todo FPU fault test command
 * \todo Safe info until next reboot
 */


// TODO: noinit doesn't work if there's a firmware upgrade
//
struct fault_info  fault_info  __attribute__ ((section (".noinit")));

/**
 * Hard fault handler function
 *
 * \see Joseph Yiu's Definitive Guide to the ARM Cortex-M3
 *
 */

__attribute__((used))
void HardFault_Handler_C(unsigned int *fault_args)
{
    fault_info.r0   = fault_args[0];
    fault_info.r1   = fault_args[1];
    fault_info.r2   = fault_args[2];
    fault_info.r3   = fault_args[3];
    fault_info.r12  = fault_args[4];
    fault_info.lr   = fault_args[5];
    fault_info.pc   = fault_args[6];
    fault_info.psr  = fault_args[7];
    fault_info.bfar = SCB->BFAR;
    fault_info.cfsr = SCB->CFSR;
    fault_info.hfsr = SCB->HFSR;
    fault_info.dfsr = SCB->DFSR;
    fault_info.afsr = SCB->AFSR;
    fault_info.magic = FAULT_INFO_MAGIC;

    board_set_leds(LED_RED);

    for(;;);
}


void HardFault_Handler(void)
{
    __asm volatile (
        "tst lr, #4             \n\t"
        "ite eq                 \n\t"
        "mrseq r0, msp          \n\t"
        "mrsne r0, psp          \n\t"
        "b HardFault_Handler_C  \n\t"
    );
}


void print_fault_info(void)
{
    uint32_t rcc_csr = RCC->CSR;

    printf("RCC->CSR: 0x%08lx%s%s%s%s%s%s%s\n", rcc_csr,
            rcc_csr & RCC_CSR_LPWRRSTF ? " LPWR" : "",
            rcc_csr & RCC_CSR_WWDGRSTF ? " WWDG" : "",
            rcc_csr & RCC_CSR_WDGRSTF  ? " WDG"  : "",
            rcc_csr & RCC_CSR_SFTRSTF  ? " SFT"  : "",
            rcc_csr & RCC_CSR_PORRSTF  ? " POR"  : "",
            rcc_csr & RCC_CSR_PADRSTF  ? " PAD"  : "",
            rcc_csr & RCC_CSR_BORRSTF  ? " BOR"  : ""
    );

    RCC_ClearFlag();

    if (fault_info.magic == FAULT_INFO_MAGIC) {
        printf(
            "R0  : %08lx  R1  : %08lx  R2  : %08lx  R3  : %08lx\n"
            "R12 : %08lx  LR  : %08lx  PC  : %08lx  PSR : %08lx\n"
            "BFAR: %08lx  CFSR: %08lx  HFSR: %08lx  DFSR: %08lx  AFSR: %08lx\n",
            fault_info.r0,   fault_info.r1,   fault_info.r2,   fault_info.r3,
            fault_info.r12,  fault_info.lr,   fault_info.pc,   fault_info.psr,
            fault_info.bfar, fault_info.cfsr, fault_info.hfsr, fault_info.dfsr,
            fault_info.afsr
        );
        fault_info.magic = 0;
    }
}

// -------------------- Shell commands --------------------
//
#include "command.h"
#include "FreeRTOS.h"
#include "task.h"

static void cmd_fault(int argc, char *argv[])
{
    if (argc != 2)
        goto usage;

    if (!strcmp(argv[1], "hard")) {
        // unaligned multiple load/store
        //
        __asm volatile (
            "movs r0, #1       \t\n"
            "ldm  r0, {r1-r2}  \t\n"
            "bx   lr           \t\n"
        );
        for(;;);
    }
    else if (!strcmp(argv[1], "wdog")) {
        // woof! arrr! woof!
        //
        __disable_irq();
        for(;;);
    }
    else if (!strcmp(argv[1], "stack")) {
        alloca(4096);
        taskYIELD();
        for(;;);
    }
    else if (!strcmp(argv[1], "assert")) {
        assert(23 == 42);
    }

usage:
    print_fault_info();
    printf("Usage: %s [hard|wdog|stack|assert]\n", argv[0]);
}

SHELL_CMD(fault, (cmdfunc_t)cmd_fault, "generate a fault")
