/* SPDX-License-Identifier: GPL-3.0-only
 * Small bridge from GATOR's MIDI/patch/audio interface to upstream Felucca-Engines LOFI.
 * Donor DSP source files under ../mutant are unmodified MIT-licensed code.
 * No new oscillator or filter implementations live here.
 */
#define NVOICE 1
#define NPART 1
#include "fe_tables.h"
#include "../mutant/felucca_engine.h"
enum { P_ATK, P_REL, P_FTYPE, P_E0, P_E1, P_E2, P_E3, P_E4, P_E5, P_E6, P_E7, P_COUNT };
typedef struct track { int16_t p[P_COUNT]; voice_t v[NVOICE]; } track_t;
static track_t lo_tracks[NPART];
#define trk lo_tracks
#include "../mutant/dsp.c"
#include "../mutant/eng_lofi.c"
#undef trk

static uint32_t lo_age;

static int32_t lo_bound(int32_t v, int32_t a, int32_t b)
{
    return v < a ? a : v > b ? b : v;
}

/* Reuse the existing persistent GATOR patch values, unchanged on disk.
 * Main: HARM=CHIP, TIMBRE=WAVE, MORPH=DUTY
 * ATTN: FM=CRSH, TIMBRE=SWP, MORPH=VIB
 * LPG: DECAY=ARP, COLOUR=TONE
 */
static void lo_set_patch(const macro_patch_t *p)
{
    track_t *t = &lo_tracks[0];
    t->p[P_E0] = (int16_t)(p->harmonics * 4 / 1000);
    t->p[P_E1] = (int16_t)(p->timbre * 4 / 1000);
    t->p[P_E2] = (int16_t)(p->morph * 127 / 1000);
    t->p[P_E3] = (int16_t)((p->fm_amt + 100) * 127 / 200);
    t->p[P_E4] = (int16_t)((p->timbre_amt + 100) * 127 / 200);
    t->p[P_E5] = (int16_t)((p->morph_amt + 100) * 127 / 200);
    t->p[P_E6] = (int16_t)(p->decay * 3 / 1000);
    t->p[P_E7] = (int16_t)(p->colour * 127 / 1000);
    t->p[P_ATK] = (int16_t)(p->env_a * 127 / 1000);
    t->p[P_REL] = (int16_t)(p->env_r * 127 / 1000);
}

static int32_t lo_current_note10(void)
{
    voice_t *v = &lo_tracks[0].v[0];
    return v->active ? (int32_t)v->note * 10 : 600;
}
static int lo_current_gate(void)
{
    voice_t *v = &lo_tracks[0].v[0];
    return v->active && v->gate;
}

static void lo_all_off(void)
{
    voice_t *v = &lo_tracks[0].v[0];
    fm1_irq_off();
    v->gate = v->active = v->stage = 0;
    v->env = v->env_out = 0;
    fm1_irq_on();
}

static void lo_note_on(uint8_t note, uint8_t velocity, const macro_patch_t *p)
{
    voice_t *v = &lo_tracks[0].v[0];
    fm1_irq_off();
    lo_set_patch(p);
    v->note = note;
    v->vel = velocity;
    v->gate = 1;
    v->active = 1;
    v->stage = 1;
    v->env = v->env_out = 0;
    v->pitch16 = v->pitch_cur = (int32_t)note * 16;
    v->age = ++lo_age;
    ENG_LOFI.note_on(&lo_tracks[0], v);
    fm1_irq_on();
}

static void lo_note_off(uint8_t note)
{
    voice_t *v = &lo_tracks[0].v[0];
    fm1_irq_off();
    if (v->active && v->gate && v->note == note) {
        v->gate = 0;
        v->stage = 3;
    }
    fm1_irq_on();
}

static int32_t lo_env_tick(voice_t *v)
{
    if (!v->active) return 0;
    if (v->stage == 1) {
        v->env += 1 << 22;
        if (v->env >= (1 << 24)) {
            v->env = 1 << 24;
            v->stage = 2;
        }
    } else if (v->stage == 2) {
        v->env -= (v->env - (3 << 22)) >> 3;
    } else if (v->stage == 3) {
        v->env -= v->env >> 4;
        if (v->env < (1 << 24) / 1000) {
            v->env = 0;
            v->stage = 0;
            v->active = 0;
        }
    }
    return v->env >> 9;
}

static void lo_render(const macro_patch_t *p, const macro_global_t *g, int16_t bend,
                      int32_t *lr, uint32_t frames)
{
    track_t *t = &lo_tracks[0];
    voice_t *v = &t->v[0];
    uint32_t pos = 0;
    lo_set_patch(p);
    while (pos < frames) {
        uint32_t i, n = frames - pos > CTL ? CTL : frames - pos;
        int32_t mono[CTL] = {0};
        if (v->active) {
            vmod_t m = {0};
            int32_t amp = lo_env_tick(v);
            if (ENG_LOFI.amp) amp = ENG_LOFI.amp(t, v, amp);
            if (v->active) {
                int32_t pitch = ((int32_t)v->note + p->transpose) * 16;
                pitch += (int32_t)bend * g->bend_range * 16 / 8192;
                pitch += (int32_t)g->fine * 16 / 100;
                m.pitch16 = lo_bound(pitch, 0, 2047);
                m.inc = pitch_inc((uint32_t)m.pitch16);
                m.amp0 = v->env_out * v->vel / 127;
                m.amp1 = amp * v->vel / 127;
                m.envq15 = amp;
                v->env_out = amp;
                ENG_LOFI.render(t, v, mono, n, &m);
            }
        }
        for (i = 0; i < n; ++i) {
            int32_t s = (int32_t)(((int64_t)mono[i] * g->volume) >> 8);
            lr[2u * (pos + i)] = s;
            lr[2u * (pos + i) + 1u] = s;
        }
        pos += n;
    }
}
