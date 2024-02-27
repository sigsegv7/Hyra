/*
 * Copyright (c) 2023-2024 Ian Marco Moffett and the Osmora Team.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of Hyra nor the names of its
 *    contributors may be used to endorse or promote products derived from
 *    this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/loader.h>
#include <sys/cdefs.h>
#include <sys/elf.h>
#include <sys/types.h>
#include <sys/syslog.h>
#include <vm/vm.h>
#include <vm/map.h>
#include <vm/physseg.h>
#include <string.h>
#include <assert.h>

__MODULE_NAME("kern_loader");
__KERNEL_META("$Hyra$: kern_loader.c, Ian Marco Moffett, "
              "Kernel ELF loader");

#if !defined(DEBUG)
#define DBG(...) __nothing
#else
#define DBG(...) KDEBUG(__VA_ARGS__)
#endif

#define PHDR(hdrptr, IDX) \
    (void *)((uintptr_t)hdr + (hdrptr)->e_phoff + (hdrptr->e_phentsize*IDX))

int
loader_load(const void *dataptr, struct auxval *auxv)
{
    const Elf64_Ehdr *hdr = dataptr;
    Elf64_Phdr *phdr;
    vm_prot_t prot = 0;

    uintptr_t physmem;
    size_t misalign, page_count;

    int status;
    const size_t GRANULE = vm_get_page_size();

    void *tmp_ptr;

    if (auxv == NULL) {
        DBG("Auxval argument NULL\n");
        return -1;
    }

    if (memcmp(hdr->e_ident, ELFMAG, 4) != 0) {
        /* Bad ELF header */
        DBG("ELF header bad! (Magic incorrect)\n");
        return -1;
    }

    /* Parse program headers */
    for (size_t i = 0; i < hdr->e_phnum; ++i) {
        phdr = PHDR(hdr, i);
        switch (phdr->p_type) {
        case PT_LOAD:
            if (__TEST(phdr->p_flags, PF_W))
                prot |= PROT_WRITE;
            if (__TEST(phdr->p_flags, PF_X)) {
                prot |= PROT_EXEC;
            }

            misalign = phdr->p_vaddr & (GRANULE - 1);
            page_count = __DIV_ROUNDUP(phdr->p_memsz + misalign, GRANULE);

            physmem = vm_alloc_pageframe(page_count);
            __assert(physmem != 0);     /* TODO: Handle better */

            status = vm_map_create(phdr->p_vaddr, physmem, prot,
                                   page_count * GRANULE);

            __assert(status == 0);  /* TODO: Handle better */

            /* Now we want to copy the data */
            tmp_ptr = (void *)((uintptr_t)hdr + phdr->p_offset);
            memcpy((void *)phdr->p_vaddr, tmp_ptr, phdr->p_filesz);
        }
    }

    auxv->at_entry = hdr->e_entry;
    auxv->at_phent = hdr->e_phentsize;
    auxv->at_phnum = hdr->e_phnum;
    return 0;
}
