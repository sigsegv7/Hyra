/*
 * Copyright (c) 2024 Ian Marco Moffett and the Osmora Team.
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

#include <machine/spectre.h>
#include <machine/cpuid.h>
#include <machine/msr.h>
#include <sys/syslog.h>
#include <sys/types.h>

__MODULE_NAME("spectre");
__KERNEL_META("$Hyra$: spectre.c, Ian Marco Moffett, "
              "Spectre mitigation support");

#if __SPECTRE_MITIGATION == 1

/*
 * Returns true if Indirect Branch Restricted Speculation (IBRS)
 * is supported.
 */
__naked bool
__can_mitigate_spectre(void);

/*
 * Returns EXIT_FAILURE if not supported, returns
 * EXIT_SUCCESS if mitigation is now active.
 *
 * This function will be NULL if spectre mitigation isn't enabled;
 * therefore it is wise to verify to prevent access violations and
 * undefined behaviour.
 *
 * This behaviour is governed by the __SPECTRE_MITIGATION define
 *
 * TODO: Try to enable others, not just IBRS
 */
__weak int
try_spectre_mitigate(void)
{
    uint64_t tmp;

    if (!__can_mitigate_spectre()) {
        KINFO("IBRS not supported; spectre mitigation NOT enabled\n");
        return EXIT_FAILURE;
    }

    KINFO("IBRS supported; spectre mitigation enabled\n");
    tmp = rdmsr(IA32_SPEC_CTL);
    tmp |= __BIT(0);                /* IBRS */
    wrmsr(IA32_SPEC_CTL, tmp);
    return EXIT_SUCCESS;
}

#endif      /* __SPECTRE_MITIGATION == 1 */
