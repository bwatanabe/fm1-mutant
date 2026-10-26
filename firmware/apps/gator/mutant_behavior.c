/* SPDX-License-Identifier: GPL-3.0-only
 * FM-1 Mutant v0.9: non-destructive STUDIO randomize / MUTANT motion.
 * This is a CONTROL-LAYER extension only: sound engines, patch format,
 * original Plaits modulation matrix and GATOR store routines unchanged.
 */
enum { MUT_DRIFT, MUT_HAUNT, MUT_DECAY, MUT_STYLES };
static const char *const MUT_STYLE_NAME[MUT_STYLES] = {"DRIFT", "HAUNT", "DECAY"};

static uint32_t mut_rng = 0xD9EAA72Bu;
static uint8_t mut_machine;                  /* 0 studio, 1 mutant */
static uint8_t mut_running;
static uint8_t mut_have_capture;
static macro_patch_t mut_anchor, mut_goal, mut_capture;
static uint32_t mut_start_ms, mut_next_ms, mut_last_ms;
static int16_t mut_depth = 45, mut_speed = 35, mut_instability = 25, mut_style = MUT_DRIFT;

static uint32_t mut_random(void)
{
    /* Independent xorshift state. Never mutate audio-engine PRNG state. */
    uint32_t x = mut_rng;
    x ^= x << 13; x ^= x >> 17; x ^= x << 5;
    return mut_rng = x ? x : 0xD9EAA72Bu;
}
static uint32_t mut_range(uint32_t span) { return span ? mut_random() % span : 0; }
static int32_t mut_clamp(int32_t x, int32_t low, int32_t high)
{
    return x < low ? low : x > high ? high : x;
}

/* Parameter selection deliberately covers the sound of every supported
 * engine WITHOUT touching MIDI channel, original GATOR EDIT routes, bank
 * contents, note-mode, pitch bend or engine identity. No mutation guardrail
 * narrows the full available range during STUDIO RANDOM.
 *
 * The 11 parameters include the eight source controls, transpose, and two
 * additional sound fields reused for hybrid morph, grouping or speed.
 */
enum { MUT_FIELDS = 11 };
static int16_t *mut_field(macro_patch_t *p, uint32_t i)
{
    switch (i) {
    case 0: return &p->harmonics;
    case 1: return &p->timbre;
    case 2: return &p->morph;
    case 3: return &p->fm_amt;
    case 4: return &p->timbre_amt;
    case 5: return &p->morph_amt;
    case 6: return &p->decay;
    case 7: return &p->colour;
    case 8: return &p->transpose;
    case 9: return &p->blend;
    default: return &p->spread;
    }
}
static int32_t mut_min(uint32_t i)
{
    return i >= 3 && i <= 5 ? -100 : i == 8 ? -24 : 0;
}
static int32_t mut_max(uint32_t i)
{
    return i == 8 ? 24 : i >= 9 ? 100 : i >= 3 && i <= 5 ? 100 : 1000;
}
static void mut_normalize(macro_patch_t *p)
{
    uint32_t i;
    for (i = 0; i < MUT_FIELDS; ++i)
        *mut_field(p, i) = (int16_t)mut_clamp(*mut_field(p, i), mut_min(i), mut_max(i));
}

/* PLAY in STUDIO: one smaller, bounded relative mutation. ARP in STUDIO:
 * genuinely FULL-RANGE source parameter randomization with each click. */
static void mut_randomize_full(uint32_t ms)
{
    uint32_t i;
    macro_patch_t q = patch;
    mut_rng ^= ms | 1u;
    for (i = 0; i < MUT_FIELDS; ++i)
        *mut_field(&q, i) = (int16_t)(mut_min(i) +
                                   (int32_t)mut_range((uint32_t)(mut_max(i)-mut_min(i)+1)));
    fm1_irq_off();
    patch = q;
    fm1_irq_on();
    dirty = D_ALL;
    message("RANDOM", ms);
}
static void mut_step(uint32_t ms)
{
    uint32_t j;
    macro_patch_t q = patch;
    mut_rng ^= ms | 1u;
    /* Mutate two parameters, retain the chosen engine and EDIT routes.
     * DEPTH determines maximum travel from the current sound. */
    for (j = 0; j < 2; ++j) {
        const uint32_t i = mut_range(MUT_FIELDS);
        const int32_t max_d = (mut_max(i) - mut_min(i)) * mut_depth / 100;
        const int32_t offset = (int32_t)mut_range((uint32_t)(max_d * 2 + 1)) - max_d;
        int16_t *field = mut_field(&q, i);
        *field = (int16_t)mut_clamp((int32_t)*field + offset, mut_min(i), mut_max(i));
    }
    fm1_irq_off();
    patch = q;
    fm1_irq_on();
    dirty = D_ALL;
    message("MUTATE", ms);
}

