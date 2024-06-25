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

#include <vm/dynalloc.h>
#include <vm/vm.h>

/*
 * Dynamically allocates memory
 *
 * @sz: The amount of bytes to allocate
 */
void *
dynalloc(size_t sz)
{
    struct vm_ctx *vm_ctx = vm_get_ctx();
    void *tmp;

    spinlock_acquire(&vm_ctx->dynalloc_lock);
    tmp = tlsf_malloc(vm_ctx->tlsf_ctx, sz);
    spinlock_release(&vm_ctx->dynalloc_lock);
    return tmp;
}

/*
 * Reallocates memory pool created by `dynalloc()'
 *
 * @old_ptr: Pointer to old pool.
 * @newsize: Size of new pool.
 */
void *
dynrealloc(void *old_ptr, size_t newsize)
{
    struct vm_ctx *vm_ctx = vm_get_ctx();
    void *tmp;

    spinlock_acquire(&vm_ctx->dynalloc_lock);
    tmp = tlsf_realloc(vm_ctx->tlsf_ctx, old_ptr, newsize);
    spinlock_release(&vm_ctx->dynalloc_lock);
    return tmp;
}

/*
 * Free dynamically allocated memory
 *
 * @ptr: Pointer to base of memory.
 */
void
dynfree(void *ptr)
{
    struct vm_ctx *vm_ctx = vm_get_ctx();

    spinlock_acquire(&vm_ctx->dynalloc_lock);
    tlsf_free(vm_ctx->tlsf_ctx, ptr);
    spinlock_release(&vm_ctx->dynalloc_lock);
}
