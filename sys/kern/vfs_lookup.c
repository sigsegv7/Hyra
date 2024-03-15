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

#include <sys/types.h>
#include <sys/vfs.h>
#include <vm/dynalloc.h>
#include <string.h>

/*
 * Fetches the filename within a path at
 * the nth index denoted by `idx'
 *
 * Returns memory allocated by dynalloc()
 * containing the filename.
 *
 * XXX: MUST FREE RETURN VALUE WITH dynfree() WHEN
 *      DONE!
 */
char *
vfs_get_fname_at(const char *path, size_t idx)
{
    size_t pathlen = strlen(path);
    size_t fname_len;

    char *path_tmp = dynalloc(pathlen + 2);
    char *ret = NULL;
    char *start_ptr, *ptr;

    /* Make one-based */
    ++idx;

    if (path_tmp == NULL) {
        return NULL;
    }

    ptr = path_tmp;
    memcpy(path_tmp, path, pathlen + 1);

    /*
     * We want to by default have a '/' at the end
     * to keep the parsing logic from getting more
     * complicated than it needs to be.
     */
    path_tmp[pathlen] = '/';
    path_tmp[pathlen + 1] = '\0';

    /* Skip any leading slashes */
    while (*ptr == '/')
        ++ptr;

    start_ptr = ptr;

    /* Get each filename */
    while (*ptr != '\0') {
        /* Handle duplicate delimiter */
        if (*ptr == '/' && *(ptr + 1) == '/') {
            /*
             * Snip this delimiter and skip, the next
             * will be read and filename returned (if of course
             * the index is reached).
             */
            *(ptr++) = '\0';
            continue;
        }

        if (*ptr == '/') {
            *(ptr++) = '\0';

            /* Continue if index not reached */
            if ((--idx) != 0) {
                start_ptr = ptr;
                continue;
            }

            /* Index has been reached, start_ptr contains name */
            fname_len = strlen(start_ptr);
            ret = dynalloc(fname_len + 1);

            if (ret != NULL) {
                memcpy(ret, start_ptr, fname_len + 1);
            }
            break;
        }

        ++ptr;
    }

    dynfree(path_tmp);
    return ret;
}

struct vnode *
vfs_path_to_node(const char *path)
{
    /* TODO */
    return NULL;
}
