#ifndef _MACHINE_IOAPICVAR_H_
#define _MACHINE_IOAPICVAR_H_

#include <sys/types.h>

/* Register offsets */
#define IOREGSEL    0x00
#define IOWIN       0x10
#define IOAPICVER   0x01
#define IOREDTBL    0x10

union ioapic_redentry {
    struct {
        uint8_t vector;
        uint8_t delmod          : 3;
        uint8_t destmod         : 1;
        uint8_t delivs          : 1;
        uint8_t intpol          : 1;
        uint8_t remote_irr      : 1;
        uint8_t trigger_mode    : 1;
        uint8_t interrupt_mask  : 1;
        uint64_t reserved       : 39;
        uint8_t dest_field;
    };
    uint64_t value;
};

#endif  /* !_MACHINE_IOAPICVAR_H_ */
