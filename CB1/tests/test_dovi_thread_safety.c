/* SPDX-License-Identifier: GPL-2.0-only */
#include "dovi_engine.h"
#include <assert.h>
#include <stdio.h>
#include <pthread.h>
#include <stdbool.h>

#define NUM_THREADS 4
#define ITERATIONS_PER_THREAD 20

static void *worker_thread(void *arg)
{
    (void)arg;
    for (int iter = 0; iter < ITERATIONS_PER_THREAD; ++iter) {
        struct cb1_dovi_instance *inst = cb1_dovi_create();
        assert(inst != NULL);

        /* 1. Tone curve LUT generation */
        float lut[4096];
        bool ok = cb1_dovi_generate_tone_lut_fp32(62, 3696, 62, 3079, 0, 0, 0, lut);
        assert(ok);
        assert(lut[0] >= 0.0f && lut[4095] <= 1.0f);

        /* 2. Target VSVDB parsing */
        uint8_t vsvdb[32] = {
            0xeb, 0x01, 0x46, 0xd0, 0x00, 0x20, 0x20, 0x05, 0
        };
        struct cb1_dovi_target_desc target;
        int ret = cb1_dovi_vsvdb_parse(vsvdb, &target);
        assert(ret == 0);

        /* 3. Thread-local trim reduction */
        uint8_t packed_metadata[1024] = {0};
        packed_metadata[70] = 1;
        uint8_t *rec0 = packed_metadata + 71;
        rec0[4] = 2;
        rec0[5] = (uint8_t)(3079 & 0xFF);
        rec0[6] = (uint8_t)((3079 >> 8) & 0xFF);

        int red_ret = cb1_dovi_reduce_trims(target.raw_target, packed_metadata);
        assert(red_ret >= 0);

        cb1_dovi_destroy(inst);
    }
    return NULL;
}

int main(void)
{
    pthread_t threads[NUM_THREADS];
    for (int i = 0; i < NUM_THREADS; ++i) {
        int err = pthread_create(&threads[i], NULL, worker_thread, NULL);
        assert(err == 0);
    }
    for (int i = 0; i < NUM_THREADS; ++i) {
        int err = pthread_join(threads[i], NULL);
        assert(err == 0);
    }
    printf("test_dovi_thread_safety: PASSED (%d threads x %d iterations clean)\n",
           NUM_THREADS, ITERATIONS_PER_THREAD);
    return 0;
}
