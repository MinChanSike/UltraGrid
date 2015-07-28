/**
 * @file   audio/playback/audio_playback.c
 * @author Martin Pulec     <pulec@cesnet.cz>
 */
/*
 * Copyright (c) 2015 CESNET, z. s. p. o.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, is permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * 3. Neither the name of CESNET nor the names of its contributors may be
 *    used to endorse or promote products derived from this software without
 *    specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHORS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESSED OR IMPLIED WARRANTIES, INCLUDING,
 * BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
 * EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#ifdef HAVE_CONFIG_H
#include "config.h"
#include "config_unix.h"
#include "config_win32.h"
#endif
#include "debug.h"
#include "host.h"

#include "audio/audio.h"
#include "audio/audio_playback.h"
#include "audio/playback/coreaudio.h"
#include "audio/playback/dummy.h"
#include "audio/playback/none.h"
#include "audio/playback/sdi.h"

#include "lib_common.h"

#include "video_display.h" /* flags */

struct state_audio_playback {
        char name[128];
        const struct audio_playback_info *funcs;
        void *state;
};

static void init_static_aplay() __attribute__((constructor));
static void init_static_aplay() {
#ifndef UV_IN_YURI
        register_library("embedded", &aplay_sdi_info, LIBRARY_CLASS_AUDIO_PLAYBACK, AUDIO_PLAYBACK_ABI_VERSION);
        register_library("AESEBU", &aplay_sdi_info, LIBRARY_CLASS_AUDIO_PLAYBACK, AUDIO_PLAYBACK_ABI_VERSION);
        register_library("analog", &aplay_sdi_info, LIBRARY_CLASS_AUDIO_PLAYBACK, AUDIO_PLAYBACK_ABI_VERSION);
#if defined HAVE_COREAUDIO
        register_library("coreaudio", &aplay_coreaudio_info, LIBRARY_CLASS_AUDIO_PLAYBACK, AUDIO_PLAYBACK_ABI_VERSION);
#endif
        register_library("dummy", &aplay_dummy_info, LIBRARY_CLASS_AUDIO_PLAYBACK, AUDIO_PLAYBACK_ABI_VERSION);
#endif
        register_library("none", &aplay_none_info, LIBRARY_CLASS_AUDIO_PLAYBACK, AUDIO_PLAYBACK_ABI_VERSION);
};

void audio_playback_help()
{
        printf("Available audio playback devices:\n");
        list_modules(LIBRARY_CLASS_AUDIO_PLAYBACK, AUDIO_PLAYBACK_ABI_VERSION);
}

int audio_playback_init(const char *device, const char *cfg, struct state_audio_playback **state)
{
        struct state_audio_playback *s;

        s = calloc(1, sizeof(struct state_audio_playback));
        s->funcs = load_library(device, LIBRARY_CLASS_AUDIO_PLAYBACK, AUDIO_PLAYBACK_ABI_VERSION);

        if (s->funcs == NULL) {
                log_msg(LOG_LEVEL_ERROR, "Unknown audio playback driver: %s\n", device);
                goto error;
        }

        strncpy(s->name, device, sizeof s->name - 1);
        s->state = s->funcs->init(cfg);

        if(!s->state) {
                goto error;
        }

        if(s->state == &audio_init_state_ok) {
                free(s);
                return 1;
        }

        *state = s;
        return 0;

error:
        free(s);
        return -1;
}

struct state_audio_playback *audio_playback_init_null_device(void)
{
        struct state_audio_playback *device;
        int ret = audio_playback_init("none", NULL, &device);
        assert(ret == 0);

        return device;
}

void audio_playback_done(struct state_audio_playback *s)
{
        if(s) {
                s->funcs->done(s->state);
                free(s);
        }
}

unsigned int audio_playback_get_display_flags(struct state_audio_playback *s)
{
        if(!s)
                return 0u;

        if (strcasecmp(s->name, "embedded") == 0) {
                return DISPLAY_FLAG_AUDIO_EMBEDDED;
        } else if (strcasecmp(s->name, "AESEBU") == 0) {
                return DISPLAY_FLAG_AUDIO_AESEBU;
        } else if (strcasecmp(s->name, "analog") == 0) {
                return DISPLAY_FLAG_AUDIO_ANALOG;
        } else  {
                return 0u;
        }
}

void audio_playback_put_frame(struct state_audio_playback *s, struct audio_frame *frame)
{
        s->funcs->write(s->state, frame);
}

int audio_playback_reconfigure(struct state_audio_playback *s, int quant_samples, int channels,
                int sample_rate)
{
        return s->funcs->reconfigure(s->state, quant_samples, channels, sample_rate);
}

void  *audio_playback_get_state_pointer(struct state_audio_playback *s)
{
        return s->state;
}

/* vim: set expandtab: sw=8 */

