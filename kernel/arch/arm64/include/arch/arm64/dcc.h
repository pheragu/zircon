#pragma once

#include <arch/arm64.h>

/* DCC Status Bits */
#define DCC_STATUS_RX		(1 << 30)
#define DCC_STATUS_TX		(1 << 29)

static inline uint64_t __dcc_getstatus(void)
{
    return ARM64_READ_SYSREG(mdccsr_el0);
}

static inline char __dcc_getchar(void)
{
    char c = (char) ARM64_READ_SYSREG(dbgdtrrx_el0);
    return c;
}

static inline void __dcc_putchar(char c)
{
    ARM64_WRITE_SYSREG(dbgdtrtx_el0, (unsigned char) c);
}
