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
        .length = 0x40000000,
    },
    {
        .type = ZBI_MEM_RANGE_RESERVED,
        .paddr = 0x85700000,
        .length = 0x600000,
    },
    {
        .type = ZBI_MEM_RANGE_RESERVED,
        .paddr = 0x85e00000,
        .length = 0x100000,
    },
    {
        .type = ZBI_MEM_RANGE_RESERVED,
        .paddr = 0x85fc0000,
        .length = 0x2f40000,
    },
    {
        .type = ZBI_MEM_RANGE_RESERVED,
        .paddr = 0x8ab00000,
        .length = 0x1400000,
    },
    {
        .type = ZBI_MEM_RANGE_RESERVED,
        .paddr = 0x8bf00000,
        .length = 0x500000,
    },
    {
        .type = ZBI_MEM_RANGE_RESERVED,
        .paddr = 0x8c400000,
        .length = 0x10000,
    },
    {
        .type = ZBI_MEM_RANGE_RESERVED,
        .paddr = 0x8c410000,
        .length = 0x5000,
    },

    {
        .type = ZBI_MEM_RANGE_RESERVED,
        .paddr = 0x8c415000,
        .length = 0x2000,
    },
    {
        .type = ZBI_MEM_RANGE_RESERVED,
        .paddr = 0x8c500000,
        .length = 0x1a00000,
    },
    {
        .type = ZBI_MEM_RANGE_RESERVED,
        .paddr = 0x8df00000,
        .length = 0x100000,
    },
    {
        .type = ZBI_MEM_RANGE_RESERVED,
        .paddr = 0x8e000000,
        .length = 0x7800000,
    },
    {
        .type = ZBI_MEM_RANGE_RESERVED,
        .paddr = 0x95800000,
        .length = 0x500000,
    },
    {
        .type = ZBI_MEM_RANGE_RESERVED,
        .paddr = 0x95d00000,
        .length = 0x800000,
    },
    {
        .type = ZBI_MEM_RANGE_RESERVED,
        .paddr = 0x96500000,
        .length = 0x200000,
    },
    {
        .type = ZBI_MEM_RANGE_RESERVED,
        .paddr = 0x96700000,
        .length = 0x1400000,
    },
    {
        .type = ZBI_MEM_RANGE_RESERVED,
        .paddr = 0x97b00000,
        .length = 0x100000,
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
    .use_hvc = true,
};

static const dcfg_simple_t dcc_driver = {};

static const dcfg_simple_t wdog_driver = {
    .mmio_phys = 0x17980000,
    .irq = 32,
};

static const zbi_platform_id_t platform_id = {
    .vid = PDEV_VID_QCOM,
    .pid = PDEV_PID_SDM845,
    .board_name = "sdm845",
};

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

    append_boot_item(bootdata, ZBI_TYPE_KERNEL_DRIVER, KDRV_ARM_DCC,
                     &dcc_driver, sizeof(dcc_driver));

    append_boot_item(bootdata, ZBI_TYPE_KERNEL_DRIVER, KDRV_ARM_WATCH_DOG,
                     &wdog_driver, sizeof(wdog_driver));

    append_boot_item(bootdata, ZBI_TYPE_PLATFORM_ID, 0, &platform_id,
                     sizeof(platform_id));
}
