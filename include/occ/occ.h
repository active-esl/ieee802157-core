/* SPDX-License-Identifier: GPL-3.0-only */
#ifndef OCC_OCC_H
#define OCC_OCC_H
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Development waveform/profile primitives, NOT full IEEE conformance.
 * Time is monotonic nanoseconds; channel bits are logical, never board pins.
 * No heap, OS calls, driver calls, global mutable state, or camera access. */
typedef enum {
    OCC_OK = 0, OCC_INVALID = -1, OCC_CAPACITY = -2,
    OCC_UNCERTAIN = -3, OCC_UNSUPPORTED = -4
} occ_result;

#define OCC_MAX_SEGMENT_NS UINT64_C(180000000000)
#define OCC_MAX_RATE_NUM UINT32_C(100000)

/* active_mask selects outputs. inverted_mask is a subset of active_mask.
 * rate_num/rate_den Hz, 50% duty; rate_num=0 means constant active_mask.
 * A zero active_mask means dark. Carrier phase restarts at a segment boundary.
 * Rounding is <1 ns per segment; adapter must measure accumulated phase error. */
typedef struct {
    uint64_t duration_ns;
    uint32_t rate_num;
    uint32_t rate_den;
    uint8_t active_mask;
    uint8_t inverted_mask;
} occ_segment;

typedef struct {
    occ_segment *segments;
    size_t count;
    size_t capacity;
} occ_wave;

typedef struct {
    uint32_t fps_num;
    uint32_t fps_den;
    uint32_t frequency_multiple; /* space=N*fps; mark=(N-1/2)*fps */
    uint32_t delimiter_hz;       /* >1000 Hz, independent of payload */
    uint8_t channel;             /* one logical channel bit */
} occ_ufsook_profile;

/* Payload bits are caller supplied binary values, not bytes.
 * Includes opening/closing four-video-period delimiters.
 * Optional IEEE CRC-3 is NOT currently emitted. Application CRC is separate. */
occ_result occ_ufsook_encode(const occ_ufsook_profile *p,
                            const uint8_t *bits, size_t n, occ_wave *out);

/* Spatial S2-PSK Figure179 polarity: in-phase=1, inverse-phase=0.
 * Half-rate line code 0->00, 1->01, preamble 1111.
 * Two distinct, optically resolvable channels; RGB co-location is insufficient.
 * clock_hz is line-symbol rate, carrier_hz is modulation rate.
 * Explicit closing preamble; arbitrary payload, no outer FEC at this stage. */
typedef struct {
    uint32_t clock_hz;
    uint32_t carrier_hz;
    uint8_t channel_a;
    uint8_t channel_b;
} occ_s2psk_profile;
occ_result occ_s2psk_encode(const occ_s2psk_profile *p,
                           const uint8_t *bits, size_t n, occ_wave *out);

/* Manchester C-OOK sub-packet: 011100 + Manchester(Ab,data,Ab).
 * One Ab bit; repetitions belong to one packet and retain Ab.
 * No inner/outer FEC or full PPDU/MAC at this stage.
 * Manchester convention in this adapted profile: 0->01, 1->10. */
typedef struct {
    uint32_t optical_clock_hz;
    uint32_t repetitions;
    uint8_t channel;
} occ_cook_profile;
occ_result occ_cook_encode(const occ_cook_profile *p, uint8_t ab,
                          const uint8_t *bits, size_t n, occ_wave *out);

/* Pure waveform lookup; no hardware actuation. */
occ_result occ_segment_level(const occ_segment *s, uint64_t t_ns,
                             uint8_t *level);
occ_result occ_wave_level(const occ_wave *w, uint64_t t_ns, uint8_t *level);
occ_result occ_wave_duration(const occ_wave *w, uint64_t *duration_ns);

/* Primitive demappers require already-tracked, confidently classified samples.
 * -1 is uncertain, not OFF. Acquisition/framing are separate work. */
occ_result occ_ufsook_demod_pair(int first, int second, uint8_t *bit);
occ_result occ_s2psk_demod_pair(int a, int b, uint8_t *bit);
occ_result occ_s2psk_line_decode(uint8_t first, uint8_t second, uint8_t *bit);
occ_result occ_manchester_decode(uint8_t first, uint8_t second, uint8_t *bit);

/* Explicit CRC-16/CCITT-FALSE: poly1021, initFFFF, non-reflected, xorout0.
 * This is an application integrity primitive, NOT UFSOOK's IEEE CRC-3. */
uint16_t occ_crc16(const uint8_t *bytes, size_t n);

/* Observations do not imply instructions, authority or authenticated freshness.
 * Repeat packets may confirm reception but must not advance progress_ns.
 * session/sequence checking is the caller's job; this helper never infers it. */
typedef enum {
    OCC_NO_SIGNAL, OCC_UNCERTAIN_SIGNAL, OCC_VALID_OBSERVATION,
    OCC_STALE_OBSERVATION
} occ_observation_state;
typedef struct {
    uint64_t last_signal_ns;
    uint64_t last_progress_ns;
    uint8_t have_signal;
    uint8_t have_valid;
    uint8_t uncertain;
} occ_observation;
occ_result occ_observe(occ_observation *o, uint64_t now_ns,
                      int signal, int valid, int progresses);
occ_observation_state occ_observation_status(const occ_observation *o,
                        uint64_t now_ns, uint64_t signal_timeout_ns,
                        uint64_t progress_timeout_ns);
#ifdef __cplusplus
}
#endif
#endif
