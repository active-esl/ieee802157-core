/* SPDX-License-Identifier: GPL-3.0-only */
#include "occ/occ.h"
#include <limits.h>

static int one_bit(uint8_t c)
{
    return c != 0U && (c & (uint8_t)(c - 1U)) == 0U;
}
static int bits_valid(const uint8_t *bits, size_t n)
{
    size_t i;
    if (n != 0U && bits == NULL) return 0;
    for (i = 0; i < n; ++i) if (bits[i] > 1U) return 0;
    return 1;
}
static int segment_valid(const occ_segment *s)
{
    return s != NULL && s->duration_ns != 0U &&
        s->duration_ns <= OCC_MAX_SEGMENT_NS &&
        s->rate_num <= OCC_MAX_RATE_NUM && s->rate_den != 0U &&
        s->rate_den <= UINT32_C(1000000) &&
        (s->inverted_mask & s->active_mask) == s->inverted_mask &&
        (s->rate_num != 0U || s->inverted_mask == 0U);
}
static occ_result prepare(occ_wave *out, size_t needed)
{
    if (out == NULL || out->segments == NULL) return OCC_INVALID;
    if (needed > out->capacity) return OCC_CAPACITY;
    out->count = 0U;
    return OCC_OK;
}
static void push(occ_wave *w, uint64_t duration, uint32_t num, uint32_t den,
                 uint8_t active, uint8_t inv)
{
    occ_segment s = {duration, num, den, active, inv};
    w->segments[w->count++] = s;
}
occ_result occ_segment_level(const occ_segment *s, uint64_t t, uint8_t *level)
{
    uint64_t halves;
    if (!segment_valid(s) || level == NULL || t >= s->duration_ns)
        return OCC_INVALID;
    if (s->rate_num == 0U) { *level = s->active_mask; return OCC_OK; }
    /* Bounds above prove t*rate_num and 1e9*rate_den cannot overflow. */
    halves = (t * s->rate_num) / (UINT64_C(500000000) * s->rate_den);
    *level = (halves & 1U) == 0U ?
        (uint8_t)(s->active_mask ^ s->inverted_mask) : s->inverted_mask;
    return OCC_OK;
}
occ_result occ_wave_duration(const occ_wave *w, uint64_t *duration)
{
    size_t i;
    uint64_t total = 0U;
    if (w == NULL || duration == NULL || w->segments == NULL ||
        w->count > w->capacity) return OCC_INVALID;
    for (i = 0U; i < w->count; ++i) {
        if (!segment_valid(&w->segments[i]) ||
            UINT64_MAX - total < w->segments[i].duration_ns) return OCC_INVALID;
        total += w->segments[i].duration_ns;
    }
    *duration = total;
    return OCC_OK;
}
occ_result occ_wave_level(const occ_wave *w, uint64_t t, uint8_t *level)
{
    size_t i;
    uint64_t total;
    if (level == NULL || occ_wave_duration(w, &total) != OCC_OK)
        return OCC_INVALID;
    if (t >= total) { *level = 0U; return OCC_OK; }
    for (i = 0U; i < w->count; ++i) {
        if (t < w->segments[i].duration_ns)
            return occ_segment_level(&w->segments[i], t, level);
        t -= w->segments[i].duration_ns;
    }
    return OCC_INVALID;
}
occ_result occ_ufsook_encode(const occ_ufsook_profile *p,
                            const uint8_t *bits, size_t n, occ_wave *out)
{
    uint64_t bit_ns, den, space, mark;
    size_t i;
    occ_result r;
    if (p == NULL || !bits_valid(bits, n) || !one_bit(p->channel) ||
        p->fps_num == 0U || p->fps_num > 1000U ||
        p->fps_den == 0U || p->fps_den > 1000U ||
        p->frequency_multiple == 0U || p->frequency_multiple > 100U ||
        p->delimiter_hz <= 1000U || p->delimiter_hz > OCC_MAX_RATE_NUM ||
        n > SIZE_MAX - 4U) return OCC_INVALID;
    bit_ns = UINT64_C(2000000000) * p->fps_den / p->fps_num;
    den = (uint64_t)p->fps_den * 2U;
    space = (uint64_t)p->frequency_multiple * p->fps_num * 2U;
    mark = ((uint64_t)p->frequency_multiple * 2U - 1U) * p->fps_num;
    if (bit_ns == 0U || bit_ns > OCC_MAX_SEGMENT_NS ||
        space > OCC_MAX_RATE_NUM || mark > OCC_MAX_RATE_NUM)
        return OCC_UNSUPPORTED;
    r = prepare(out, n + 4U); if (r != OCC_OK) return r;
    push(out, bit_ns, p->delimiter_hz, 1U, p->channel, 0U);
    push(out, bit_ns, (uint32_t)mark, (uint32_t)den, p->channel, 0U);
    for (i = 0U; i < n; ++i)
        push(out, bit_ns, (uint32_t)(bits[i] == 0U ? space : mark),
             (uint32_t)den, p->channel, 0U);
    push(out, bit_ns, p->delimiter_hz, 1U, p->channel, 0U);
    push(out, bit_ns, (uint32_t)mark, (uint32_t)den, p->channel, 0U);
    return OCC_OK;
}
occ_result occ_s2psk_encode(const occ_s2psk_profile *p,
                           const uint8_t *bits, size_t n, occ_wave *out)
{
    uint64_t symbol_ns;
    size_t i;
    uint8_t channels;
    occ_result r;
    if (p == NULL || !bits_valid(bits, n) || !one_bit(p->channel_a) ||
        !one_bit(p->channel_b) || p->channel_a == p->channel_b ||
        p->clock_hz == 0U || p->clock_hz > 1000U ||
        p->carrier_hz == 0U || p->carrier_hz > OCC_MAX_RATE_NUM ||
        p->carrier_hz < p->clock_hz || p->carrier_hz % p->clock_hz != 0U ||
        n > (SIZE_MAX - 8U) / 2U) return OCC_INVALID;
    symbol_ns = UINT64_C(1000000000) / p->clock_hz;
    channels = (uint8_t)(p->channel_a | p->channel_b);
    r = prepare(out, n * 2U + 8U); if (r != OCC_OK) return r;
    for (i = 0U; i < 4U; ++i)
        push(out, symbol_ns, p->carrier_hz, 1U, channels, 0U);
    for (i = 0U; i < n; ++i) {
        push(out, symbol_ns, p->carrier_hz, 1U, channels, p->channel_b);
        push(out, symbol_ns, p->carrier_hz, 1U, channels,
             bits[i] == 0U ? p->channel_b : 0U);
    }
    for (i = 0U; i < 4U; ++i)
        push(out, symbol_ns, p->carrier_hz, 1U, channels, 0U);
    return OCC_OK;
}
static void manchester(occ_wave *out, uint8_t bit, uint64_t ns, uint8_t ch)
{
    push(out, ns, 0U, 1U, bit == 0U ? 0U : ch, 0U);
    push(out, ns, 0U, 1U, bit == 0U ? ch : 0U, 0U);
}
occ_result occ_cook_encode(const occ_cook_profile *p, uint8_t ab,
                          const uint8_t *bits, size_t n, occ_wave *out)
{
    static const uint8_t preamble[6] = {0U,1U,1U,1U,0U,0U};
    size_t clocks, i, repeat;
    uint64_t ns;
    occ_result r;
    if (p == NULL || !bits_valid(bits, n) || !one_bit(p->channel) || ab > 1U ||
        p->optical_clock_hz == 0U || p->optical_clock_hz > OCC_MAX_RATE_NUM ||
        p->repetitions == 0U || p->repetitions > 1000U ||
        n > (SIZE_MAX - 10U) / 2U) return OCC_INVALID;
    clocks = n * 2U + 10U;
    if (clocks > SIZE_MAX / p->repetitions) return OCC_INVALID;
    r = prepare(out, clocks * p->repetitions); if (r != OCC_OK) return r;
    ns = UINT64_C(1000000000) / p->optical_clock_hz;
    for (repeat = 0U; repeat < p->repetitions; ++repeat) {
        for (i = 0U; i < 6U; ++i)
            push(out, ns, 0U, 1U, preamble[i] == 0U ? 0U : p->channel, 0U);
        manchester(out, ab, ns, p->channel);
        for (i = 0U; i < n; ++i) manchester(out, bits[i], ns, p->channel);
        manchester(out, ab, ns, p->channel);
    }
    return OCC_OK;
}
static occ_result xor_sample(int a, int b, uint8_t *bit)
{
    if (bit == NULL) return OCC_INVALID;
    if ((a != 0 && a != 1) || (b != 0 && b != 1)) return OCC_UNCERTAIN;
    *bit = (uint8_t)(a ^ b);
    return OCC_OK;
}
occ_result occ_ufsook_demod_pair(int a, int b, uint8_t *bit)
{ return xor_sample(a,b,bit); }
occ_result occ_s2psk_demod_pair(int a, int b, uint8_t *bit)
{
    uint8_t difference;
    occ_result r;
    if (bit == NULL) return OCC_INVALID;
    r=xor_sample(a,b,&difference);
    if (r != OCC_OK) return r;
    /* Figure179: in-phase=1, inverse-phase=0. AnnexI XOR wording differs. */
    *bit=(uint8_t)(1U-difference);
    return OCC_OK;
}
occ_result occ_s2psk_line_decode(uint8_t a, uint8_t b, uint8_t *bit)
{
    if (bit == NULL) return OCC_INVALID;
    if (a != 0U || b > 1U) return OCC_UNCERTAIN;
    *bit = b; return OCC_OK;
}
occ_result occ_manchester_decode(uint8_t a, uint8_t b, uint8_t *bit)
{
    if (bit == NULL) return OCC_INVALID;
    if (a > 1U || b > 1U || a == b) return OCC_UNCERTAIN;
    *bit = a; return OCC_OK;
}
uint16_t occ_crc16(const uint8_t *bytes, size_t n)
{
    uint16_t crc = UINT16_C(0xffff);
    size_t i;
    unsigned j;
    if (bytes == NULL && n != 0U) return 0U;
    for (i = 0U; i < n; ++i) {
        crc = (uint16_t)(crc ^ (uint16_t)((uint16_t)bytes[i] << 8U));
        for (j = 0U; j < 8U; ++j)
            crc = (uint16_t)((uint16_t)(crc << 1U) ^
                ((crc & UINT16_C(0x8000)) != 0U ? UINT16_C(0x1021) : 0U));
    }
    return crc;
}
occ_result occ_observe(occ_observation *o, uint64_t now,
                      int signal, int valid, int progresses)
{
    if (o == NULL || (valid && !signal) || (progresses && !valid) ||
        (o->have_signal && now < o->last_signal_ns) ||
        (o->have_valid && now < o->last_progress_ns)) return OCC_INVALID;
    if (signal) { o->have_signal = 1U; o->last_signal_ns = now; }
    o->uncertain = (uint8_t)(signal && !valid);
    if (valid && progresses) {
        o->have_valid = 1U; o->last_progress_ns = now;
    }
    return OCC_OK;
}
occ_observation_state occ_observation_status(const occ_observation *o,
                        uint64_t now, uint64_t signal_timeout,
                        uint64_t progress_timeout)
{
    if (o == NULL || !o->have_signal) return OCC_NO_SIGNAL;
    if (now < o->last_signal_ns || (o->have_valid && now < o->last_progress_ns))
        return OCC_UNCERTAIN_SIGNAL;
    if (now - o->last_signal_ns >= signal_timeout) return OCC_NO_SIGNAL;
    if (o->uncertain || !o->have_valid) return OCC_UNCERTAIN_SIGNAL;
    if (now - o->last_progress_ns >= progress_timeout)
        return OCC_STALE_OBSERVATION;
    return OCC_VALID_OBSERVATION;
}
