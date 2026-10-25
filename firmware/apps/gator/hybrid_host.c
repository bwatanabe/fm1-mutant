/* SPDX-License-Identifier: GPL-3.0-only
 * HYBRID: unchanged Felucca LOFI oscillator -> JIANT Klatt-style vowel filter.
 * Audio source: lofi_host.c's original ENG_LOFI.render().
 * Resonators: the four Q30 cascade stages from JIANT's eng_formant.c,
 * using its unchanged vowel_at(), fmt_ratio(), fres_coef() and soft_knee().
 *
 * No sequencer, no resynthesis of the donor engines. Adapter only.
 * JIANT source: f2fd85865ae4910a85069e848dd9f92bf2645996 GPL-3.0-only
 * Felucca source: 04ffe903eb87a752305d1e5b10da01b53e4ab004 MIT
 */
typedef struct {
    int32_t y[8];             /* 4 biquads x (y1,y2), same as JIANT VOICE */
} mutant_hybrid_filter_t;

static mutant_hybrid_filter_t hy_filter;

/* The hybrid stores its independent patch in GATOR's unchanged patch layout.
 * HOME: CHIP, WAVE, VOWEL | ATTN: SHIFT, Q, CRSH | FX: MIX, TONE.
 * Felucca's LOFI's own oscillator code is not modified.
 * Build an *ephemeral* LOFI source patch: DUTY=64, SWP=VIB=ARP=0.
 */
static macro_patch_t hy_source_patch(const macro_patch_t *p)
{
    macro_patch_t q = *p;
    q.morph = 500;               /* pulse duty / wavetable index */
    q.fm_amt = p->morph_amt;     /* CRSH from ATTN K3 */
    q.timbre_amt = -100;         /* no sweep */
    q.morph_amt = -100;          /* no vibrato */
    q.decay = 0;                 /* no LOFI arp */
    return q;
}

static void hy_all_off(void)
{
    uint32_t i;
    fm1_irq_off();
    for (i = 0; i < 8; ++i) hy_filter.y[i] = 0;
    fm1_irq_on();
}

static void hy_note_on(uint8_t note, uint8_t velocity, const macro_patch_t *p)
{
    macro_patch_t q = hy_source_patch(p);
    hy_all_off();                  /* avoid an old note's resonator ringing */
    lo_note_on(note, velocity, &q);
}
static void hy_note_off(uint8_t note) { lo_note_off(note); }
static int32_t hy_current_note10(void) { return lo_current_note10(); }
static int hy_current_gate(void) { return lo_current_gate(); }

/* Coefficients computed once for an audio block rather than per sample:
 * same JIANT VOWEL tables, Q controls and shifted F1..F4 design.
 * The excitation is LOFI PCM instead of JIANT's glottal source.
 */
static void hy_coefficients(const macro_patch_t *p, fres_t r[4], int32_t *og, int32_t *ogk)
{
    int32_t f[3], b[3], j;
    const voice_t *v = &lo_tracks[0].v[0];
    const int32_t note16 = v->active ? v->pitch16 : 60 * 16;
    const uint32_t pos = (uint32_t)(p->morph * 127 / 1000);
    const int32_t q = p->timbre_amt * 127 / 200 + 64; /* -100..100 -> 0..127 */
    const uint32_t ratio = fmt_ratio((p->fm_amt * 12 / 100) * 16 +
                          ((FMT_FOLOW * (note16 - 60 * 16) * 516) >> 16));
    const uint32_t bws = fmt_ratio((64 - q) * 4);
    vowel_at((int32_t)pos << 8, f, b);
    for (j = 0; j < 3; ++j) {
        const int32_t freq = (int32_t)(((int64_t)f[j] * ratio) >> 16);
        const int32_t bw = (int32_t)(((((int64_t)b[j] * ratio) >> 16) * bws) >> 16);
        fres_coef(&r[j], (uint32_t)lo_bound(freq, 60 * 16, 7000 * 16),
                       (uint32_t)lo_bound(bw, 20 * 16, 1500 * 16));
    }
    fres_coef(&r[3], (uint32_t)lo_bound((int32_t)((3300u * 16u * (uint64_t)ratio) >> 16),
                                      60 * 16, 7000 * 16),
             (uint32_t)lo_bound((int32_t)((((250u * 16u * (uint64_t)ratio) >> 16) * bws) >> 16),
                                      20 * 16, 1500 * 16));
    *og = 13000 - ((q - 64) * 50);         /* same JIANT Q-gain scaling */
    *ogk = (*og >> 2) * 12;                /* JIANT PRES shelf with BUZZ at mid */
}

