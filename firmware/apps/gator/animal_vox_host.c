/* SPDX-License-Identifier: GPL-3.0-only
 * FM-1 Mutant v0.8 experimental ANIMAL VOX:
 * unmodified JIANT ANIMAL engine -> existing JIANT four-formant cascade.
 *
 * The source is ENG_ANIMAL.render() in animal_dsp.c (JIANT original).
 * The filter is hy_filter_sample_state() / hy_coefficients_at_pitch()
 * previously used by CHIP VOX; not a reimplementation of either DSP.
 * Source revision f2fd85865ae4910a85069e848dd9f92bf2645996.
 *
 * GATOR's persistent patch struct is unchanged. The temporary patch maps:
 * MAIN: MODE, CALL, VOWEL / transpose
 * SCL : SHIFT, Q, CHAOS
 * FX  : WET, RATE, GROUP, TEMP/AIR/FM
 */
static mutant_hybrid_filter_t av_filter;

/* Preserve all original ANIMAL mode algorithms and their existing host.
 * This is only an ephemeral parameter remapping for the new model:
 * SIZE/CAP defaults to 64 and SWEEP/HYST defaults to 0 so they do not
 * compete with the crucial formant SHIFT and Q controls in a 3-knob page. */
static macro_patch_t av_animal_source(const macro_patch_t *p)
{
    macro_patch_t q = *p;
    q.morph = (int16_t)lo_bound((p->morph_amt + 100) * 1000 / 200, 0, 1000);
    q.fm_amt = 0;                                  /* animal SIZE = 64 */
    q.timbre_amt = 0;                              /* animal SWEEP = 0 */
    q.morph_amt = (int16_t)(lo_bound(p->colour, 0, 1000) * 200 / 1000 - 100);
    q.decay = (int16_t)(lo_bound(p->blend, 0, 100) * 10);
    q.colour = (int16_t)(lo_bound(p->spread, 0, 100) * 10);
    return q;
}

static void av_all_off(void)
{
    uint32_t i;
    fm1_irq_off();
    for (i = 0; i < 8; ++i)
        av_filter.y[i] = 0;
    fm1_irq_on();
}

static void av_note_on(uint8_t note, uint8_t velocity, const macro_patch_t *p)
{
    const macro_patch_t source = av_animal_source(p);
    av_all_off();
    an_note_on(note, velocity, &source);
}
static void av_note_off(uint8_t note) { an_note_off(note); }
static int32_t av_note10(void) { return an_note10(); }
static int av_gate(void) { return an_gate(); }

static void av_render(const macro_patch_t *p, const macro_global_t *g, int16_t bend,
                      int32_t *lr, uint32_t frames)
{
    const macro_patch_t source = av_animal_source(p);
    macro_global_t full_volume = *g;
    fres_t resonators[4];
    int32_t gain, gaink;
    uint32_t pos = 0;
    const int32_t wet = lo_bound(p->decay, 0, 1000);
    full_volume.volume = 256;  /* apply master gain once, after wet/dry mix */

    while (pos < frames) {
        const uint32_t n = frames - pos > CTL ? CTL : frames - pos;
        const voice_t *v = &an_tracks[0].v[0];
        int32_t creature[2 * CTL];
        uint32_t i;
        /* Same coefficient math as CHIP VOX; independent resonator state.
         * Track MIDI/transpose/pitch-bend consistently with the source. */
        int32_t note16 = (v->active ? (int32_t)v->note : 60) * 16 + p->transpose * 16;
        note16 += (int32_t)bend * g->bend_range * 16 / 8192;
        note16 += (int32_t)g->fine * 16 / 100;
        hy_coefficients_at_pitch(p, (uint32_t)lo_bound(p->morph, 0, 1000) * 127 / 1000,
                                 lo_bound(note16, 0, 2047), resonators, &gain, &gaink);
        an_render(&source, &full_volume, bend, creature, n);
        for (i = 0; i < n; ++i) {
            const int32_t dry = creature[2 * i];
            const int32_t voice = hy_filter_sample_state(&av_filter, dry, resonators, gain, gaink);
            const int32_t mixed = (int32_t)(((int64_t)dry * (1000 - wet) +
                                             (int64_t)voice * wet) / 1000);
            const int32_t sample = lo_bound((int32_t)(((int64_t)mixed * g->volume) >> 8),
                                             -32768, 32767);
            lr[2u * (pos + i)] = sample;
            lr[2u * (pos + i) + 1u] = sample;
        }
        pos += n;
    }
}
