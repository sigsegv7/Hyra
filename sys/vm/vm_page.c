/*
 * Copyright (c) 2023 Ian Marco Moffett and the Osmora Team.
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

#include <sys/cdefs.h>
#include <sys/panic.h>
#include <vm/vm_page.h>
#include <vm/vm.h>
#include <string.h>

__MODULE_NAME("vm_page");
__KERNEL_META("$Hyra$: vm_page.c, Ian Marco Moffett, "
              "Virtual memory page specific operations");

/*
 * Zero `page_count' pages.
 *
 * @page: First page to start zeroing at
 * @page_count: Number of pages to zero.
 */
void
vm_zero_page(void *page, size_t page_count)
{
    const size_t PAGE_SIZE = vm_get_page_size();
    size_t bytes = page_count * PAGE_SIZE;

    /*
     * This *should not* happen. Page sizes
     * are usually 2^n but if this runs, something
     * *very* weird is happening and either something
     * broke badly. Or it's that damn technological
     * degeneration that is getting out of hand within
     * our world!! But really, page sizes *should* be a
     * power of 2 and it would be pretty worrying if
     * this branches causing a panic stating something
     * that shouldn't happen, happened.
     *
     * The reason why it is wise for pages to be a power of
     * 2 is so we can efficiently read/write to the page in
     * fixed-sized power of 2 blocks cleanly without the risk
     * of clobbering other pages we didn't intend to mess with.
     */
    if ((PAGE_SIZE & 1) != 0) {
        panic("Unexpected page size, not power of 2!\n");
    }

    memset64(page, 0, bytes/8);
}
