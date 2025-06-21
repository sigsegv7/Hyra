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

#include <sys/time.h>
#include <sys/cdefs.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>

#define MONTHS_PER_YEAR 12
#define DAYS_PER_WEEK 7

/* Months of the year */
static const char *montab[] = {
    "Jan", "Feb", "Mar",
    "Apr", "May", "Jun",
    "Jul", "Aug", "Sep",
    "Oct", "Nov", "Dec"
};

/* Days of the week */
static const char *daytab[] = {
    "Sat", "Sun", "Mon",
    "Tue", "Wed", "Thu",
    "Fri"
};

int
main(void)
{
    const char *day, *month;
    char date_str[32];
    struct date d;
    int rtc_fd;

    if ((rtc_fd = open("/dev/rtc", O_RDONLY)) < 0) {
        return rtc_fd;
    }

    read(rtc_fd, &d, sizeof(d));
    close(rtc_fd);

    /* This should not happen */
    if (__unlikely(d.month > MONTHS_PER_YEAR)) {
        printf("got bad month %d from RTC\n", d.month);
        return -1;
    }
    if (__unlikely(d.month == 0 || d.day == 0)) {
        printf("got zero month/day from RTC\n");
        return -1;
    }

    day = daytab[d.day % DAYS_PER_WEEK];
    month = montab[d.month - 1];

    snprintf(date_str, sizeof(date_str), "%s %s %d %02d:%02d:%02d\n",
        day, month, d.day, d.hour, d.min, d.sec);
    fputs(date_str, stdout);
    return 0;
}
