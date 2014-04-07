/**
 * @file   capture_filter/logo.c
 * @author Martin Pulec     <pulec@cesnet.cz>
 */
/*
 * Copyright (c) 2013 CESNET z.s.p.o.
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#include "config_unix.h"
#include "config_win32.h"
#endif /* HAVE_CONFIG_H */

#include <fstream>
#include <iostream>

#include "capture_filter.h"

#include "debug.h"

#include "video.h"
#include "video_codec.h"

struct state_capture_filter_logo {
        unsigned char *logo;
        unsigned int width, height;
        int x, y;
};

static int init(struct module *parent, const char *cfg, void **state);
static void done(void *state);
static struct video_frame *filter(void *state, struct video_frame *in);

using namespace std;

static bool load_logo_data_from_file(struct state_capture_filter_logo *s, const char *filename) {
        try {
                string line;
                ifstream file(filename, ifstream::in | ifstream::binary);

                file.exceptions(ifstream::failbit | ifstream::badbit );

                getline(file, line);
                if (!file.good() || line != "P7") {
                        throw string("Only logo in PAM format is currently supported.");
                }
                getline(file, line);
                while (!file.eof()) {
                        if (line.compare(0, strlen("WIDTH"), "WIDTH") == 0) {
                                s->width = atoi(line.c_str() + strlen("WIDTH "));
                        } else if (line.compare(0, strlen("HEIGHT"), "HEIGHT") == 0) {
                                s->height = atoi(line.c_str() + strlen("HEIGHT "));
                        } else if (line.compare(0, strlen("DEPTH"), "DEPTH") == 0) {
                                if (atoi(line.c_str() + strlen("DEPTH ")) != 4) {
                                        throw string("Only depth 4 is supported.");
                                }
                        } else if (line.compare(0, strlen("MAXVAL"), "MAXVAL") == 0) {
                                if (atoi(line.c_str() + strlen("MAXVAL ")) != 255) {
                                        throw string("Only supported maxval is 255.");
                                }
                        } else if (line.compare(0, strlen("TUPLETYPE"), "MAXVAL") == 0) {
                                if (line.compare("TUPLTYPE RGB_ALPHA") != 0) {
                                        throw string("Only supported tuple type is RGBA_APLHA.");
                                }
                        } else if (line.compare(0, strlen("ENDHDR"), "ENDHDR") == 0) {
                                break;
                        }
                        getline(file, line);
                }
                int datalen = 4 * s->width * s->height;
                s->logo = (unsigned char *) malloc(datalen);
                file.read((char *) s->logo, datalen);
                if (file.eof()) {
                        throw string("Unable to load logo data from file.");
                }
                file.close();
        } catch (string const & s) {
                cerr << s << endl;
                return false;
        } catch (exception const & e) {
                cerr << e.what() << endl;
                return false;
        } catch (...) {
                return false;
        }

        return true;
}

static int init(struct module *parent, const char *cfg, void **state)
{
        UNUSED(parent);
        struct state_capture_filter_logo *s = (struct state_capture_filter_logo *)
                calloc(1, sizeof(struct state_capture_filter_logo));

        s->x = s->y = -1;

        if (!cfg || strcasecmp(cfg, "help") == 0) {
                printf("Draws overlay logo over video:\n\n");
                printf("'logo' usage:\n");
                printf("\tlogo:<file>[:<x>[:<y>]]\n");
                printf("\t\t<file> - is path to logo to be added in PAM format with alpha\n");
                free(s);
                return 1;
        }
        char *tmp = strdup(cfg);
        char *save_ptr = NULL;
        char *item;
        if ((item = strtok_r(tmp, ":", &save_ptr)) == NULL) {
                fprintf(stderr, "File name with logo required!\n");
                return -1;
        }

        if (!load_logo_data_from_file(s, item)) {
                free(s);
                return -1;
        }

        if ((item = strtok_r(tmp, ":", &save_ptr))) {
                s->x = atoi(item);
                if ((item = strtok_r(NULL, ":", &save_ptr))) {
                        s->y = atoi(item);
                }
        }
        free(tmp);

        *state = s;
        return 0;
}

static void done(void *state)
{
        struct state_capture_filter_logo *s = (struct state_capture_filter_logo *)
                state;
        free(s->logo);
        free(state);
}

static struct video_frame *filter(void *state, struct video_frame *in)
{
        struct state_capture_filter_logo *s = (struct state_capture_filter_logo *)
                state;
        int linesize = s->width * 3;
        decoder_t decoder, coder;
        decoder = get_decoder_from_to(in->color_spec, RGB, true);
        coder = get_decoder_from_to(RGB, in->color_spec, true);
        int rect_x = s->x;
        int rect_y = s->y;

        if (decoder == NULL || coder == NULL)
                return in;

        if (rect_x < 0 || rect_x + s->width > in->tiles[0].width) {
                rect_x = in->tiles[0].width - s->width;
        }
        rect_x = (rect_x / get_pf_block_size(in->color_spec)) * get_pf_block_size(in->color_spec);

        if (rect_y < 0 || rect_y + s->height > in->tiles[0].height) {
                rect_y = in->tiles[0].height - s->height;
        }

        if (rect_x < 0 || rect_y < 0)
                return in;

        unsigned char *segment = (unsigned char *) malloc(s->width * s->height * 3);

        for (unsigned int y = 0; y < s->height; ++y) {
                decoder(segment + y * linesize, (unsigned char *) in->tiles[0].data + (y + rect_y) *
                                vc_get_linesize(in->tiles[0].width, in->color_spec) +
                                vc_get_linesize(rect_x, in->color_spec), linesize,
                                0, 8, 16);
        }

        unsigned char *image_data = segment;
        const unsigned char *overlay_data = s->logo;
        for (unsigned int y = 0; y < s->height; ++y) {
                for (unsigned int x = 0; x < s->width; ++x) {
                        int alpha = overlay_data[3];
                        for (int i = 0; i < 3; ++i) {
                                *image_data = (*image_data * (255 - alpha) + *overlay_data++ * alpha) / 255;
                                image_data++;
                        }
                        overlay_data++; // skip alpha
                }
        }

        for (unsigned int y = 0; y < s->height; ++y) {
                coder((unsigned char *) in->tiles[0].data + (rect_y + y) *
                                vc_get_linesize(in->tiles[0].width, in->color_spec) +
                                vc_get_linesize(rect_x, in->color_spec),
                                segment + y * linesize,
                                vc_get_linesize(s->width, in->color_spec), 0, 8, 16);
        }

        free(segment);

        return in;
}

struct capture_filter_info capture_filter_logo = {
        .name = "logo",
        .init = init,
        .done = done,
        .filter = filter,
};
