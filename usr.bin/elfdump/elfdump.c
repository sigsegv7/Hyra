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
#include <sys/errno.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

/* Ehdr.e_type table */
static const char *elftype[] = {
    [ET_NONE] = "Untyped",
    [ET_REL] = "Relocatable",
    [ET_EXEC] = "Executable",
    [ET_DYN] = "Shared object",
    [ET_CORE] = "Core dump"
};

/* Phdr.p_type table */
static const char *phdrtype[] = {
    [PT_NULL] = "Null",
    [PT_LOAD] = "Loadable",
    [PT_DYNAMIC] = "Dynamic",
    [PT_NOTE] = "Note (linker garbage)",
};

/*
 * Verify the validity of the ELF header from its
 * various fields such as magic bytes, ABI, endianness,
 * etc.
 *
 * Returns 0 on success.
 */
static int
elf64_verify(const Elf64_Ehdr *hdr)
{
    const char *mag = &hdr->e_ident[EI_MAG0];

    if (memcmp(mag, ELFMAG, SELFMAG) != 0) {
        printf("Bad ELF magic\n");
        return -ENOEXEC;
    }
    if (hdr->e_ident[EI_OSABI] != ELFOSABI_SYSV) {
        printf("Bad ELF ABI\n");
        return -ENOEXEC;
    }
    if (hdr->e_ident[EI_DATA] != ELFDATA2LSB) {
        printf("Bad endianness\n");
        return -ENOEXEC;
    }
    if (hdr->e_ident[EI_CLASS] != ELFCLASS64) {
        printf("ELF not 64 bits\n");
        return -ENOEXEC;
    }

    return 0;
}

static void
parse_phdrs(const Elf64_Ehdr *eh, int fd)
{
    Elf64_Phdr phdr;
    const char *type = "Unknown";

    lseek(fd, eh->e_phoff, SEEK_SET);
    printf("-- PHDRS BEGIN --\n");
    for (size_t i = 0; i < eh->e_phnum; ++i) {
        if (read(fd, &phdr, eh->e_phentsize) <= 0) {
            printf("failed to read phdr %d\n", i);
            break;
        }
        if (phdr.p_type < NELEM(phdrtype)) {
            type = phdrtype[phdr.p_type];
        }

        printf("* [P.%d] Type:        %s\n", i, type);
        printf("* [P.%d] Offset:      %d\n", i, phdr.p_offset);
        printf("* [P.%d] Vaddr:       %p\n", i, phdr.p_vaddr);
        printf("* [P.%d] Paddr:       %p\n", i, phdr.p_paddr);
        printf("* [P.%d] Memory size: %d\n", i, phdr.p_memsz);
        printf("* [P.%d] Flags:       %p\n", i, phdr.p_flags);
        printf("* [P.%d] Alignment:   %p\n", i, phdr.p_align);

        /* Seperator */
        if (i < (eh->e_phnum - 1)) {
            printf("-----------------------------\n");
        }
    }
    printf("-- PHDRS END --\n");
}

static int
parse_ehdr(const Elf64_Ehdr *eh, int fd)
{
    const char *elf_type = "Bad";

    if (eh->e_type < NELEM(elftype)) {
        elf_type = elftype[eh->e_type];
    }

    printf("* Entrypoint: %p\n", eh->e_entry);
    printf("* Program headers start offset: %p\n", eh->e_phoff);
    printf("* Section headers start offset: %p\n", eh->e_shoff);
    printf("* Number of program headers: %d\n", eh->e_phnum);
    printf("* Endianess: Little\n");
    parse_phdrs(eh, fd);
    return 0;
}

static int
elfdump_run(const char *filename)
{
    Elf64_Ehdr eh;
    int fd, error;

    fd = open(filename, O_RDONLY);
    if (fd < 0) {
        return fd;
    }

    printf("-- Dumping %s --\n", filename);
    read(fd, &eh, sizeof(eh));

    if ((error = elf64_verify(&eh)) < 0) {
        return error;
    }
    if ((error = parse_ehdr(&eh, fd)) < 0) {
        return error;
    }

    close(fd);
    return 0;
}

int
main(int argc, char **argv)
{
    if (argc < 2) {
        printf("elfdump: usage: elfdump <elf path>\n");
        return -1;
    }

    return elfdump_run(argv[1]);
}
