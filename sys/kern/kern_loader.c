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
#include <sys/errno.h>
#include <vm/vm.h>
#include <vm/map.h>
#include <vm/physseg.h>
#include <vm/dynalloc.h>
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
loader_unload(struct vas vas, struct vm_range *exec_range)
{
    size_t start, end;

    start = exec_range->start;
    end = exec_range->end;

    /* FIXME: Figure out how to free physical memory too */
    return vm_map_destroy(vas, start, (end - start));
}

int loader_load(struct vas vas, const void *dataptr, struct auxval *auxv,
                size_t load_base, char **ld_path, struct vm_range *prog_range)
{
    const Elf64_Ehdr *hdr = dataptr;
    Elf64_Phdr *phdr;
    vm_prot_t prot = PROT_USER;

    uintptr_t physmem;
    size_t misalign, page_count, map_len;
    int status;

    uintptr_t start_addr = (uintptr_t)-1;
    uintptr_t end_addr = 0;

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
            map_len = page_count * GRANULE;

            /*
             * Now we want to compute the start address of the
             * program and the end address.
             */
            if (start_addr == (uintptr_t)-1) {
                start_addr = phdr->p_vaddr;
            }

            end_addr = __MAX(end_addr, phdr->p_vaddr + page_count*GRANULE);

            /* Do we not have enough page frames? */
            if (physmem == 0) {
                DBG("Failed to allocate physical memory\n");
                vm_free_pageframe(physmem, page_count);
                return -ENOMEM;
            }

            status = vm_map_create(vas, phdr->p_vaddr + load_base, physmem, prot, map_len);

            if (status != 0) {
                return status;
            }

            /* Now we want to copy the data */
            tmp_ptr = (void *)((uintptr_t)hdr + phdr->p_offset);
            memcpy(PHYS_TO_VIRT(physmem), tmp_ptr, phdr->p_filesz);
            break;
        case PT_INTERP:
            if (ld_path == NULL) {
                break;
            }

            *ld_path = dynalloc(phdr->p_filesz);

            if (ld_path == NULL) {
                DBG("Failed to allocate memory for PT_INTERP path\n");
                return -ENOMEM;
            }

            memcpy(*ld_path, (char *)hdr + phdr->p_offset, phdr->p_filesz);
            break;
        case PT_PHDR:
            auxv->at_phdr = phdr->p_vaddr + load_base;
            break;
        }
    }

    auxv->at_entry = hdr->e_entry + load_base;
    auxv->at_phent = hdr->e_phentsize;
    auxv->at_phnum = hdr->e_phnum;
    prog_range->start = start_addr;
    prog_range->end = end_addr;
    return 0;
}
