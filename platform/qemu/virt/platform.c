/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2019 Western Digital Corporation or its affiliates.
 *
 * Authors:
 *   Anup Patel <anup.patel@wdc.com>
 *   Nick Kossifidis <mick@ics.forth.gr>
 */

#include <sbi/riscv_asm.h>
#include <sbi/riscv_encoding.h>
#include <sbi/riscv_io.h>
#include <sbi/sbi_const.h>
#include <sbi/sbi_hart.h>
#include <sbi/sbi_platform.h>
#include <sbi_utils/fdt/fdt_fixup.h>
#include <sbi_utils/irqchip/plic.h>
#include <sbi_utils/serial/uart8250.h>
#include <sbi_utils/sys/clint.h>
#include <sbi_utils/sys/sifive_test.h>

/* clang-format off */

#define VIRT_HART_COUNT			8

#define VIRT_TEST_ADDR			0x100000

#define VIRT_CLINT_ADDR			0x2000000

#define VIRT_PLIC_ADDR			0xc000000
#define VIRT_PLIC_NUM_SOURCES		127
#define VIRT_PLIC_NUM_PRIORITIES	7

#define VIRT_UART16550_ADDR		0x10000000
#define VIRT_UART_BAUDRATE		115200
#define VIRT_UART_SHIFTREG_ADDR		1843200

/* clang-format on */

static int virt_early_init(bool cold_boot)
{
	if (!cold_boot)
		return 0;

	return sifive_test_init(VIRT_TEST_ADDR);
}

static int virt_final_init(bool cold_boot)
{
	void *fdt;

	if (!cold_boot)
		return 0;

	fdt = sbi_scratch_thishart_arg1_ptr();
	fdt_fixups(fdt);

	return 0;
}

static int virt_console_init(void)
{
	return uart8250_init(VIRT_UART16550_ADDR, VIRT_UART_SHIFTREG_ADDR,
			     VIRT_UART_BAUDRATE, 0, 1);
}

static int virt_irqchip_init(bool cold_boot)
{
	int rc;
	u32 hartid = current_hartid();

	if (cold_boot) {
		rc = plic_cold_irqchip_init(
			VIRT_PLIC_ADDR, VIRT_PLIC_NUM_SOURCES, VIRT_HART_COUNT);
		if (rc)
			return rc;
	}

	return plic_warm_irqchip_init(hartid, (2 * hartid), (2 * hartid + 1));
}

static int virt_ipi_init(bool cold_boot)
{
	int rc;

	if (cold_boot) {
		rc = clint_cold_ipi_init(VIRT_CLINT_ADDR, VIRT_HART_COUNT);
		if (rc)
			return rc;
	}

	return clint_warm_ipi_init();
}

static int virt_timer_init(bool cold_boot)
{
	int rc;

	if (cold_boot) {
		rc = clint_cold_timer_init(VIRT_CLINT_ADDR,
					   VIRT_HART_COUNT, TRUE);
		if (rc)
			return rc;
	}

	return clint_warm_timer_init();
}

const struct sbi_platform_operations platform_ops = {
	.early_init		= virt_early_init,
	.final_init		= virt_final_init,
	.console_putc		= uart8250_putc,
	.console_getc		= uart8250_getc,
	.console_init		= virt_console_init,
	.irqchip_init		= virt_irqchip_init,
	.ipi_send		= clint_ipi_send,
	.ipi_clear		= clint_ipi_clear,
	.ipi_init		= virt_ipi_init,
	.timer_value		= clint_timer_value,
	.timer_event_stop	= clint_timer_event_stop,
	.timer_event_start	= clint_timer_event_start,
	.timer_init		= virt_timer_init,
	.system_reset		= sifive_test_system_reset,
};

const struct sbi_platform platform = {
	.opensbi_version	= OPENSBI_VERSION,
	.platform_version	= SBI_PLATFORM_VERSION(0x0, 0x01),
	.name			= "QEMU Virt Machine",
	.features		= SBI_PLATFORM_DEFAULT_FEATURES,
	.hart_count		= VIRT_HART_COUNT,
	.hart_stack_size	= SBI_PLATFORM_DEFAULT_HART_STACK_SIZE,
	.platform_ops_addr	= (unsigned long)&platform_ops
};
