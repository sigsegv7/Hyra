/*
 * Copyright (c) 2025 Ian Marco Moffett and the Osmora Team.
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

#include <sys/stat.h>
#include <sys/errno.h>
#include <stdio.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>
#include <stddef.h>
#include <stdint.h>
#include <assert.h>
#include <dirent.h>
#include <string.h>
#include <libgen.h>

/* OMAR magic constants */
#define OMAR_MAGIC "OMAR"
#define OMAR_EOF "RAMO"

/* OMAR type constants */
#define OMAR_REG    0
#define OMAR_DIR    1

/* OMAR modes */
#define OMAR_ARCHIVE  0
#define OMAR_EXTRACT  1

/* Revision */
#define OMAR_REV 2

#define ALIGN_UP(value, align)        (((value) + (align)-1) & ~((align)-1))
#define BLOCK_SIZE 512

static int mode = OMAR_ARCHIVE;
static int outfd;
static const char *inpath = NULL;
static const char *outpath = NULL;

/*
 * The OMAR file header, describes the basics
 * of a file.
 *
 * @magic: Header magic ("OMAR")
 * @len: Length of the file
 * @namelen: Length of the filename
 * @rev: OMAR revision
 * @mode: File permissions
 */
struct omar_hdr {
    char magic[4];
    uint8_t type;
    uint8_t namelen;
    uint32_t len;
    uint8_t rev;
    uint32_t mode;
} __attribute__((packed));

static inline void
help(void)
{
    printf("--------------------------------------\n");
    printf("The OSMORA archive format\n");
    printf("Usage: omar -i [input_dir] -o [output]\n");
    printf("-h      Show this help screen\n");
    printf("-x      Extract an OMAR archive\n");
    printf("--------------------------------------\n");
}

/*
 * Strip out root dir
 *
 * XXX: This is added code to work with Hyra
 *      initramfs.
 */
static const char *
strip_root(const char *path)
{
    const char *p;

    for (p = path; *p != '\0'; ++p) {
        if (*p == '/') {
            ++p;
            return p;
        }
    }

    return NULL;
}

/*
 * Recursive mkdir
 */
static void
mkpath(struct omar_hdr *hdr, const char *path)
{
    size_t len;
    char buf[256];
    char cwd[256];
    char *p = NULL;

    len = snprintf(buf, sizeof(buf), "%s", path);
    if (buf[len - 1] == '/') {
        buf[len - 1] = '\0';
    }
    for (p = (char *)buf + 1; *p != '\0'; ++p) {
        if (*p == '/') {
            *p = '\0';
            mkdir(buf, hdr->mode);
            *p = '/';
        }
    }

    mkdir(buf, hdr->mode);
}

/*
 * Push a file into the archive output
 *
 * @pathname: Full path name of file (NULL if EOF)
 * @name: Name of file (for EOF, set to "EOF")
 */
static int
file_push(const char *pathname, const char *name)
{
    struct omar_hdr hdr;
    struct stat sb;
    int infd, rem, error;
    int pad_len;
    size_t len;
    char *buf;

    hdr.type = OMAR_REG;

    /* Attempt to open the input file if not EOF */
    if (pathname != NULL) {
        if ((infd = open(pathname, O_RDONLY)) < 0) {
            perror("open");
            return infd;
        }

        if ((error = fstat(infd, &sb)) < 0) {
            return error;
        }

        if (S_ISDIR(sb.st_mode)) {
            hdr.type = OMAR_DIR;
        }
        hdr.mode = sb.st_mode;
    }

    hdr.len = (pathname == NULL) ? 0 : sb.st_size;
    hdr.rev = OMAR_REV;
    hdr.namelen = strlen(name);

    /*
     * If we are at the end of the file, use the OMAR_EOF
     * magic constant instant of the usual OMAR_MAGIC.
     */
    if (pathname == NULL) {
        memcpy(hdr.magic, OMAR_EOF, sizeof(hdr.magic));
    } else {
        memcpy(hdr.magic, OMAR_MAGIC, sizeof(hdr.magic));
    }

    write(outfd, &hdr, sizeof(hdr));
    write(outfd, name, hdr.namelen);

    /* If we are at the end of file, we are done */
    if (pathname == NULL) {
        close(infd);
        return 0;
    }

    /* Pad directories to zero */
    if (hdr.type == OMAR_DIR) {
        len = sizeof(hdr) + hdr.namelen;
        rem = len & (BLOCK_SIZE - 1);
        pad_len = BLOCK_SIZE - rem;

        buf = malloc(pad_len);
        memset(buf, 0, pad_len);
        write(outfd, buf, pad_len);
        free(buf);
        return 0;
    }

    /* We need the file data now */
    buf = malloc(hdr.len);
    if (buf == NULL) {
        printf("out of memory\n");
        close(infd);
        return -ENOMEM;
    }
    if (read(infd, buf, hdr.len) <= 0) {
        perror("read");
        close(infd);
        return -EIO;
    }

    /*
     * Write the actual file contents, if the file length is not
     * a multiple of the block size, we'll need to pad out the rest
     * to zero.
     */
    write(outfd, buf, hdr.len);
    len = sizeof(hdr) + (hdr.namelen + hdr.len);
    rem = len & (BLOCK_SIZE - 1);
    if (rem != 0) {
        /* Compute the padding length */
        pad_len = BLOCK_SIZE - rem;

        buf = realloc(buf, pad_len);
        memset(buf, 0, pad_len);
        write(outfd, buf, pad_len);
    }
    close(infd);
    free(buf);
    return 0;
}

/*
 * Start creating an archive from the
 * basepath of a directory.
 */
