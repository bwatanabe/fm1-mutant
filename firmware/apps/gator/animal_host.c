/* SPDX-License-Identifier: GPL-3.0-only
 * FM-1 Mutant ANIMAL adapter: plug the original JIANT five-mode DSP into
 * existing Felucca/Mutant track_t, MIDI and GATOR patch storage.
 * JIANT original animal_dsp.c unchanged, commit f2fd85865ae4910a85069e848dd9f92bf2645996
 */
#include "animal_tables.h"

/* JIANT's ANIMAL uses noise_inc() from its NOISE engine. Reuse the original
 * implementation verbatim rather than replacing its extended pitch range. */
static uint32_t noise_inc(int32_t p)
{
    uint32_t sh = 0, inc;
    while (p > 2047) { p -= 192; sh++; }
    inc = pitch_inc((uint32_t)clamp(p, 0, 2047));
    return sh && inc >= 0x80000000u >> (sh - 1u) ? 0xFFFFFFFFu : inc << sh;
}
static track_t an_tracks[1];
/* JIANT's TAG(FX) is workstation preset metadata, unused by Mutant. */
#define TAG(x) 0
#include "animal_dsp.c"
#undef TAG

static uint32_t an_age;

static void an_set_patch(const macro_patch_t *p)
{
    track_t *t = &an_tracks[0];
    t->p[P_E0] = (int16_t)clamp(p->harmonics * 4 / 1000, 0, 4); /* MODE */
    t->p[P_E1] = (int16_t)clamp(p->timbre * 127 / 1000, 0, 127); /* PRESS, TOUCH, etc */
    t->p[P_E2] = (int16_t)clamp(p->morph * 127 / 1000, 0, 127); /* CHAOS/COUPLE/Q/DRIFT */
    t->p[P_E3] = (int16_t)clamp((p->fm_amt + 100) * 127 / 200, 0, 127); /* SIZE/CAP */
    t->p[P_E4] = (int16_t)clamp(p->timbre_amt * 64 / 100, -64, 63); /* SWEEP/HYST */
    t->p[P_E5] = (int16_t)clamp((p->morph_amt + 100) * 127 / 200, 0, 127); /* RATE */
    t->p[P_E6] = (int16_t)clamp(p->decay * 15 / 1000, 0, 15); /* GROUP */
    /* For GLOTT/CMOS the original TEMP slot is AIR/FM, range 0..40.
       For other modes it is TEMPerature, 0..40, 22 nominal. */
    t->p[P_E7] = (int16_t)clamp(p->colour * 40 / 1000, 0, 40);
}

static void an_all_off(void)
{
    voice_t *v = &an_tracks[0].v[0];
    fm1_irq_off();
    v->gate = v->active = v->stage = 0;
    v->env = v->env_out = 0;
    fm1_irq_on();
}

static void an_note_on(uint8_t note, uint8_t velocity, const macro_patch_t *p)
{
    voice_t *v = &an_tracks[0].v[0];
    fm1_irq_off();
    an_set_patch(p);
    v->note = note;
    v->vel = velocity;
    v->gate = v->active = v->stage = 1;
    v->env = v->env_out = 0;
    v->fine = 0;
    v->pitch16 = v->pitch_cur = (int32_t)note * 16;
    v->age = ++an_age;
    ENG_ANIMAL.note_on(&an_tracks[0], v);
    fm1_irq_on();
}
static void an_note_off(uint8_t note)
{
    voice_t *v = &an_tracks[0].v[0];
    fm1_irq_off();
    if (v->active && v->gate && v->note == note) {
        v->gate = 0; v->stage = 3;
    }
    fm1_irq_on();
}
static int32_t an_note10(void)
{
    const voice_t *v = &an_tracks[0].v[0];
    return v->active ? (int32_t)v->note * 10 : 600;
}
static int an_gate(void)
{
    const voice_t *v = &an_tracks[0].v[0];
    return v->active && v->gate;
}
static void an_render(const macro_patch_t *p, const macro_global_t *g, int16_t bend,
                      int32_t *lr, uint32_t frames)
{
    track_t *t = &an_tracks[0];
    voice_t *v = &t->v[0];
    uint32_t pos = 0;
    an_set_patch(p);
    while (pos < frames) {
        uint32_t i, n = frames - pos > CTL ? CTL : frames - pos;
        int32_t mono[CTL] = {0};
        if (v->active) {
            vmod_t m = {0};
            int32_t amp = lo_env_tick(v);
            if (ENG_ANIMAL.amp)
                amp = ENG_ANIMAL.amp(t, v, amp);
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
                ENG_ANIMAL.render(t, v, mono, n, &m);
            }
        }
        for (i = 0; i < n; ++i) {
            const int32_t s = (int32_t)(((int64_t)mono[i] * g->volume) >> 8);
            lr[2u * (pos + i)] = s;
            lr[2u * (pos + i) + 1u] = s;
        }
        pos += n;
    }
}
