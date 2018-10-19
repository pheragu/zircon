#include <string.h>

#include <arch/arm64/dcc.h>
#include <kernel/mp.h>
#include <kernel/spinlock.h>
#include <kernel/thread.h>
#include <lib/cbuf.h>
#include <pdev/driver.h>
#include <pdev/uart.h>
#include <pow2.h>
#include <zircon/boot/driver-config.h>

#define RXBUF_SIZE 128
#define TXBUF_SIZE 1024

static spin_lock_t dcc_lock;
static thread_t dcc_do_work;
static cbuf_t rx_buf;
static cbuf_t tx_buf;
static bool initialized = false;

static size_t cbuf_space_used(cbuf_t* cbuf) {
    return modpow2((uint)(cbuf->head - cbuf->tail), cbuf->len_pow2);
}

static int dcc_pputc(char c) {
    while (__dcc_getstatus() & DCC_STATUS_TX)
        __asm__ volatile("yield" ::
                             : "memory");

    __dcc_putchar(c);

    return 1;
}

static int dcc_pgetc(void) {
    if (__dcc_getstatus() & DCC_STATUS_RX) {
        return __dcc_getchar();
    }

    return -1;
}

static int dcc_work(void* arg) {
    int c;
    char ch;
    spin_lock_saved_state_t irqflags;

    for (;;) {
        spin_lock_irqsave(&dcc_lock, irqflags);

        // write out any data in the buffer
        while (cbuf_read_char(&tx_buf, &ch, false)) {
            dcc_pputc(ch);
        }

        // see if any data is available while we're at it
        while (cbuf_space_avail(&rx_buf)) {
            c = dcc_pgetc();
            if (c == -1)
                break;
            cbuf_write_char(&rx_buf, (char)c);
        }

        spin_unlock_irqrestore(&dcc_lock, irqflags);
    }
    return 0;
}

//static int dcc_putc(char c)
static void dcc_puts(const char* str, size_t len, bool block, bool map_NL) {

    int cpu;
    bool copied_CR = false;
    spin_lock_saved_state_t irqflags;

    // make sure CPU doesn't change during this
    arch_interrupt_save(&irqflags, SPIN_LOCK_FLAG_INTERRUPTS);

    // if not CPU 0, cannot print until CPU 0 finishes initializing
    cpu = arch_curr_cpu_num();
    if (!initialized && cpu) {
        arch_interrupt_restore(irqflags, SPIN_LOCK_FLAG_INTERRUPTS);
        return;
    }

    arch_interrupt_restore(irqflags, SPIN_LOCK_FLAG_INTERRUPTS);

    // make sure cbuf, threads, and spinlock are initialized before using them
    if (initialized) {
        spin_lock_irqsave(&dcc_lock, irqflags);
        if (arch_curr_cpu_num() || cbuf_space_used(&tx_buf)) { // if not CPU 0 or there's data in the buffer
            while (len > 0) {
                if (*str == '\n' && map_NL && !copied_CR) {
                    copied_CR = true;
                    cbuf_write_char(&tx_buf, '\r'); // write char to be written by CPU 0
                } else {
                    copied_CR = false;
                    cbuf_write_char(&tx_buf, *str++); // write char to be written by CPU 0
                    len--;
                }
            }
            spin_unlock_irqrestore(&dcc_lock, irqflags);

            return;
        }
        while (len > 0) {
            if (*str == '\n' && map_NL && !copied_CR) {
                copied_CR = true;
                dcc_pputc('\r'); // write char to be written by CPU 0
            } else {
                copied_CR = false;
                dcc_pputc(*str++);
                len--;
            }
        }
        spin_unlock_irqrestore(&dcc_lock, irqflags);
    } else {
        while (len > 0) {
            if (*str == '\n' && map_NL && !copied_CR) {
                copied_CR = true;
                dcc_pputc('\r'); // write char to be written by CPU 0
            } else {
                copied_CR = false;
                dcc_pputc(*str++);
                len--;
            }
        }
    }
}

static int dcc_getc(bool wait) {
    size_t len;
    int c;
    int cpu;
    char ch;
    spin_lock_saved_state_t irqflags;

    // make sure CPU doesn't change
    arch_interrupt_save(&irqflags, SPIN_LOCK_FLAG_INTERRUPTS);

    // if not CPU 0, cannot print until CPU 0 finishes initializing
    cpu = arch_curr_cpu_num();
    if (!initialized && cpu) {
        arch_interrupt_restore(irqflags, SPIN_LOCK_FLAG_INTERRUPTS);
        return -1;
    }

    arch_interrupt_restore(irqflags, SPIN_LOCK_FLAG_INTERRUPTS);

    do {
        if (initialized) {
            spin_lock_irqsave(&dcc_lock, irqflags);

            if (arch_curr_cpu_num() || cbuf_space_used(&rx_buf)) { // if not CPU 0 or there's data in the buffer
                len = cbuf_read_char(&rx_buf, &ch, false);         // read char from CPU 0 DCC RX buffer

                if (len) {
                    spin_unlock_irqrestore(&dcc_lock, irqflags);
                    return (int)ch;
                }
            }

            c = dcc_pgetc();
            spin_unlock_irqrestore(&dcc_lock, irqflags);
            if (c != -1) {
                return c;
            }
        } else {
            c = dcc_pgetc();
            if (c != -1) {
                return c;
            }
        }
    } while (wait);
    return -1;
}

static const struct pdev_uart_ops dcc_ops = {
    .dputs = dcc_puts,
    .getc = dcc_getc,
    .pputc = dcc_pputc,
    .pgetc = dcc_pgetc};

static void dcc_init_early(const void* node, uint level) {
    pdev_register_uart(&dcc_ops);
}

static void dcc_init(const void* node, uint level) {
    spin_lock_init(&dcc_lock);

    cbuf_initialize(&rx_buf, RXBUF_SIZE);

    cbuf_initialize(&tx_buf, TXBUF_SIZE);

    thread_t* t = thread_create_etc(&dcc_do_work, "dcc_do_work", &dcc_work,
                                    NULL, DEFAULT_PRIORITY, NULL);

    if (!t) {
        panic("Unable to create dcc worker thread\n");
    }
    thread_set_cpu_affinity(t, cpu_num_to_mask(0)); // must run on CPU 0
    zx_status_t status = thread_detach_and_resume(t);

    if (status != ZX_OK) {
        panic("Unable to start dcc worker thread\n");
    }

    initialized = true;
}

LK_PDEV_INIT(dcc_init_early, KDRV_ARM_DCC, dcc_init_early, LK_INIT_LEVEL_PLATFORM_EARLY);
LK_PDEV_INIT(dcc_init, KDRV_ARM_DCC, dcc_init, LK_INIT_LEVEL_PLATFORM);
