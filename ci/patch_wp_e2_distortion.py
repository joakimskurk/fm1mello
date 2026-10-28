#!/usr/bin/env python3
from pathlib import Path
import hashlib

AUDIO = Path('mello/mello_audio.c')
HEADER = Path('mello/mello_audio.h')
PRE_AUDIO_SHA = '3bfc64f6b101a8123b1533014fece746073bfac22055b83434252d8e0252ff85'
PRE_HEADER_SHA = '302e217d551d1f250269ca37216217cffd067656a7db371ad1dc91e7ec7c61b5'
POST_AUDIO_SHA = 'd0bea0a0dd777ff27f615d6cc5f3b7330680a4e1bdddea841ca3b49139ba8e47'
POST_HEADER_SHA = '463e403fa7f5c927bfc69be48b6f0b18d475f21fe9ac45f7e6c285eaf7e0b63d'


def sha(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def replace_once(text: str, old: str, new: str, label: str) -> str:
    count = text.count(old)
    if count != 1:
        raise SystemExit(f'FAIL: WP-E2 {label}: expected one source anchor, found {count}')
    return text.replace(old, new, 1)


if sha(AUDIO) != PRE_AUDIO_SHA:
    raise SystemExit(f'FAIL: WP-E2 audio base hash {sha(AUDIO)} != {PRE_AUDIO_SHA}')
if sha(HEADER) != PRE_HEADER_SHA:
    raise SystemExit(f'FAIL: WP-E2 header base hash {sha(HEADER)} != {PRE_HEADER_SHA}')

audio = AUDIO.read_text()
header = HEADER.read_text()

# Keep the sample-slot soft_drive() unchanged. E2 deliberately replaces only the
# global distortion path so sample-slot distortion does not move as collateral.
legacy_soft = 'static int32_t soft_drive(int32_t x,uint16_t amount,uint16_t character){int32_t gain=32768+(int32_t)amount*(12+(int32_t)character/40);if(gain>65535)gain=65535;x=clamp32(x,-32767,32767);int32_t y=q15mul(x,gain);int32_t lim=32767-(int32_t)amount*8;if(lim<12000)lim=12000;if(y>lim)y=lim+((y-lim)>>3);if(y<-lim)y=-lim+((y+lim)>>3);return clamp32(y,-32767,32767);}'
if audio.count(legacy_soft) != 1:
    raise SystemExit('FAIL: WP-E2 legacy sample-slot soft_drive anchor changed')

audio = replace_once(
    audio,
    '''static uint16_t browser_dist_drive(uint16_t amount,uint16_t character){
    uint32_t a=amount>1000u?1000u:amount,c=character>1000u?1000u:character;
    uint32_t a2=(a*a+500u)/1000u;
    uint32_t a19=a2+(a-a2+5u)/10u;
    uint32_t factor=280u+(c*520u+500u)/1000u;
    uint32_t drive=(a19*factor+500u)/1000u;
    return (uint16_t)(drive>800u?800u:drive);
}
''',
    '''static uint16_t global_dist_drive(uint16_t amount){
    uint32_t a=amount>1000u?1000u:amount;
    /* Cubic AMT curve: preserves fine low/mid control but opens into the full E2 range. */
    return (uint16_t)(((a*a)*a+500000u)/1000000u);
}
''',
    'amount-to-drive mapping',
)

audio = replace_once(
    audio,
    'if(dist_changed||a->applied_generation==0u)a->dist_drive_permille=browser_dist_drive(a->fx.dist_amount,a->fx.dist_character);',
    'if(dist_changed||a->applied_generation==0u)a->dist_drive_permille=global_dist_drive(a->fx.dist_amount);',
    'derived-drive publication',
)

global_stage = '''\nstatic int32_t global_distort(int32_t x,uint16_t drive,uint16_t character){
    uint32_t d=drive>1000u?1000u:drive,c=character>1000u?1000u:character;
    int32_t gain_q12,lim,slope_q12,y;
    x=clamp32(x,-32767,32767);
    if(d==0u)return x;
    /* 1.0x..7.84x pre-gain in Q12. x*gain remains safely inside int32_t. */
    gain_q12=4096+(int32_t)d*28;
    y=(x*gain_q12)>>12;
    /* CHAR is an independent transfer-shape control: higher values move the knee
     * inward and flatten the post-knee slope without increasing AMT drive. */
    lim=28672-((int32_t)c<<3);              /* 0.875FS .. ~0.631FS */
    slope_q12=1536-(((int32_t)c*5)>>2);    /* 0.375 .. ~0.070 */
    if(y>lim)y=lim+(((y-lim)*slope_q12)>>12);
    if(y<-lim)y=-lim+(((y+lim)*slope_q12)>>12);
    return clamp32(y,-32767,32767);
}
#if defined(MELLO_DISTORTION_TEST_HOOK) && MELLO_DISTORTION_TEST_HOOK
uint16_t mello_audio_test_global_dist_drive(uint16_t amount){return global_dist_drive(amount);}
int32_t mello_audio_test_global_distort(int32_t x,uint16_t amount,uint16_t character){return global_distort(x,global_dist_drive(amount),character);}
#endif'''

audio = replace_once(audio, legacy_soft, legacy_soft + global_stage, 'global waveshaper insertion')

audio = replace_once(
    audio,
    'if(a->dist_drive_permille)wet=soft_drive(mix,a->dist_drive_permille,0u);',
    'if(a->dist_drive_permille)wet=global_distort(mix,a->dist_drive_permille,a->fx.dist_character);',
    'global render call',
)

header = replace_once(
    header,
    '''bool mello_audio_is_silent(const MelloAudio *a);

#ifdef __cplusplus
''',
    '''bool mello_audio_is_silent(const MelloAudio *a);
#if defined(MELLO_DISTORTION_TEST_HOOK) && MELLO_DISTORTION_TEST_HOOK
uint16_t mello_audio_test_global_dist_drive(uint16_t amount);
int32_t mello_audio_test_global_distort(int32_t x, uint16_t amount, uint16_t character);
#endif

#ifdef __cplusplus
''',
    'test-hook declarations',
)

AUDIO.write_text(audio)
HEADER.write_text(header)

if sha(AUDIO) != POST_AUDIO_SHA:
    raise SystemExit(f'FAIL: WP-E2 audio output hash {sha(AUDIO)} != {POST_AUDIO_SHA}')
if sha(HEADER) != POST_HEADER_SHA:
    raise SystemExit(f'FAIL: WP-E2 header output hash {sha(HEADER)} != {POST_HEADER_SHA}')
if AUDIO.read_text().count(legacy_soft) != 1:
    raise SystemExit('FAIL: WP-E2 changed legacy sample-slot soft_drive')

print(f'PASS: WP-E2 global distortion audio locked: {POST_AUDIO_SHA}')
print(f'PASS: WP-E2 distortion test-hook header locked: {POST_HEADER_SHA}')
