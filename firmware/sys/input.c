/* SPDX-License-Identifier: GPL-3.0-only */
static const uint8_t FW_BTN_MATRIX[FW_BTN_COUNT] = {2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 0, 1};
static const uint8_t FW_KNOB_MATRIX[FW_KNOB_COUNT] = {0, 1, 6, 2, 3, 4, 5};
static const char *const FW_BTN_NAME[FW_BTN_COUNT] = {
    "FX", "SCL", "ENV", "LFO", "EDIT", "GLO", "HOME", "SAVE", "ARP", "SEQ", "PLAY", "REC", "OCT-", "OCT+"
};
static const char *const FW_KNOB_NAME[FW_KNOB_COUNT] = {
    "SELECT", "ALGO", "PRESET", "K1", "K2", "K3", "K4"
};

static uint32_t fw_labels_of(uint32_t matrix_bits)
{
    uint32_t out = 0, b;
    for (b = 0; b < FW_BTN_COUNT; b++) {
        if (matrix_bits & FW_BIT(FW_BTN_MATRIX[b]))
            out |= FW_BIT(b);
    }
    return out;
}

static uint32_t fw_matrix_of(uint32_t label) { return FW_BTN_MATRIX[label]; }

static int32_t fw_master_knob;
static uint32_t fw_batt_raw;
static uint32_t fw_prev_keys;

static void fw_input_read(fw_input_t *in)
{
    uint32_t rel = 0, k;
    const uint32_t pressed = fm1_input_note_edges();
    const uint32_t keys = fm1_in.notes & ((1u << FW_KEYS) - 1u);
    const uint32_t btn_pressed = fm1_input_edges(&rel);
    const int32_t master = fm1_adc_read(FM1_ADC_MASTER);
    const int32_t batt = fm1_adc_read(FM1_ADC_BATT);
    in->ms = fm1_ms;
    in->held = fw_labels_of(fm1_in.buttons);
    in->pressed = fw_labels_of(btn_pressed);
    in->released = fw_labels_of(rel);
    in->keys = keys;
    in->keys_pressed = pressed;
    in->keys_released = (fw_prev_keys | pressed) & ~keys;
    fw_prev_keys = keys;
    for (k = 0; k < FW_KNOB_COUNT; k++)
        in->knob[k] = fm1_enc_take(FW_KNOB_MATRIX[k]);
    if (master >= 0)
        fw_master_knob += (master * 16 - fw_master_knob) / 8;
    in->master = (uint32_t)(fw_master_knob / 16);
    if (batt > 0 && !fw_batt_raw)
        fw_batt_raw = (uint32_t)batt;
    if (batt > 0)
        fw_batt_raw = (uint32_t)((int32_t)fw_batt_raw + (batt - (int32_t)fw_batt_raw) / 32);
    in->battery = fw_batt_raw;
}

static void fw_led_button(uint32_t label, int on) { fm1_led_key(fw_matrix_of(label), on); }
/* Mutant-only optional button label illumination; the FM-1 HAL provides
 * three physical layers (full bright, dim glow and timed background pulse).
 * Apply only to 14 control buttons, NEVER the 27 note key LEDs.
 * Called from the UI; recomputes the mask only when the selected level
 * changes, so it has no per-audio-block overhead.
 * 0 = OFF, 1 = low, 2 = medium, 3 = brighter inactive label.
 * Full-bright LED state always wins over the background layer. */
static void fw_led_button_background(uint32_t level)
{
#ifdef FW_HOST
    /* The host simulator has no FM-1 LED matrix hardware. Audio/button state
     * tests run normally; brightness requires a physical FM-1 inspection. */
    (void)level;
#else
    static uint32_t last_level = 0xFFFFFFFFu;
    static const uint16_t PULSE_NS[4] = {0, 4500, 10500, 22000};
    uint32_t label, col, row;
    if (level > 3u) level = 3u;
    if (last_level == level) return;
    last_level = level;
    for (col = 0; col < FM1_NCOL; ++col)
        fm1_led_bg[col] = 0;
    fm1_led_bg_ns = PULSE_NS[level];
    if (!level) return;
    for (label = 0; label < FW_BTN_COUNT; ++label) {
        const int id = (int)fw_matrix_of(label);
        for (col = 0; col < FM1_NCOL; ++col)
            for (row = 1; row < 5u; ++row)
                if (FM1_KEYMAP[row][col] == id)
                    fm1_led_bg[col] |= (uint8_t)(1u << row);
    }
#endif /* FW_HOST */
}

static void fw_led_note_key(uint32_t key, int on) { fm1_led_key(14u + key, on); }

static void fw_leds_keys(uint32_t mask)
{
    uint32_t k;
    for (k = 0; k < FW_KEYS; k++)
        fw_led_note_key(k, (mask >> k) & 1u);
}

static int fw_midi_read(fw_midi_t *m)
{
    uint32_t pkt;
    if (mi_r == mi_w)
        return 0;
    RING_PUBLISH();
    pkt = midi_in_q[mi_r % MQ];
    RING_PUBLISH();
    mi_r++;
    m->cable = (uint8_t)(pkt >> 4 & 15u);
    m->status = (uint8_t)(pkt >> 8);
    m->d1 = (uint8_t)(pkt >> 16);
    m->d2 = (uint8_t)(pkt >> 24);
    return 1;
}

static void fw_midi_send(uint8_t status, uint8_t d1, uint8_t d2)
{
    const uint32_t cin = status >= 0xF0u ? 0xFu : (uint32_t)(status >> 4);
    fm1_irq_off();
    midi_out_event(cin | (uint32_t)status << 8 | (uint32_t)d1 << 16 | (uint32_t)d2 << 24);
    fm1_irq_on();
}
