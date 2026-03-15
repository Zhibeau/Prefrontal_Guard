#ifndef AEGIS_SHIELD_H
#define AEGIS_SHIELD_H

#include "ap_int.h"
#include "hls_stream.h"
#include "ap_axi_sdata.h"

// ---------------------------------------------------------------------------
// Dimensions & CC-CBF Constants
// All values are fixed-point integers — NO floats permitted.
// ---------------------------------------------------------------------------
static const int DIM    = 16;
static const int B_BASE = 5000;   // Base safety boundary (fixed-point)
static const int ALPHA  = 6000;   // Physiological gain (fixed-point, Q10)

// ---------------------------------------------------------------------------
// Dummy weight vector W[16] — replace with trained coefficients.
// Values are Q10 fixed-point (1.0 ≈ 1024).
// ---------------------------------------------------------------------------
static const ap_int<16> W[DIM] = {
    64,  128,  96, 112,
    80,   72, 104,  88,
   120,   60,  92, 116,
    76,   84, 100,  68
};

// ---------------------------------------------------------------------------
// AXI-Stream types — use hls::axis<ap_int<16>> NOT ap_axiu<16>.
//
// ap_axiu<16,...> has ap_uint<16> TDATA → HLS infers unsigned pipeline regs.
// When x_t contains negative values (-100 = 0xFF9C), unsigned regs treat
// them as +65436, making h always positive → u_t always zero (wrong).
//
// hls::axis<ap_int<16>,...> has SIGNED TDATA → HLS infers signed regs →
// sign is preserved through all pipeline stages → correct dot product.
// ---------------------------------------------------------------------------
typedef hls::axis<ap_int<16>, 0, 0, 0> axis_word_t;
typedef hls::stream<axis_word_t>        axis_stream_t;

// ---------------------------------------------------------------------------
// Top-level function declaration
// ---------------------------------------------------------------------------
void aegis_shield(
    ap_int<16>    anx_in,
    axis_stream_t &x_t,
    axis_stream_t &u_t
);

#endif // AEGIS_SHIELD_H