static void mut_stop(void)
{
    mut_running = 0;
    dirty |= FW_BIT(D_STATUS) | FW_BIT(D_HEADER);
}
static void mut_toggle_machine(uint32_t ms)
{
    mut_stop();
    mut_machine ^= 1u;
    mut_rng ^= ms | 1u;
    message(mut_machine ? "MUTANT" : "STUDIO", ms);
    dirty = D_ALL;
}
static void mut_toggle_run(uint32_t ms)
{
    if (mut_running) {
        mut_stop();
        message("PAUSED", ms);
        return;
    }
    mut_anchor = patch;
    mut_goal = patch;
    mut_normalize(&mut_anchor);
    mut_start_ms = ms;
    mut_next_ms = ms;
    mut_last_ms = ms;
    mut_running = 1;
    message("EVOLVING", ms);
    dirty = D_ALL;
}
static void mut_checkpoint(uint32_t ms)
{
    mut_capture = patch;
    mut_have_capture = 1;
    message("CAPTURED", ms);
}
/* Checkpoints are shared between STUDIO and MUTANT. Long REC clears it,
 * and its LED goes dark; clearing never touches flash or audio sources. */
static void mut_clear_capture(uint32_t ms)
{
    mut_have_capture = 0;
    message("CAP CLEARED", ms);
    dirty |= FW_BIT(D_STATUS);
}

static void mut_restore(uint32_t ms)
{
    if (!mut_have_capture) {
        message("NO CAPTURE", ms);
        return;
    }
    mut_stop();
    fm1_irq_off();
    patch = mut_capture;
    fm1_irq_on();
    mut_goal = patch;
    dirty = D_ALL;
    message("RESTORED", ms);
}

static void mut_motion_tick(uint32_t now)
{
    uint32_t i;
    uint32_t span;
    uint32_t interval;
    macro_patch_t updated;
    int changed = 0;
    if (!mut_machine || !mut_running || now - mut_last_ms < 30u)
        return;
    mut_last_ms = now;
    /* Longer between new targets at low SPEED; more fluid at high SPEED. */
    interval = 100u + (uint32_t)(100 - mut_speed) * 11u;
    if ((int32_t)(now - mut_next_ms) >= 0) {
        uint32_t picks = 1u + (mut_instability >= 75 ? 1u : 0u);
        while (picks--) {
            const uint32_t field_idx = mut_range(MUT_FIELDS);
            const int32_t lo = mut_min(field_idx), hi = mut_max(field_idx);
            const int32_t anchor = *mut_field(&mut_anchor, field_idx);
            int32_t available = (hi-lo) * mut_depth / 100;
            int32_t lower, upper, value;
            if (mut_style == MUT_DECAY) {
                /* Slowly widen the possible damage over about 12 seconds. */
                const uint32_t fraction = (now - mut_start_ms) / 120u;
                available = available * (int32_t)(fraction > 100u ? 100u : fraction) / 100;
            }
            lower = mut_clamp(anchor - available, lo, hi);
            upper = mut_clamp(anchor + available, lo, hi);
            value = lower + (int32_t)mut_range((uint32_t)(upper - lower + 1));
            if (mut_style == MUT_HAUNT && mut_range(100) < 42u)
                value = anchor; /* sound keeps returning toward its memory */
            *mut_field(&mut_goal, field_idx) = (int16_t)value;
            if (mut_range(100) < (uint32_t)mut_instability / 5u) {
                /* Rare sudden deviations, not constant random preset jumps. */
                fm1_irq_off();
                *mut_field(&patch, field_idx) = (int16_t)value;
                fm1_irq_on();
                changed = 1;
            }
        }
        mut_next_ms = now + interval;
    }
    updated = patch;
    for (i = 0; i < MUT_FIELDS; ++i) {
        int16_t *v = mut_field(&updated, i);
        const int32_t diff = (int32_t)*mut_field(&mut_goal, i) - *v;
        if (diff) {
            /* Glide rather than hopping instantly. At most 1/9 remaining
             * per 30ms, with minimum 1 unit to avoid stalls. */
            int32_t delta = diff / 9;
            if (delta == 0) delta = diff > 0 ? 1 : -1;
            *v = (int16_t)mut_clamp((int32_t)*v + delta, mut_min(i), mut_max(i));
            changed = 1;
        }
    }
    if (changed) {
        fm1_irq_off();
        patch = updated;
        fm1_irq_on();
        dirty |= FW_BIT(D_HEADER) | 0x1Eu;
    }
}

/* Seeding is local to performance controls, not the firmware's persistent
 * bank. Loading/saving a preset remains strictly explicit. */
static void mut_reset_on_preset(void)
{
    mut_running = 0;
    mut_goal = patch;
}
