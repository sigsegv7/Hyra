/*
 * Copyright (c) 2023-2025 Ian Marco Moffett and the Osmora Team.
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

#include <sys/elf.h>
#include <sys/exec.h>
#include <sys/param.h>
#include <sys/syslog.h>
#include <sys/namei.h>
#include <sys/proc.h>
#include <sys/vnode.h>
#include <sys/errno.h>
#include <vm/pmap.h>
#include <vm/physmem.h>
#include <vm/dynalloc.h>
#include <vm/vm.h>
#include <vm/map.h>
#include <string.h>
#include <machine/pcb.h>

#define pr_trace(fmt, ...) kprintf("elf64: " fmt, ##__VA_ARGS__)
#define pr_error(...) pr_trace(__VA_ARGS__)

#define PHDR(HDRP, IDX) \
    (void *)((uintptr_t)HDRP + (HDRP)->e_phoff + (HDRP->e_phentsize * IDX))

struct elf_file {
    char *data;
    size_t size;
};

/*
 * Load the file and give back an "elf_file"
 * structure.
 */
static int
elf_get_file(const char *pathname, struct elf_file *res)
{
    struct vnode *vp = NULL;
    struct nameidata nd;
    struct vattr vattr;
    struct vop_getattr_args getattr_args;
    struct sio_txn read_txn;
    int status = 0;

    nd.path = pathname;
    nd.flags = 0;

    if (res == NULL)
        return -EINVAL;
    if ((status = namei(&nd)) != 0)
        return status;

    vp = nd.vp;

    getattr_args.res = &vattr;
    getattr_args.vp = vp;
    status = vfs_vop_getattr(vp, &getattr_args);
    if (status != 0)
        goto done;

    /* Can we use the size field? */
    if (vattr.size == VNOVAL) {
        status = -EIO;
        goto done;
    }

    res->size = vattr.size;
    res->data = dynalloc(sizeof(char) * res->size);
    if (res->data == NULL) {
        status = -ENOMEM;
        goto done;
    }

    /* Read data into our buffer */
    read_txn.buf = res->data;
    read_txn.len = res->size;
    read_txn.offset = 0;
    vfs_vop_read(vp, &read_txn);

done:
    if (vp != NULL) {
        vfs_release_vnode(nd.vp);
    }
    return status;
}

/*
 * Verify the validity of the ELF header.
 * Returns 0 on success.
 */
static int
elf64_verify(Elf64_Ehdr *hdr)
{
    const char *mag = &hdr->e_ident[EI_MAG0];

    if (memcmp(mag, ELFMAG, SELFMAG) != 0) {
        /* Bad magic */
        return -ENOEXEC;
    }

    if (hdr->e_ident[EI_OSABI] != ELFOSABI_SYSV) {
        /* ABI used is not System V */
        return -ENOEXEC;
    }

    if (hdr->e_ident[EI_DATA] != ELFDATA2LSB) {
        /* Not little-endian */
        return -ENOEXEC;
    }

    if (hdr->e_ident[EI_CLASS] != ELFCLASS64) {
        /* Not 64-bits */
        return -ENOEXEC;
    }

    if (hdr->e_type != ET_EXEC) {
        /* Not executable */
        return -ENOEXEC;
    }

    if (hdr->e_phnum > MAX_PHDRS) {
        /* Too many program headers */
        return -ENOEXEC;
    }

    return 0;
}

void
elf_unload(struct proc *td, struct exec_prog *prog)
{
    size_t map_len;
    struct pcb *pcbp = &td->pcb;
    struct auxval auxval = prog->auxval;
    struct exec_range *loadmap = prog->loadmap;

    for (size_t i = 0; i < auxval.at_phnum; ++i) {
        map_len = (loadmap[i].end - loadmap[i].start);
        vm_unmap(pcbp->addrsp, loadmap[i].start, map_len);
        vm_free_frame(loadmap[i].start, map_len / DEFAULT_PAGESIZE);
    }
}

int
elf64_load(const char *pathname, struct proc *td, struct exec_prog *prog)
{
    vm_prot_t prot = (PROT_READ | PROT_USER);
    Elf64_Ehdr *hdr;
    Elf64_Phdr *phdr;
    paddr_t physmem;
    vaddr_t start, end;
    off_t misalign;
    void *tmp;
    size_t page_count, map_len;
    struct elf_file file;
    struct pcb *pcbp;
    struct exec_range loadmap[MAX_PHDRS];
    struct auxval *auxvalp;
    size_t loadmap_idx = 0;
    int status = 0;

    if ((status = elf_get_file(pathname, &file)) != 0)
        return status;

    hdr = (Elf64_Ehdr *)file.data;
    if ((status = elf64_verify(hdr)) != 0)
        goto done;

    memset(loadmap, 0, sizeof(loadmap));
    pcbp = &td->pcb;
    start = -1;
    end = 0;

    /* Load program headers */
    for (size_t i = 0; i < hdr->e_phnum; ++i) {
        phdr = PHDR(hdr, i);
        switch (phdr->p_type) {
        case PT_LOAD:
            if (ISSET(phdr->p_flags, PF_W))
                prot |= PROT_WRITE;
            if (ISSET(phdr->p_flags, PF_X))
                prot |= PROT_EXEC;

            misalign = phdr->p_vaddr & (DEFAULT_PAGESIZE - 1);
            map_len = ALIGN_UP(phdr->p_memsz + misalign, DEFAULT_PAGESIZE);
            page_count = map_len / DEFAULT_PAGESIZE;

            /* Try to allocate page frames */
            physmem = vm_alloc_frame(page_count);
            if (physmem == 0) {
                pr_error("out of physical memory\n");
                status = -ENOMEM;
                break;
            }

            status = vm_map(pcbp->addrsp, phdr->p_vaddr, physmem,
                prot, map_len);

            if (status != 0) {
                break;
            }

            tmp = (void *)((uintptr_t)hdr + phdr->p_offset);
            memcpy(PHYS_TO_VIRT(physmem), tmp, phdr->p_filesz);

            loadmap[loadmap_idx].start = physmem;
            loadmap[loadmap_idx].end = physmem + map_len;
            loadmap[loadmap_idx].vbase = phdr->p_vaddr;

            /* Get start/end addresses */
            if (start == (vaddr_t)-1)
                start = loadmap[loadmap_idx].vbase;
            if (phdr->p_vaddr > end)
                end = loadmap[loadmap_idx].vbase + phdr->p_memsz;

            ++loadmap_idx;
        }
    }

    memcpy(prog->loadmap, loadmap, sizeof(loadmap));
    prog->start = start;
    prog->end = end;

    auxvalp = &prog->auxval;
    auxvalp->at_entry = hdr->e_entry;
    auxvalp->at_phent = hdr->e_phentsize;
    auxvalp->at_phnum = hdr->e_phnum;

    /* Did program header loading fail? */
    if (status != 0) {
        elf_unload(td, prog);
        goto done;
    }

done:
    dynfree(file.data);
    return status;
}
