/**
 * @file   utils/video_frame_pool.h
 * @author Martin Pulec     <pulec@cesnet.cz>
 */
/*
 * Copyright (c) 2014 CESNET z.s.p.o.
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

#ifndef VIDEO_FRAME_POOL_H_
#define VIDEO_FRAME_POOL_H_

#include "config.h"
#include "config_unix.h"
#include "config_win32.h"

#include "debug.h"

#include "video.h"

#ifdef __cplusplus
#include "utils/lock_guard.h"

#include <iostream>
#include <queue>
#include <stdexcept>

struct default_data_allocator {
        void *allocate(size_t size) {
                return malloc(size);
        }
        void deallocate(void *ptr) {
                free(ptr);
        }
};

struct auto_video_frame_disposer {
        auto_video_frame_disposer(struct video_frame *frame) :
                m_frame(frame) {
        }

        ~auto_video_frame_disposer() {
                VIDEO_FRAME_DISPOSE(m_frame);
        }

        struct video_frame *m_frame;
};

template <typename allocator>
struct video_frame_pool {
        struct vf_udata {
                vf_udata(struct video_frame_pool<allocator> &pool,
                                int generation) : m_pool(pool), m_generation(generation) {
                }

                struct video_frame_pool<allocator> &m_pool;
                int                            m_generation;
        };

        public:
                video_frame_pool() : m_generation(0), m_desc{}, m_max_data_len(0) {
                        pthread_mutex_init(&m_lock, NULL);
                }

                virtual ~video_frame_pool() {
                        remove_free_frames();
                        pthread_mutex_destroy(&m_lock);
                }

                void reconfigure(struct video_desc new_desc, size_t new_size) {
                        lock_guard guard(m_lock);
                        m_desc = new_desc;
                        m_max_data_len = new_size;
                        remove_free_frames();
                        m_generation++;
                }

                struct video_frame *get_frame() {
                        assert(m_generation != 0);
                        struct video_frame *ret = NULL;
                        lock_guard guard(m_lock);
                        if (!m_free_frames.empty()) {
                                ret = m_free_frames.front();
                                m_free_frames.pop();
                        } else {
                                try {
                                        ret = vf_alloc_desc(m_desc);
                                        for (unsigned int i = 0; i < m_desc.tile_count; ++i) {
                                                ret->tiles[i].data = (char *)
                                                        m_allocator.allocate(m_max_data_len);
                                                if (ret->tiles[i].data == NULL) {
                                                        throw std::runtime_error("Cannot allocate data");
                                                }
                                                ret->tiles[i].data_len = m_max_data_len;
                                        }
                                        ret->dispose_udata = new vf_udata(*this, m_generation);
                                        ret->dispose = video_frame_pool<allocator>::dispose;
                                } catch (std::exception &e) {
                                        std::cerr << e.what() << std::endl;
                                        deallocate_frame(ret);
                                        throw e;
                                }
                        }
                        return ret;
                }

                static void dispose(struct video_frame *frame) {
                        struct vf_udata *udata = (struct vf_udata *) frame->dispose_udata;
                        udata->m_pool.put_frame(frame);
                }

                void put_frame(struct video_frame *frame) {
                        struct vf_udata *udata = (struct vf_udata *) frame->dispose_udata;

                        lock_guard guard(m_lock);

                        if (udata->m_generation != m_generation) {
                                deallocate_frame(frame);
                        } else {
                                m_free_frames.push(frame);
                        }
                }

                allocator & get_allocator() {
                        return m_allocator;
                }

        private:
                void remove_free_frames() {
                        while (!m_free_frames.empty()) {
                                struct video_frame *frame = m_free_frames.front();
                                m_free_frames.pop();
                                deallocate_frame(frame);
                        }
                }

                void deallocate_frame(struct video_frame *frame) {
                        if (frame == NULL)
                                return;
                        struct vf_udata *udata = (struct vf_udata *) frame->dispose_udata;
                        for (unsigned int i = 0; i < frame->tile_count; ++i) {
                                m_allocator.deallocate(frame->tiles[i].data);
                        }
                        delete udata;
                        vf_free(frame);
                }

                std::queue<struct video_frame *> m_free_frames;
                pthread_mutex_t   m_lock;
                int               m_generation;
                struct video_desc m_desc;
                size_t            m_max_data_len;
                allocator         m_allocator;
};
#endif //  __cplusplus

#endif // VIDEO_FRAME_POOL_H_

