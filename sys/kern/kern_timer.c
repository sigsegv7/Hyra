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

#include <sys/timer.h>

/*
 * When a timer on the machine has been registered
 * to the Hyra kernel, they'll be added to the Hyra timer
 * registry.
 */
static const struct timer *tmr_registry[TIMER_ID_COUNT] = { 0 };

/*
 * Returns true if the timer ID given
 * is valid.
 *
 * @id: ID to verify.
 */
static inline bool
is_timer_id_valid(timer_id_t id)
{
    return id < TIMER_ID_COUNT;
}

/*
 * Adds timer on the machine to the timer registry. To be specific,
 * this function writes information about the specific timer to the
 * timer registry. However, it will not overwrite an entry. To do this
 * you must use tmr_registry_overwrite(), of course with caution.
 *
 * @id: ID of timer to register.
 * @tmr: Timer descriptor to register.
 */
tmrr_status_t
register_timer(timer_id_t id, const struct timer *tmr)
{
    if (!is_timer_id_valid(id))
        return TMRR_INVALID_TYPE;

    if (tmr_registry[id] != NULL)
        return TMRR_HAS_ENTRY;

    tmr_registry[id] = tmr;
    return TMRR_SUCCESS;
}

/*
 * Overwrites an entry within the timer registery.
 * Use with caution.
 *
 * @id: ID of entry to overwrite.
 * @tmr: Timer descriptor to write.
 */
tmrr_status_t
tmr_registry_overwrite(timer_id_t id, const struct timer *tmr)
{
    if (!is_timer_id_valid(id))
        return TMRR_INVALID_TYPE;

    tmr_registry[id] = tmr;
    return TMRR_SUCCESS;
}

/*
 * Requests a specific timer descriptor
 * with a specific ID.
 *
 * @id: ID to request.
 * @tmr_out: Pointer to memory that'll hold the
 *           requested descriptor.
 */
tmrr_status_t
req_timer(timer_id_t id, struct timer *tmr_out)
{
    if (!is_timer_id_valid(id))
        return TMRR_INVALID_TYPE;

    if (tmr_registry[id] == NULL)
        return TMRR_EMPTY_ENTRY;

    if (tmr_out == NULL)
        return TMRR_INVALID_ARG;

    *tmr_out = *tmr_registry[id];
    return TMRR_SUCCESS;
}
