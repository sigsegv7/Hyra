#include <sys/types.h>

struct kernel_symbol {
	uint64_t addr;
	char* name;
};

const struct kernel_symbol ksym_table[] = {
 { .addr = 0xffffffff80000000, .name = "main" },
    { .addr = (size_t)-1, .name = "" }
};

const int ksym_elem_count = sizeof(ksym_table) / sizeof(*ksym_table);
