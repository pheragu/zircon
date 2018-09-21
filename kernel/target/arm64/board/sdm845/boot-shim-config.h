#define HAS_DEVICE_TREE 1

static const zbi_cpu_config_t cpu_config = {
    .cluster_count = 1,
    .clusters = {
        {
            .cpu_count = 1,
        },
    },
};

static const zbi_mem_range_t mem_config[] = {
    {
        .type = ZBI_MEM_RANGE_RAM,
	.paddr = 0x80000000,
        .length = 0xc0000000, // 3GB
    },
    {
        .type = ZBI_MEM_RANGE_PERIPHERAL,
	.paddr = 0x0,
        .length = 0x40000000, // 3GB
    },
};

static const dcfg_arm_gicv3_driver_t gicv3_driver = {
    .mmio_phys = 0x17a00000,
    .gicd_offset = 0x00000,
    .gicr_offset = 0x060000,
    .gicr_stride = 0x020000,
    .ipi_base = 0,
};

static const dcfg_arm_generic_timer_driver_t timer_driver = {
    .irq_virt = 19,
    .freq_override = 19200000,
};

static const dcfg_arm_psci_driver_t psci_driver = {
    .use_hvc = false,
};

//static const dcfg_simple_t dcc_driver = {
//};
 
static void append_board_boot_item(zbi_header_t* bootdata) {
    // add CPU configuration
    append_boot_item(bootdata, ZBI_TYPE_CPU_CONFIG, 0, &cpu_config,
                    sizeof(zbi_cpu_config_t) +
                    sizeof(zbi_cpu_cluster_t) * cpu_config.cluster_count);

    // add memory configuration
    append_boot_item(bootdata, ZBI_TYPE_MEM_CONFIG, 0, &mem_config,
                    sizeof(zbi_mem_range_t) * countof(mem_config));

    append_boot_item(bootdata, ZBI_TYPE_KERNEL_DRIVER, KDRV_ARM_GIC_V3,
			&gicv3_driver, sizeof(gicv3_driver));

    append_boot_item(bootdata, ZBI_TYPE_KERNEL_DRIVER, 
			KDRV_ARM_GENERIC_TIMER, &timer_driver,
                    	sizeof(timer_driver));

    append_boot_item(bootdata, ZBI_TYPE_KERNEL_DRIVER, KDRV_ARM_PSCI,
			&psci_driver, sizeof(psci_driver));

//    append_boot_item(bootdata, ZBI_TYPE_KERNEL_DRIVER, 0,
//			&dcc_driver, sizeof(dcc_driver));
}
