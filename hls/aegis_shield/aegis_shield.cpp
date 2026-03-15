// =============================================================================
// aegis_shield.cpp — CC-CBF Physics Engine  (corrected pipeline structure)
// =============================================================================

#include "aegis_shield.h"

static inline ap_int<16> saturate(ap_int<32> val)
{
#pragma HLS INLINE
    if      (val >  32767) return  32767;
    else if (val < -32768) return -32768;
    else                   return (ap_int<16>)val;
}

void aegis_shield(
    ap_int<16>    anx_in,
    axis_stream_t &x_t,
    axis_stream_t &u_t
)
{
    // ---- HLS interface directives -------------------------------------------
#pragma HLS INTERFACE ap_none      port=anx_in
#pragma HLS INTERFACE axis         port=x_t
#pragma HLS INTERFACE axis         port=u_t
#pragma HLS INTERFACE ap_ctrl_none port=return

    // =========================================================================
    // PIPELINE STRUCTURE — why we do NOT pipeline the whole function:
    //
    //   #pragma HLS PIPELINE on the function + UNROLL on stream-read loops
    //   causes HLS to generate a multi-cycle pipelined FSM where some x[]
    //   pipeline registers are inferred as unsigned [15:0], and register
    //   partial-updates zero out LSBs of intermediate sums.  This makes h
    //   always ≥ 0, so the safe branch is always taken → u_t = 0.
    //
    //   Correct structure:
    //     READ_LOOP  : #pragma HLS PIPELINE II=1 (one stream word / cycle)
    //     DOT_LOOP   : #pragma HLS UNROLL         (parallel reduction tree)
    //     STEER_LOOP : #pragma HLS UNROLL         (parallel per-element)
    //     WRITE_LOOP : #pragma HLS PIPELINE II=1 (one stream word / cycle)
    //
    //   Total latency ≈ 16 + 1 + 1 + 16 = 34 cycles per 16-element frame.
    //   At 115200 baud each frame takes 285 714 cycles → ample headroom.
    // =========================================================================

    // =========================================================================
    // Stage 1 — Read 16 words from x_t AXI-Stream.
    // PIPELINE II=1: one read per clock; signed ap_int<16> register per element.
    // =========================================================================
    ap_int<16> x[DIM];
#pragma HLS ARRAY_PARTITION variable=x complete dim=1

    READ_LOOP: for (int i = 0; i < DIM; i++)
    {
#pragma HLS PIPELINE II=1
        axis_word_t beat = x_t.read();
        // Explicit sign extension — does not rely on HLS type inference.
        // If MSB==1, subtract 65536 to get the negative value arithmetically.
        // This forces HLS to emit a subtractor, not a bit-cast, so signedness
        // is preserved through all pipeline registers regardless of their type.
        ap_uint<16> raw = (ap_uint<16>)beat.data;
        ap_int<32> xi;
        if (raw[15]) xi = (ap_int<32>)raw - (ap_int<32>)65536;
        else         xi = (ap_int<32>)raw;
        x[i] = xi;
    }

    // =========================================================================
    // Stage 2 — Boundary B = B_BASE − ((ALPHA × anx_in) >> 10)
    // =========================================================================
    ap_int<32> B = (ap_int<32>)B_BASE
                 - (((ap_int<32>)ALPHA * (ap_int<32>)anx_in) >> 10);

    // =========================================================================
    // Stage 3 — Barrier h = B + W · x
    // UNROLL: fully parallel adder tree; all 16 products computed in one cycle.
    // x[i] is ap_int<16> (signed); W[i] is ap_int<16> (signed).
    // The product (ap_int<16>)×(ap_int<16>) is ap_int<32> — no overflow.
    // =========================================================================
#pragma HLS ARRAY_PARTITION variable=W complete dim=1

    ap_int<32> h = B;

    DOT_LOOP: for (int i = 0; i < DIM; i++)
    {
#pragma HLS UNROLL
        h += x[i] * (ap_int<32>)W[i];
    }

    // =========================================================================
    // Stage 4 — Steering vector u_t
    // UNROLL: 16 parallel multiply-shift-saturate paths.
    // =========================================================================
    ap_int<16> u[DIM];
#pragma HLS ARRAY_PARTITION variable=u complete dim=1

    STEER_LOOP: for (int i = 0; i < DIM; i++)
    {
#pragma HLS UNROLL
        if (h >= 0) {
            u[i] = 0;
        } else {
            u[i] = saturate(((-h) * (ap_int<32>)W[i]) >> 10);
        }
    }

    // =========================================================================
    // Stage 5 — Write 16 words to u_t AXI-Stream.
    // PIPELINE II=1: one write per clock.
    // =========================================================================
    WRITE_LOOP: for (int i = 0; i < DIM; i++)
    {
#pragma HLS PIPELINE II=1
        axis_word_t beat;
        beat.data = u[i];
        beat.keep = (ap_uint<2>)0x3;
        beat.last = (ap_uint<1>)(i == DIM - 1);
        u_t.write(beat);
    }
}
