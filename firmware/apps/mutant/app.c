/* SPDX-License-Identifier: GPL-3.0-only
 * FM-1 MUTANT: first proof-of-integration, Felucca's unchanged LOFI sound engine.
 * This application is independent of the original GATOR app.
 */
#define NVOICE 1
#define NPART 1
#include "fe_tables.h"
#include "felucca_engine.h"
enum { P_ATK, P_REL, P_FTYPE, P_E0, P_E1, P_E2, P_E3, P_E4, P_E5, P_E6, P_E7, P_COUNT };
typedef struct track { int16_t p[P_COUNT]; voice_t v[NVOICE]; } track_t;
static track_t trk[NPART];
#include "dsp.c"
#include "eng_lofi.c"

static track_t *const sound = &trk[0];
static uint32_t page, preset, octave = 3, dirty = 1, master_q8 = 256, age;
static uint8_t local_note[FW_KEYS];
static uint32_t local_keys;

static int32_t bound(int32_t n, int32_t lo, int32_t hi)
{ return n < lo ? lo : n > hi ? hi : n; }

static void load_preset(uint32_t k)
{
    uint32_t i;
    const preset_t *p = &LOFI_PRESETS[k % ENG_LOFI.npresets];
    fm1_irq_off();
    for (i = 0; i < 8; ++i) sound->p[P_E0 + i] = p->e[i];
    sound->p[P_ATK] = p->env[0]; sound->p[P_REL] = p->env[3];
    fm1_irq_on();
    preset = k % ENG_LOFI.npresets; dirty = 1;
}
static void key_on(uint8_t note, uint8_t velocity)
{
    voice_t *v = &sound->v[0];
    fm1_irq_off();
    v->note = note; v->vel = velocity; v->gate = 1; v->active = 1; v->stage = 1;
    v->env = 0; v->env_out = 0;
    v->pitch16 = v->pitch_cur = (int32_t)note * 16;
    v->age = ++age; ENG_LOFI.note_on(sound, v);
    fm1_irq_on(); dirty = 1;
}
static void key_off(uint8_t note)
{
    voice_t *v = &sound->v[0];
    fm1_irq_off();
    if (v->active && v->note == note && v->gate) { v->gate = 0; v->stage = 3; }
    fm1_irq_on();
}
static void panic(void)
{
    voice_t *v = &sound->v[0];
    fm1_irq_off();
    v->gate = v->active = v->stage = 0; v->env = v->env_out = 0;
    fm1_irq_on();
}
static void midi_read(void)
{
    fw_midi_t m;
    while (fw_midi_read(&m)) {
        const uint32_t kind = m.status & 0xF0u;
        if (kind == 0x90u && m.d2) key_on(m.d1, m.d2);
        else if (kind == 0x80u || (kind == 0x90u && !m.d2)) key_off(m.d1);
        else if (kind == 0xB0u) {
            if (m.d1 == 120u || m.d1 == 123u) panic();
            else if (m.d1 == 1u || (m.d1 >= 16u && m.d1 <= 23u)) {
                uint32_t i = m.d1 == 1u ? 7u : (uint32_t)m.d1 - 16u;
                const param_desc_t *d = &ENG_LOFI.edit[i];
                int32_t v = d->min + ((int32_t)m.d2 * (d->max - d->min) + 63) / 127;
                fm1_irq_off();
                sound->p[P_E0 + i] = (int16_t)bound(v, d->min, d->max);
                fm1_irq_on(); dirty = 1;
            }
        }
    }
}
static void keys_read(const fw_input_t *in)
{
    uint32_t k;
    for (k = 0; k < FW_KEYS; ++k) {
        const uint32_t bit = FW_BIT(k);
        if (in->keys_released & bit) {
            if (local_keys & bit) key_off(local_note[k]);
            local_keys &= ~bit;
        }
        if (in->keys_pressed & bit) {
            uint8_t n = (uint8_t)bound((int32_t)FW_KEY_BASE_NOTE + (int32_t)k +
                                      ((int32_t)octave - 3) * 12, 0, 127);
            local_note[k] = n; local_keys |= bit; key_on(n, 100);
        }
    }
    if (in->pressed & FW_BIT(FW_BTN_OCT_DOWN)) { octave = (uint32_t)bound((int32_t)octave - 1, 0, 6); dirty = 1; }
    if (in->pressed & FW_BIT(FW_BTN_OCT_UP)) { octave = (uint32_t)bound((int32_t)octave + 1, 0, 6); dirty = 1; }
    if (in->pressed & FW_BIT(FW_BTN_EDIT)) { page ^= 1u; dirty = 1; }
}
static void knobs_read(const fw_input_t *in)
{
    uint32_t i;
    if (in->knob[FW_KNOB_PRESET]) {
        int32_t n = (int32_t)preset + in->knob[FW_KNOB_PRESET], max = ENG_LOFI.npresets;
        load_preset((uint32_t)((n % max + max) % max));
    }
    for (i = 0; i < 6; ++i) {
        const int32_t delta = in->knob[i < 2 ? FW_KNOB_SELECT + i : FW_KNOB_1 + i - 2];
        const uint32_t idx = i < 2 ? i : page * 4 + i - 2;
        const param_desc_t *d = &ENG_LOFI.edit[idx];
        if (!delta) continue;
        fm1_irq_off();
        sound->p[P_E0 + idx] = (int16_t)bound(sound->p[P_E0 + idx] + delta, d->min, d->max);
        fm1_irq_on(); dirty = 1;
    }
    master_q8 = in->master * in->master >> 12;
}
static void draw_ui(void)
{
    uint32_t i;
    char b[16];
    cv_begin(240, 28, C_BLACK);
    cv_text(10, 4, &FONT_S, "FM-1 MUTANT  /  LOFI", C_WHITE); cv_blit(0, 8);
    for (i = 0; i < 4; ++i) {
        const uint32_t idx = page * 4 + i;
        const param_desc_t *d = &ENG_LOFI.edit[idx];
        const int32_t v = sound->p[P_E0 + idx];
        cv_begin(240, 28, C_BLACK);
        cv_text(10, 3, &FONT_S, d->label, C_GRAY);
        fmt_int(b, v); cv_text(104, 3, &FONT_S, b, C_HI);
        cv_rect(160, 12, 65, 5, C_DIM);
        cv_rect(160, 12, (v - d->min) * 65 / (d->max - d->min), 5, C_ACCENT);
        cv_blit(0, 45 + i * 34);
    }
    cv_begin(240, 40, C_BLACK);
    cv_text(9, 2, &FONT_S, page ? "MOTION / EDIT TO SWITCH" : "CHIP / EDIT TO SWITCH", C_ACCENT);
    cv_text(10, 20, &FONT_S, "PRESET", C_GRAY);
    fmt_int(b, (int32_t)preset + 1); cv_text(90, 20, &FONT_S, b, C_WHITE);
    cv_blit(0, 194);
    dirty = 0;
}
static int32_t envelope(voice_t *v)
{
    if (!v->active) return 0;
    if (v->stage == 1) {
        v->env += 1 << 22;
        if (v->env >= (1 << 24)) { v->env = 1 << 24; v->stage = 2; }
    } else if (v->stage == 2)
        v->env -= (v->env - (3 << 22)) >> 3;
    else if (v->stage == 3) {
        v->env -= v->env >> 4;
        if (v->env < (1 << 24) / 1000) { v->env = 0; v->active = 0; v->stage = 0; }
    }
    return v->env >> 9;
}
static void app_init(void) { load_preset(0); }
static void app_frame(const fw_input_t *in)
{
    keys_read(in); midi_read(); knobs_read(in);
    if (dirty) draw_ui();
}
static void app_audio(int32_t *lr, uint32_t frames)
{
    uint32_t pos = 0;
    voice_t *v = &sound->v[0];
    while (pos < frames) {
        const uint32_t n = frames - pos > CTL ? CTL : frames - pos;
        int32_t mono[CTL] = {0};
        uint32_t i;
        if (v->active) {
            vmod_t m = {0};
            int32_t amp = envelope(v);
            if (ENG_LOFI.amp) amp = ENG_LOFI.amp(sound, v, amp);
            if (v->active) {
                m.pitch16 = v->pitch16; m.inc = pitch_inc((uint32_t)bound(m.pitch16, 0, 2047));
                m.amp0 = v->env_out; m.amp1 = amp; m.envq15 = amp;
                v->env_out = amp;
                ENG_LOFI.render(sound, v, mono, n, &m);
            }
        }
        for (i = 0; i < n; ++i) {
            const int32_t s = (int32_t)(((int64_t)mono[i] * (int32_t)master_q8) >> 8);
            lr[2u * (pos + i)] = s; lr[2u * (pos + i) + 1u] = s;
        }
        pos += n;
    }
}