/* One sample of JIANT's F1 -> F2 -> F3 -> F4 Q30 cascade,
 * retaining its order, rounding, scaling, and soft-knee behavior.
 * Only the glottis (x) is replaced with external LOFI output.
 */
static int32_t hy_filter_sample(int32_t sample, const fres_t r[4], int32_t og, int32_t ogk)
{
    int32_t a, k, previous;
    int32_t *const y = hy_filter.y;
    const int32_t x = lo_bound(sample, -32768, 32767);
    /* Same cascade as formant_render(), using signed multiplication instead
     * of (x << 6) to avoid left shifting a negative signed sample. */
    a = (int32_t)(((int64_t)r[0].a * ((int64_t)x * 64) +
                    (int64_t)r[0].b * y[0] + (int64_t)r[0].c * y[1] +
                    (1ll << 29)) >> 30);
    y[1] = y[0]; y[0] = a;
    a = (int32_t)(((int64_t)r[1].a * a +
                    (int64_t)r[1].b * y[2] + (int64_t)r[1].c * y[3] +
                    (1ll << 29)) >> 30);
    y[3] = y[2]; y[2] = a;
    a = (int32_t)(((int64_t)r[2].a * a +
                    (int64_t)r[2].b * y[4] + (int64_t)r[2].c * y[5] +
                    (1ll << 29)) >> 30);
    y[5] = y[4]; y[4] = a;
    a = (int32_t)(((int64_t)r[3].a * a +
                    (int64_t)r[3].b * y[6] + (int64_t)r[3].c * y[7] +
                    (1ll << 29)) >> 30);
    previous = y[6];
    y[7] = previous;
    y[6] = lo_bound(a, -(1 << 28), 1 << 28);
    k = (int32_t)(((int64_t)y[6] * og + (int64_t)(y[6] - y[7]) * ogk) >> 21);
    return soft_knee(lo_bound(k, -200000, 200000), 24000);
}

static void hy_render(const macro_patch_t *p, const macro_global_t *g, int16_t bend,
                      int32_t *lr, uint32_t frames)
{
    const macro_patch_t source = hy_source_patch(p);
    macro_global_t full_volume = *g;
    fres_t resonators[4];
    int32_t og, ogk;
    uint32_t pos = 0;
    const int32_t mix = lo_bound(p->decay, 0, 1000);
    full_volume.volume = 256;  /* once at final output, rather than twice */
    hy_coefficients(p, resonators, &og, &ogk);
    while (pos < frames) {
        const uint32_t n = frames - pos > CTL ? CTL : frames - pos;
        int32_t chip[2 * CTL];
        uint32_t i;
        lo_render(&source, &full_volume, bend, chip, n);
        for (i = 0; i < n; ++i) {
            const int32_t dry = chip[2 * i];
            const int32_t wet = hy_filter_sample(dry, resonators, og, ogk);
            const int32_t mixed = (int32_t)(((int64_t)dry * (1000 - mix) +
                                            (int64_t)wet * mix) / 1000);
            const int32_t sample = lo_bound((int32_t)(((int64_t)mixed * g->volume) >> 8),
                                            -32768, 32767);
            lr[2u * (pos + i)] = sample;
            lr[2u * (pos + i) + 1u] = sample;
        }
        pos += n;
    }
}
