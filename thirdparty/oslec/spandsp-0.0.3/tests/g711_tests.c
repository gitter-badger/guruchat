/*
 * SpanDSP - a series of DSP components for telephony
 *
 * g711_tests.c
 *
 * Written by Steve Underwood <steveu@coppice.org>
 *
 * Copyright (C) 2006 Steve Underwood
 *
 * All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2, as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 * $Id: g711_tests.c,v 1.4 2006/11/19 14:07:27 steveu Exp $
 */

/*! \page g711_tests_page A-law and u-law conversion tests
\section g711_tests_page_sec_1 What does it do?

\section g711_tests_page_sec_2 How is it used?
*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <inttypes.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#if defined(HAVE_TGMATH_H)
#include <tgmath.h>
#endif
#if defined(HAVE_MATH_H)
#include <math.h>
#endif
#include <assert.h>
#include <audiofile.h>
#include <tiffio.h>

#include "spandsp.h"

#define OUT_FILE_NAME   "g711.wav"

int16_t amp[65536];

const uint8_t alaw_1khz_sine[] = {0x34, 0x21, 0x21, 0x34, 0xB4, 0xA1, 0xA1, 0xB4};
const uint8_t ulaw_1khz_sine[] = {0x1E, 0x0B, 0x0B, 0x1E, 0x9E, 0x8B, 0x8B, 0x9E};

int main(int argc, char *argv[])
{
    AFfilehandle outhandle;
    AFfilesetup filesetup;
    power_meter_t power_meter;
    int outframes;
    int i;
    int block;
    int pre;
    int post;
    int alaw_failures;
    int ulaw_failures;
    float worst_alaw;
    float worst_ulaw;
    float tmp;
    
    if ((filesetup = afNewFileSetup()) == AF_NULL_FILESETUP)
    {
        fprintf(stderr, "    Failed to create file setup\n");
        exit(2);
    }
    afInitSampleFormat(filesetup, AF_DEFAULT_TRACK, AF_SAMPFMT_TWOSCOMP, 16);
    afInitRate(filesetup, AF_DEFAULT_TRACK, (float) SAMPLE_RATE);
    afInitFileFormat(filesetup, AF_FILE_WAVE);
    afInitChannels(filesetup, AF_DEFAULT_TRACK, 1);

    if ((outhandle = afOpenFile(OUT_FILE_NAME, "w", filesetup)) == AF_NULL_FILEHANDLE)
    {
        fprintf(stderr, "    Cannot create wave file '%s'\n", OUT_FILE_NAME);
        exit(2);
    }

    printf("Conversion accuracy tests.\n");
    alaw_failures = 0;
    ulaw_failures = 0;
    worst_alaw = 0.0;
    worst_ulaw = 0.0;
    for (block = 0;  block < 1;  block++)
    {
        for (i = 0;  i < 65536;  i++)
        {
            pre = i - 32768;
            post = alaw_to_linear(linear_to_alaw(pre));
            if (abs(pre) > 140)
            {
                tmp = (float) abs(post - pre)/(float) abs(pre);
                if (tmp > 0.10)
                {
                    printf("A-law: Excessive error at %d (%d)\n", pre, post);
                    alaw_failures++;
                }
                if (tmp > worst_alaw)
                    worst_alaw = tmp;
            }
            else
            {
                /* Small values need different handling for sensible measurement */
                if (abs(post - pre) > 15)
                {
                    printf("A-law: Excessive error at %d (%d)\n", pre, post);
                    alaw_failures++;
                }
            }
            amp[i] = post;
        }
        outframes = afWriteFrames(outhandle,
                                  AF_DEFAULT_TRACK,
                                  amp,
                                  65536);
        if (outframes != 65536)
        {
            fprintf(stderr, "    Error writing wave file\n");
            exit(2);
        }
        for (i = 0;  i < 65536;  i++)
        {
            pre = i - 32768;
            post = ulaw_to_linear(linear_to_ulaw(pre));
            if (abs(pre) > 40)
            {
                tmp = (float) abs(post - pre)/(float) abs(pre);
                if (tmp > 0.10)
                {
                    printf("u-law: Excessive error at %d (%d)\n", pre, post);
                    ulaw_failures++;
                }
                if (tmp > worst_ulaw)
                    worst_ulaw = tmp;
            }
            else
            {
                /* Small values need different handling for sensible measurement */
                if (abs(post - pre) > 4)
                {
                    printf("u-law: Excessive error at %d (%d)\n", pre, post);
                    ulaw_failures++;
                }
            }
            amp[i] = post;
        }
        outframes = afWriteFrames(outhandle,
                                  AF_DEFAULT_TRACK,
                                  amp,
                                  65536);
        if (outframes != 65536)
        {
            fprintf(stderr, "    Error writing wave file\n");
            exit(2);
        }
    }
    printf("Worst A-law error (ignoring small values) %f%%\n", worst_alaw*100.0);
    printf("Worst u-law error (ignoring small values) %f%%\n", worst_ulaw*100.0);
    if (alaw_failures  ||  ulaw_failures)
    {
        printf("%d A-law values with excessive error\n", alaw_failures);
        printf("%d u-law values with excessive error\n", ulaw_failures);
        printf("Tests failed\n");
        exit(2);
    }
    
    printf("Reference power level tests.\n");
    power_meter_init(&power_meter, 7);

    for (i = 0;  i < 8000;  i++)
    {
        amp[i] = ulaw_to_linear(ulaw_1khz_sine[i & 7]);
        power_meter_update(&power_meter, amp[i]);
    }
    printf("Reference u-law 1kHz tone is %fdBm0\n", power_meter_dbm0(&power_meter));
    outframes = afWriteFrames(outhandle,
                              AF_DEFAULT_TRACK,
                              amp,
                              8000);
    if (outframes != 8000)
    {
        fprintf(stderr, "    Error writing wave file\n");
        exit(2);
    }
    if (0.1f < fabs(power_meter_dbm0(&power_meter)))
    {
        printf("Test failed.\n");
        exit(2);
    }

    for (i = 0;  i < 8000;  i++)
    {
        amp[i] = alaw_to_linear(alaw_1khz_sine[i & 7]);
        power_meter_update(&power_meter, amp[i]);
    }
    printf("Reference A-law 1kHz tone is %fdBm0\n", power_meter_dbm0(&power_meter));
    outframes = afWriteFrames(outhandle,
                              AF_DEFAULT_TRACK,
                              amp,
                              8000);
    if (outframes != 8000)
    {
        fprintf(stderr, "    Error writing wave file\n");
        exit(2);
    }
    if (0.1f < fabs(power_meter_dbm0(&power_meter)))
    {
        printf("Test failed.\n");
        exit(2);
    }

    /* Check the transcoding functions. */
    printf("Testing transcoding A-law -> u-law -> A-law\n");
    for (i = 0;  i < 256;  i++)
    {
        if (alaw_to_ulaw(ulaw_to_alaw(i)) != i)
        {
            if (abs(alaw_to_ulaw(ulaw_to_alaw(i)) - i) > 1)
            {
                printf("u-law -> A-law -> u-law gave %d -> %d\n", i, alaw_to_ulaw(ulaw_to_alaw(i)));
                printf("Test failed\n");
                exit(2);
            }
        }
    }

    printf("Testing transcoding u-law -> A-law -> u-law\n");
    for (i = 0;  i < 256;  i++)
    {
        if (ulaw_to_alaw(alaw_to_ulaw(i)) != i)
        {
            if (abs(alaw_to_ulaw(ulaw_to_alaw(i)) - i) > 1)
            {
                printf("A-law -> u-law -> A-law gave %d -> %d\n", i, ulaw_to_alaw(alaw_to_ulaw(i)));
                printf("Test failed\n");
                exit(2);
            }
        }
    }
    
    if (afCloseFile(outhandle))
    {
        fprintf(stderr, "    Cannot close wave file '%s'\n", OUT_FILE_NAME);
        exit(2);
    }
    afFreeFileSetup(filesetup);

    printf("Tests passed.\n");
    return 0;
}
/*- End of function --------------------------------------------------------*/
/*- End of file ------------------------------------------------------------*/
