// =============================================================================
// eda_dsp.cpp — EDA Feature Extraction HLS Implementation
// =============================================================================

#include "eda_dsp.h"
#include "hls_math.h"       // hls::sqrt() — synthesizable fixed-point CORDIC

// ---------------------------------------------------------------------------
// Helper: z-score (safe against zero std)
// ---------------------------------------------------------------------------
static inline eda_feat_t z_score(eda_feat_t raw,
                                  eda_feat_t mean,
                                  eda_feat_t std_val)
{
#pragma HLS INLINE
    if (std_val <= eda_feat_t(0)) return eda_feat_t(0);
    return (raw - mean) / std_val;
}

static inline eda_feat_t safe_sqrt(eda_feat_t x)
{
#pragma HLS INLINE
    return hls::sqrt(x > eda_feat_t(0) ? x : eda_feat_t(0));
}

// =============================================================================
// eda_dsp — top-level HLS entry point
// =============================================================================
void eda_dsp(
    ap_int<16>   eda_sample,
    ap_uint<1>   sample_valid,
    eda_feat_t  *feat_eda_mean,
    eda_feat_t  *feat_eda_std,
    eda_feat_t  *feat_eda_min,
    eda_feat_t  *feat_eda_max,
    eda_feat_t  *feat_eda_scl,
    eda_feat_t  *feat_eda_scr,
    eda_feat_t  *feat_eda_peaks
)
{
    // ---- HLS interface directives -------------------------------------------
#pragma HLS INTERFACE ap_none port=eda_sample
#pragma HLS INTERFACE ap_none port=sample_valid
#pragma HLS INTERFACE ap_none port=feat_eda_mean
#pragma HLS INTERFACE ap_none port=feat_eda_std
#pragma HLS INTERFACE ap_none port=feat_eda_min
#pragma HLS INTERFACE ap_none port=feat_eda_max
#pragma HLS INTERFACE ap_none port=feat_eda_scl
#pragma HLS INTERFACE ap_none port=feat_eda_scr
#pragma HLS INTERFACE ap_none port=feat_eda_peaks
#pragma HLS INTERFACE ap_ctrl_none port=return

    // At 16 Hz and 200 MHz there are ~12.5 M cycles per sample. No tight II.

    // =========================================================================
    // Static state
    // =========================================================================

    // ---- EDA 60-second ring buffer (960 × 16-bit = 1.9 KB → BRAM) ----------
    static ap_int<16> eda_buf[EDA_WIN];
#pragma HLS BIND_STORAGE variable=eda_buf type=RAM_2P impl=BRAM
    static ap_uint<16> wr_ptr   = 0;
    static ap_uint<16> samp_cnt = 0;    // saturates at EDA_WIN

    // ---- Running raw EDA statistics (O(1) sliding-window) ------------------
    static acc48_t run_sum    = 0;      // Σ eda[i]
    static acc48_t run_sum_sq = 0;      // Σ eda[i]²
    static ap_int<16> run_min = 32767;  // Rolling minimum
    static ap_int<16> run_max = -32768; // Rolling maximum
    // Note: min/max from the full buffer are re-scanned once per new sample
    // only when the outgoing old_sample was the previous extreme. This avoids
    // storing a sorted structure while keeping worst-case latency bounded.
    // At 16 Hz the O(N) re-scan (N=960) takes ~960 cycles — well within budget.

    // ---- EMA (Exponential Moving Average) for SCL extraction ---------------
    // SCL = 0.99 × SCL_prev + 0.01 × EDA_sample    (α = 0.01, τ ≈ 100 s)
    // SCR = EDA_sample − SCL
    static eda_feat_t scl = 0;          // Skin Conductance Level (EMA state)

    // ---- SCR peak detection state ------------------------------------------
    // Running mean and std of SCR stored as a separate ring buffer to keep
    // the peak-detection threshold adaptive and independent from the raw EDA stats.
    static acc48_t scr_sum    = 0;
    static acc48_t scr_sum_sq = 0;

    static ap_uint<16> dead_cnt   = 0;  // Remaining dead-time samples (<=700)
    static ap_uint<8>  peak_cnt   = 0;  // SCR peaks in current window
    static eda_feat_t  prev_scr   = 0;  // Previous SCR value (for local-max check)
    static ap_int<1>   rising     = 0;  // 1 = SCR was rising last sample

    // ---- SCR ring buffer for exact sliding-window SCR stats ----------------
    static ap_int<16> scr_buf[EDA_WIN];
#pragma HLS BIND_STORAGE variable=scr_buf type=RAM_2P impl=BRAM

    // ---- Output registers --------------------------------------------------
    static eda_feat_t out_mean  = 0;
    static eda_feat_t out_std   = 0;
    static eda_feat_t out_min   = 0;
    static eda_feat_t out_max   = 0;
    static eda_feat_t out_scl   = 0;
    static eda_feat_t out_scr   = 0;
    static eda_feat_t out_peaks = 0;

    // =========================================================================
    // Processing — gated by sample_valid strobe
    // =========================================================================
    if (sample_valid) {

        // =====================================================================
        // Stage 1 — EMA filter: SCL and SCR computation
        //   SCL_{n} = (1 - α) × SCL_{n-1} + α × EDA_{n}
        //   SCR_{n} = EDA_{n} − SCL_{n}
        // Both operations are single-cycle multiply-accumulate in ap_fixed.
        // =====================================================================
        eda_feat_t eda_fx = eda_feat_t(eda_sample);
        scl = EMA_ONE_MINUS * scl + EMA_ALPHA * eda_fx;
        eda_feat_t scr = eda_fx - scl;
        ap_int<16> scr_int = (ap_int<16>)scr;  // truncate for BRAM storage

        out_scl = z_score(scl, EDA_SCL_MEAN, EDA_SCL_STD);
        out_scr = z_score(scr, EDA_SCR_MEAN, EDA_SCR_STD);

        // =====================================================================
        // Stage 2 — Slide EDA ring buffer, update running raw EDA statistics
        // =====================================================================
        ap_int<16> old_eda = eda_buf[wr_ptr];
        ap_int<16> old_scr = scr_buf[wr_ptr];

        eda_buf[wr_ptr] = eda_sample;
        scr_buf[wr_ptr] = scr_int;

        wr_ptr = (wr_ptr == (ap_uint<16>)(EDA_WIN - 1))
                     ? (ap_uint<16>)0
                     : (ap_uint<16>)(wr_ptr + 1);

        if (samp_cnt < (ap_uint<16>)EDA_WIN) {
            run_sum    += eda_sample;
            run_sum_sq += (acc48_t)eda_sample * eda_sample;
            scr_sum    += scr_int;
            scr_sum_sq += (acc48_t)scr_int * scr_int;
            samp_cnt++;
        } else {
            // O(1) sliding-window update
            run_sum    += (acc48_t)eda_sample - old_eda;
            run_sum_sq += (acc48_t)eda_sample * eda_sample
                        - (acc48_t)old_eda    * old_eda;
            scr_sum    += (acc48_t)scr_int - old_scr;
            scr_sum_sq += (acc48_t)scr_int * scr_int
                        - (acc48_t)old_scr * old_scr;
        }

        ap_uint<16> N = (samp_cnt < (ap_uint<16>)EDA_WIN)
                            ? samp_cnt : (ap_uint<16>)EDA_WIN;

        // =====================================================================
        // Stage 3 — EDA mean, std, min, max
        // =====================================================================
        eda_feat_t eda_mean = eda_feat_t(run_sum)    / eda_feat_t(N);
        eda_feat_t eda_var  = eda_feat_t(run_sum_sq) / eda_feat_t(N)
                            - eda_mean * eda_mean;
        eda_feat_t eda_std  = safe_sqrt(eda_var);

        out_mean = z_score(eda_mean, EDA_MEAN_MEAN, EDA_MEAN_STD);
        out_std  = z_score(eda_std,  EDA_STD_MEAN,  EDA_STD_STD);

        // Min/max: update incrementally on insertion; re-scan only when the
        // outgoing old sample was the current extreme (rare worst-case O(N)).
        if (eda_sample < run_min) run_min = eda_sample;
        if (eda_sample > run_max) run_max = eda_sample;

        if (samp_cnt >= (ap_uint<16>)EDA_WIN) {
            // If old_eda was the extreme, rescan the now-updated buffer.
            if (old_eda <= run_min || old_eda >= run_max) {
                ap_int<16> scan_min = eda_buf[0];
                ap_int<16> scan_max = eda_buf[0];
                RESCAN: for (int i = 1; i < EDA_WIN; i++) {
#pragma HLS PIPELINE
                    ap_int<16> v = eda_buf[i];
                    if (v < scan_min) scan_min = v;
                    if (v > scan_max) scan_max = v;
                }
                run_min = scan_min;
                run_max = scan_max;
            }
        }

        out_min = z_score(eda_feat_t(run_min), EDA_MIN_MEAN, EDA_MIN_STD);
        out_max = z_score(eda_feat_t(run_max), EDA_MAX_MEAN, EDA_MAX_STD);

        // =====================================================================
        // Stage 4 — SCR peak detection
        //   Adaptive threshold = SCR_mean + 0.3 × SCR_std
        //   A peak is a positive local maximum exceeding the threshold.
        //   1-second dead-time (EDA_DEAD = 16 samples) prevents double-count.
        //
        //   Local max detection: SCR rose last sample and is now falling,
        //   while the previous value exceeded the threshold.
        // =====================================================================
        eda_feat_t scr_mean = eda_feat_t(scr_sum)    / eda_feat_t(N);
        eda_feat_t scr_std  = safe_sqrt(
                              eda_feat_t(scr_sum_sq) / eda_feat_t(N)
                              - scr_mean * scr_mean);
        eda_feat_t peak_thr = scr_mean + eda_feat_t(0.3) * scr_std;

        if (dead_cnt > 0) {
            dead_cnt--;
        } else {
            // Detect falling edge after a peak (local maximum in SCR)
            ap_int<1> currently_rising = (scr > prev_scr) ? (ap_int<1>)1
                                                           : (ap_int<1>)0;
            if (rising && !currently_rising && prev_scr > peak_thr) {
                peak_cnt++;
                dead_cnt = (ap_uint<8>)EDA_DEAD;
            }
            rising = currently_rising;
        }
        prev_scr = scr;

        // EDA_Peaks: raw count z-scored (the RF sees peaks per 60-second window)
        out_peaks = z_score(eda_feat_t(peak_cnt), EDA_PEAKS_MEAN, EDA_PEAKS_STD);
    }

    // =========================================================================
    // Drive outputs from registered state
    // =========================================================================
    *feat_eda_mean  = out_mean;
    *feat_eda_std   = out_std;
    *feat_eda_min   = out_min;
    *feat_eda_max   = out_max;
    *feat_eda_scl   = out_scl;
    *feat_eda_scr   = out_scr;
    *feat_eda_peaks = out_peaks;
}
