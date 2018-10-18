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
};

static void append_board_boot_item(zbi_header_t* bootdata) {
    // add CPU configuration
    append_boot_item(bootdata, ZBI_TYPE_CPU_CONFIG, 0, &cpu_config,
                     sizeof(zbi_cpu_config_t) +
                         sizeof(zbi_cpu_cluster_t) * cpu_config.cluster_count);

    // add memory configuration
    append_boot_item(bootdata, ZBI_TYPE_MEM_CONFIG, 0, &mem_config,
                     sizeof(zbi_mem_range_t) * countof(mem_config));
}