static int
archive_create(const char *base, const char *dirname)
{
    DIR *dp;
    struct dirent *ent;
    struct omar_hdr hdr;
    const char *p = NULL, *p1;
    char pathbuf[256];
    char namebuf[256];

    dp = opendir(base);
    if (dp == NULL) {
        perror("opendir");
        return -ENOENT;
    }

    while ((ent = readdir(dp)) != NULL) {
        if (ent->d_name[0] == '.') {
            continue;
        }

        snprintf(pathbuf, sizeof(pathbuf), "%s/%s", base, ent->d_name);
        snprintf(namebuf, sizeof(namebuf), "%s/%s", dirname, ent->d_name);
        p1 = strip_root(namebuf);

        if (ent->d_type == DT_DIR) {
            printf("%s [d]\n", p1);
            file_push(pathbuf, p1);
            archive_create(pathbuf, namebuf);
        } else if (ent->d_type == DT_REG) {
            printf("%s [f]\n", p1);
            file_push(pathbuf, p1);
        }
    }

    return 0;
}

/*
 * Extract a single file
 *
 * @hp: File header
 * @data: Data to extract
 * @len: Length of data
 * @path: Path to output file
 */
static int
extract_single(struct omar_hdr *hp, char *data, size_t len, const char *path)
{
    int fd;

    if ((fd = open(path, O_WRONLY | O_CREAT, hp->mode)) < 0) {
        return fd;
    }

    return write(fd, data, len) > 0 ? 0 : -1;
}

/*
 * Extract an OMAR archive.
 *
 * XXX: The input file [-i] will be the OMAR archive to
 *      be extracted, the output directory [-o] will be
 *      where the files get extracted.
 */
static int
archive_extract(void)
{
    char *buf, *name, *p;
    struct stat sb;
    struct omar_hdr *hdr;
    int fd, error;
    size_t len;
    off_t off;
    char namebuf[256];
    char pathbuf[256];

    if ((fd = open(inpath, O_RDONLY)) < 0) {
        perror("open");
        return fd;
    }

    if ((error = fstat(fd, &sb)) != 0) {
        perror("fstat");
        close(fd);
        return error;
    }

    buf = malloc(sb.st_size);
    if (buf == NULL) {
        fprintf(stderr, "out of memory\n");
        close(fd);
        return -ENOMEM;
    }

    if (read(fd, buf, sb.st_size) <= 0) {
        fprintf(stderr, "omar: no data read\n");
        close(fd);
        return -EIO;
    }

    hdr = (struct omar_hdr *)buf;
    for (;;) {
        if (memcmp(hdr->magic, OMAR_EOF, sizeof(OMAR_EOF)) == 0) {
            printf("EOF!\n");
            return 0;
        }

        /* Ensure the header is valid */
        if (memcmp(hdr->magic, "OMAR", 4) != 0) {
            fprintf(stderr, "bad magic\n");
            break;
        }
        if (hdr->rev != OMAR_REV) {
            fprintf(stderr, "cannot extract rev %d archive\n", hdr->rev);
            fprintf(stderr, "current OMAR revision: %d\n", OMAR_REV);
        }

        name = (char *)hdr + sizeof(struct omar_hdr);
        memcpy(namebuf, name, hdr->namelen);
        namebuf[hdr->namelen] = '\0';

        /* Get the full path */
        len = snprintf(pathbuf, sizeof(pathbuf), "%s/%s", outpath, namebuf);
        if (len < 0) {
            free(buf);
            return len;
        }
        printf("unpacking %s\n", pathbuf);

        if (hdr->type == OMAR_DIR) {
            off = 512;
            mkpath(hdr, pathbuf);
        } else {
            off = ALIGN_UP(sizeof(*hdr) + hdr->namelen + hdr->len, BLOCK_SIZE);
            p = (char *)hdr + sizeof(struct omar_hdr);
            p += hdr->namelen;
            extract_single(hdr, p, hdr->len, pathbuf);
        }

        hdr = (struct omar_hdr *)((char *)hdr + off);
        memset(namebuf, 0, sizeof(namebuf));
    }

    free(buf);
}

int
main(int argc, char **argv)
{
    int optc, retval;
    int error, flags;

    if (argc < 2) {
        help();
        return -1;
    }

    while ((optc = getopt(argc, argv, "xhi:o:")) != -1) {
        switch (optc) {
        case 'x':
            mode = OMAR_EXTRACT;
            break;
        case 'i':
            inpath = optarg;
            break;
        case 'o':
            outpath = optarg;
            break;
        case 'h':
            help();
            return 0;
        default:
            help();
            return -1;
        }
    }

    if (inpath == NULL) {
        fprintf(stderr, "omar: no input path\n");
        help();
        return -1;
    }
    if (outpath == NULL) {
        fprintf(stderr, "omar: no output path\n");
        help();
        return -1;
    }

    /*
     * Do our specific job based on the mode
     * OMAR is set to be in.
     */
    switch (mode) {
    case OMAR_ARCHIVE:
        /* Begin archiving the file */
        outfd = open(outpath, O_WRONLY | O_CREAT, 0700);
        if (outfd < 0) {
            printf("omar: failed to open output file\n");
            return outfd;
        }

        retval = archive_create(inpath, basename((char *)inpath));
        file_push(NULL, "EOF");
        break;
    case OMAR_EXTRACT:
        /* Begin extracting the file */
        if ((error = mkdir(outpath, 0700) != 0)) {
            perror("mkdir");
            return error;
        }

        retval = archive_extract();
        break;
    }
    close(outfd);
    return retval;
}
