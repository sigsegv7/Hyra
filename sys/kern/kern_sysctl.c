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

#include <sys/sysctl.h>
#include <sys/syscall.h>
#include <sys/param.h>
#include <sys/errno.h>
#include <sys/systm.h>
#include <vm/dynalloc.h>
#include <vm/vm.h>
#include <string.h>

#define HYRA_RELEASE "Hyra/" HYRA_ARCH " " \
        HYRA_VERSION " "              \
        HYRA_BUILDDATE

static uint32_t pagesize = DEFAULT_PAGESIZE;
static char machine[] = HYRA_ARCH;
static char hyra[] = "Hyra";
static char hyra_version[] = HYRA_VERSION;
static char osrelease[] = HYRA_RELEASE;

/*
 * XXX: Readonly values can be statically allocated as they won't
 *      ever be changed. Values that are not readonly *must* be dynamically
 *      allocated through dynalloc(9).
 */
static struct sysctl_entry common_optab[] = {
    /* 'kern.*' */
    [KERN_OSTYPE] = { KERN_OSTYPE, SYSCTL_OPTYPE_STR_RO, hyra },
    [KERN_OSRELEASE] = { KERN_OSRELEASE, SYSCTL_OPTYPE_STR_RO, &osrelease },
    [KERN_VERSION] = { KERN_VERSION, SYSCTL_OPTYPE_STR_RO, &hyra_version },
    [KERN_VCACHE_TYPE] = { KERN_VCACHE_TYPE, SYSCTL_OPTYPE_STR, NULL },
    [KERN_HOSTNAME] = { KERN_HOSTNAME, SYSCTL_OPTYPE_STR, NULL },

    /* 'hw.*' */
    [HW_PAGESIZE] = { HW_PAGESIZE, SYSCTL_OPTYPE_INT_RO, &pagesize },
    [HW_NCPU] = { HW_NCPU, SYSCTL_OPTYPE_INT, NULL },
    [HW_MACHINE] = {HW_MACHINE, SYSCTL_OPTYPE_STR_RO, &machine }
};

static int
sysctl_write(struct sysctl_entry *entry, void *p, size_t len)
{
    void *tmp;

    /* Allocate a new value if needed */
    if (entry->data == NULL) {
        entry->data = dynalloc(len);
        goto done;
    }
    if (entry->data == NULL) {
        return -ENOMEM;
    }

    /* If we already have data allocated, resize it */
    if ((tmp = entry->data) != NULL) {
        entry->data = dynrealloc(entry->data, len);
        goto done;
    }
    if (entry->data == NULL) {
        entry->data = tmp;
        return -ENOMEM;
    }

done:
    memcpy(entry->data, p, len);
    return 0;
}

/*
 * Helper for sys_sysctl()
 */
static int
do_sysctl(struct sysctl_args *args)
{
    struct sysctl_args new_args;
    size_t name_len = 1, oldlenp = 0;
    int *name = NULL;
    void *oldp = NULL, *newp = NULL;
    int retval = 0, have_oldlen = 0;

    if (args->oldlenp != NULL) {
        have_oldlen = 1;
        name_len = args->nlen;
        retval = copyin(args->oldlenp, &oldlenp, sizeof(oldlenp));
        if (retval != 0) {
            goto done;
        }
    }

    /* Copy in newp if it is set */
    if (args->newp == NULL) {
        newp = NULL;
    } else {
        newp = dynalloc(args->newlen);
        retval = copyin(args->newp, newp, args->newlen);
    }

    if (retval != 0) {
        goto done;
    }

    name = dynalloc(name_len * sizeof(int));
    retval = copyin(args->name, name, name_len * sizeof(int));
    if (retval != 0) {
        return retval;
    }

    if (oldlenp != 0) {
        oldp = dynalloc(oldlenp);
        retval = copyin(args->oldp, oldp, oldlenp);
        if (retval != 0) {
            return retval;
        }
    }

    /* Prepare the arguments for the sysctl call */
    new_args.name = name;
    new_args.nlen = name_len;
    new_args.oldp = oldp;
    new_args.oldlenp = (have_oldlen) ? &oldlenp : NULL;
    new_args.newp = newp;
    new_args.newlen = args->newlen;

    retval = sysctl(&new_args);
    if (retval != 0) {
        goto done;
    }

    if (oldlenp != 0) {
        copyout(oldp, args->oldp, oldlenp);
    }
done:
    if (name != NULL)
        dynfree(name);
    if (newp != NULL)
        dynfree(newp);
    if (oldp != NULL)
        dynfree(oldp);

    return retval;
}

/*
 * Clear a writable sysctl string variable to the
 * value of "(undef)"
 *
 * @name: Name to clear
 */
int
sysctl_clearstr(int name)
{
    struct sysctl_args args;
    char val[] = "(undef)";
    int error;

    args.name = &name;
    args.nlen = 1;
    args.oldlenp = 0;
    args.oldp = NULL;
    args.newp = val;
    args.newlen = sizeof(val);

    if ((error = sysctl(&args)) != 0) {
        return error;
    }

    return 0;
}

int
sysctl(struct sysctl_args *args)
{
    struct sysctl_entry *tmp;
    char *tmp_str = NULL;
    int tmp_int = 0;
    size_t oldlen, len;
    int name;

    if (args->name == NULL) {
        return -EINVAL;
    }

    /* If oldlenp is not set, oldp must be the same */
    if (args->oldlenp == NULL && args->oldp != NULL) {
        return -EINVAL;
    }

    /* Get the name and make sure it is in range */
    name = *args->name;
    if (name >= NELEM(common_optab) || name < 0) {
        return -EINVAL;
    }

    tmp = &common_optab[name];
    oldlen = (args->oldlenp == NULL) ? 0 : *args->oldlenp;

    /*
     * Make sure we aren't trying to write readonly
     * entries to avoid issues...
     */
    switch (tmp->optype) {
    case SYSCTL_OPTYPE_STR_RO:
    case SYSCTL_OPTYPE_INT_RO:
        if (args->newp != NULL) {
            return -EACCES;
        }
    }

    /* If the value is unknown, bail out */
    if (args->oldp != NULL && tmp->data == NULL) {
        return -ENOTSUP;
    }

    /*
     * Now it is time to extract the value from this sysctl
     * entry if requested by the user.
     */
    if (args->oldp != NULL) {
        switch (tmp->optype) {
        /* Check for strings */
        case SYSCTL_OPTYPE_STR_RO:
        case SYSCTL_OPTYPE_STR:
            tmp_str = (char *)tmp->data;
            len = strlen(tmp_str);
            break;
        /* Check for ints */
        case SYSCTL_OPTYPE_INT_RO:
        case SYSCTL_OPTYPE_INT:
            tmp_int = *((int *)tmp->data);
            len = sizeof(tmp_int);
            break;
        }
    }

    /* If newp is set, write the new value */
    if (args->newp != NULL) {
        sysctl_write(tmp, args->newp, args->newlen);
    }

    /* Copy back old value if oldp is not NULL */
    if (args->oldp != NULL && tmp_str != NULL) {
        memcpy(args->oldp, tmp_str, oldlen);
    } else if (args->oldp != NULL) {
        memcpy(args->oldp, &tmp_int, oldlen);
    }

    if (args->oldlenp != NULL && len > oldlen) {
        return -ENOMEM;
    }

    return 0;
}

scret_t
sys_sysctl(struct syscall_args *scargs)
{
    struct sysctl_args args;
    int error;

    error = copyin((void *)scargs->arg0, &args, sizeof(args));
    if (error != 0) {
        return error;
    }

    return do_sysctl(&args);
}
