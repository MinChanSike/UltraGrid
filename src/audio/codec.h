/*
 * FILE:    audio/echo.h
 * AUTHORS: Martin Benes     <martinbenesh@gmail.com>
 *          Lukas Hejtmanek  <xhejtman@ics.muni.cz>
 *          Petr Holub       <hopet@ics.muni.cz>
 *          Milos Liska      <xliska@fi.muni.cz>
 *          Jiri Matela      <matela@ics.muni.cz>
 *          Dalibor Matura   <255899@mail.muni.cz>
 *          Ian Wesley-Smith <iwsmith@cct.lsu.edu>
 *
 * Copyright (c) 2005-2010 CESNET z.s.p.o.
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *
 *      This product includes software developed by CESNET z.s.p.o.
 *
 * 4. Neither the name of CESNET nor the names of its contributors may be used
 *    to endorse or promote products derived from this software without specific
 *    prior written permission.
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
 *
 *
 */

#ifndef AUDIO_CODEC_H_
#define AUDIO_CODEC_H_

#include "audio/audio.h"

#ifdef __cplusplus
extern "C" {
#endif

struct audio_codec {
        const audio_codec_t *supported_codecs;
        void *(*init)(audio_codec_t);
        audio_frame2 *(*compress)(void *, audio_frame2 *);
        audio_frame2 *(*decompress)(void *, audio_frame2 *);
        void (*done)(void *);
};

struct audio_codec_state;

audio_codec_t get_audio_codec_to_name(const char *name);
const char *get_name_to_audio_codec(audio_codec_t codec);
uint32_t get_audio_tag(audio_codec_t codec);
audio_codec_t get_audio_codec_to_tag(uint32_t audio_tag);

struct audio_codec_state *audio_codec_init(audio_codec_t audio_codec);
struct audio_codec_state *audio_codec_reconfigure(struct audio_codec_state *old, audio_codec_t audio_codec);
audio_frame2 *audio_codec_compress(struct audio_codec_state *, audio_frame2 *);
audio_frame2 *audio_codec_decompress(struct audio_codec_state *, audio_frame2 *);
void audio_codec_done(struct audio_codec_state *);

#ifdef __cplusplus
}
#endif

#endif /* AUDIO_CODEC_H */