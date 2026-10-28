#include "mello_audio.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>

#if !defined(MELLO_DISTORTION_TEST_HOOK) || !MELLO_DISTORTION_TEST_HOOK
#error "WP-E2 smoke requires MELLO_DISTORTION_TEST_HOOK=1"
#endif

static void bounded_and_symmetric(void)
{
    static const uint16_t amounts[] = {0u, 50u, 100u, 250u, 500u, 750u, 1000u};
    static const uint16_t chars[] = {0u, 250u, 500u, 750u, 1000u};
    unsigned ai, ci;
    int32_t x;
    for (ai = 0u; ai < sizeof(amounts) / sizeof(amounts[0]); ++ai) {
        for (ci = 0u; ci < sizeof(chars) / sizeof(chars[0]); ++ci) {
            int32_t prev = -32767;
            for (x = -32767; x <= 32767; x += 257) {
                int32_t y = mello_audio_test_global_distort(x, amounts[ai], chars[ci]);
                int32_t yn = mello_audio_test_global_distort(-x, amounts[ai], chars[ci]);
                assert(y >= -32767 && y <= 32767);
                assert(y >= prev); /* no foldback / transfer reversal */
                assert(yn == -y);  /* symmetric => no static DC offset */
                prev = y;
            }
        }
    }
}

int main(void)
{
    /* Cubic AMT preserves the old ~12.5% midpoint drive while opening the top end. */
    assert(mello_audio_test_global_dist_drive(0u) == 0u);
    assert(mello_audio_test_global_dist_drive(50u) == 0u);
    assert(mello_audio_test_global_dist_drive(100u) == 1u);
    assert(mello_audio_test_global_dist_drive(250u) == 16u);
    assert(mello_audio_test_global_dist_drive(500u) == 125u);
    assert(mello_audio_test_global_dist_drive(750u) == 422u);
    assert(mello_audio_test_global_dist_drive(1000u) == 1000u);

    /* Zero AMT is exact identity regardless of CHAR. */
    assert(mello_audio_test_global_distort(12345, 0u, 0u) == 12345);
    assert(mello_audio_test_global_distort(12345, 0u, 1000u) == 12345);
    assert(mello_audio_test_global_distort(-12345, 0u, 1000u) == -12345);

    /* Low and midpoint settings stay graduated rather than jumping into clipping. */
    assert(mello_audio_test_global_distort(6000, 100u, 350u) == 6041);
    assert(mello_audio_test_global_distort(6000, 250u, 350u) == 6656);
    assert(mello_audio_test_global_distort(6000, 500u, 350u) == 11126);
    assert(mello_audio_test_global_distort(6000, 750u, 350u) == 23308);

    /* The top end is genuinely extreme: max AMT can saturate a modest input. */
    assert(mello_audio_test_global_distort(6000, 1000u, 0u) == 32767);

    /* CHAR changes transfer shape at fixed AMT instead of secretly changing drive. */
    assert(mello_audio_test_global_distort(6000, 1000u, 350u) == 31544);
    assert(mello_audio_test_global_distort(6000, 1000u, 1000u) == 22511);
    assert(mello_audio_test_global_distort(12000, 500u, 0u) == 22253);
    assert(mello_audio_test_global_distort(12000, 500u, 1000u) == 20782);

    bounded_and_symmetric();

    puts("PASS: WP-E2 extreme bounded global distortion contract");
    return 0;
}
