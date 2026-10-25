/* SPDX-License-Identifier: GPL-3.0-only
 * Adapter for JIANT's original vowel/glottis/resonator DSP.
 * Reuses track_t, voice_t, lo_env_tick, fe_tables and shared PCM helpers
 * already imported for Felucca LOFI. No new oscillator/filter algorithms.
 */
static track_t vo_tracks[1];
#include "voice_dsp.c"
static uint32_t vo_age;

static void vo_set_patch(const macro_patch_t *p)
{
    track_t *t = &vo_tracks[0];
    t->p[P_E0] = (int16_t)(p->harmonics * 127 / 1000);        /* VOWEL */
    t->p[P_E1] = (int16_t)(p->timbre * 127 / 1000);          /* VOWEL2 */
    t->p[P_E2] = (int16_t)(p->morph * 127 / 1000);           /* TALK */
    t->p[P_E3] = (int16_t)((p->fm_amt * 12) / 100);          /* SHIFT, +/-12 semitones */
    t->p[P_E4] = (int16_t)((p->timbre_amt + 100) * 127 / 200); /* BUZZ */
    t->p[P_E5] = (int16_t)((p->morph_amt + 100) * 127 / 200);  /* BREATH */
    t->p[P_E6] = (int16_t)(p->decay * 127 / 1000);          /* Q */
    t->p[P_E7] = (int16_t)(p->colour * 127 / 1000);         /* RAND */
}
static void vo_all_off(void)
{
    voice_t *v = &vo_tracks[0].v[0];
    fm1_irq_off();
    v->gate = v->active = v->stage = 0;
    v->env = v->env_out = 0;
    fm1_irq_on();
}
static void vo_note_on(uint8_t note, uint8_t vel, const macro_patch_t *p)
{
    voice_t *v = &vo_tracks[0].v[0];
    fm1_irq_off();
    vo_set_patch(p);
    v->note = note; v->vel = vel;
    v->gate = v->active = v->stage = 1;
    v->env = v->env_out = 0;
    v->pitch16 = v->pitch_cur = (int32_t)note * 16;
    v->age = ++vo_age;
    formant_note_on(&vo_tracks[0], v);
    fm1_irq_on();
}
static void vo_note_off(uint8_t note)
{
    voice_t *v = &vo_tracks[0].v[0];
    fm1_irq_off();
    if (v->active && v->gate && v->note == note) {
        v->gate = 0;
        v->stage = 3;
    }
    fm1_irq_on();
}
static int32_t vo_current_note10(void)
{
    const voice_t *v = &vo_tracks[0].v[0];
    return v->active ? (int32_t)v->note * 10 : 600;
}
static int vo_current_gate(void)
{
    const voice_t *v = &vo_tracks[0].v[0];
    return v->active && v->gate;
}
static void vo_render(const macro_patch_t *p, const macro_global_t *g, int16_t bend,
                      int32_t *lr, uint32_t frames)
{
    track_t *t = &vo_tracks[0];
    voice_t *v = &t->v[0];
    uint32_t pos = 0;
    vo_set_patch(p);
    while (pos < frames) {
        const uint32_t n = frames - pos > CTL ? CTL : frames - pos;
        int32_t mono[CTL] = {0};
        uint32_t i;
        if (v->active) {
            vmod_t m = {0};
            int32_t amp = lo_env_tick(v);  /* same stable monophonic gate as LOFI */
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
                v->pitch_cur = v->pitch16 = m.pitch16; /* no sung glide in vowel-only mode */
                formant_render(t, v, mono, n, &m);
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
