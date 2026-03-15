// =============================================================================
// ecg_dsp.cpp — ECG Feature Extraction HLS Implementation
// =============================================================================

#include "ecg_dsp.h"
#include "hls_math.h"       // hls::sqrt() — fixed-point CORDIC, no float hw

// ---------------------------------------------------------------------------
// Helper: z-score a raw feature value.
// Returns (raw - mean) / std. Outputs 0 if std == 0 (prevents div-by-zero).
// ---------------------------------------------------------------------------
static inline ecg_feat_t z_score(ecg_feat_t raw,
                                  ecg_feat_t mean,
                                  ecg_feat_t std_val)
{
#pragma HLS INLINE
    if (std_val <= ecg_feat_t(0)) return ecg_feat_t(0);
    return (raw - mean) / std_val;
}

// ---------------------------------------------------------------------------
// Helper: safe fixed-point sqrt (clamps negative values to 0 before sqrt).
// ---------------------------------------------------------------------------
static inline ecg_feat_t safe_sqrt(ecg_feat_t x)
{
#pragma HLS INLINE
    return hls::sqrt(x > ecg_feat_t(0) ? x : ecg_feat_t(0));
}

// =============================================================================
// ecg_dsp — top-level HLS entry point
// =============================================================================
void ecg_dsp(
    ap_int<16>   ecg_sample,
    ap_uint<1>   sample_valid,
    ecg_feat_t  *feat_hr,
    ecg_feat_t  *feat_std,
    ecg_feat_t  *feat_rmssd,
    ecg_feat_t  *feat_pnn50,
    ecg_feat_t  *feat_sdnn
)
{
    // ---- HLS interface directives -------------------------------------------
#pragma HLS INTERFACE ap_none port=ecg_sample
#pragma HLS INTERFACE ap_none port=sample_valid
#pragma HLS INTERFACE ap_none port=feat_hr
#pragma HLS INTERFACE ap_none port=feat_std
#pragma HLS INTERFACE ap_none port=feat_rmssd
#pragma HLS INTERFACE ap_none port=feat_pnn50
#pragma HLS INTERFACE ap_none port=feat_sdnn
#pragma HLS INTERFACE ap_ctrl_none port=return

    // At 700 Hz and 200 MHz the function has ~285 000 cycles per sample.
    // No II=1 required; HLS schedules arithmetic freely within the gap.

    // =========================================================================
    // Static state — synthesised as registers or BRAM by Vitis HLS
    // =========================================================================

    // ---- ECG 60-second ring buffer (42 000 × 16-bit = 84 KB → BRAM) --------
    static ap_int<16> ecg_buf[ECG_WIN];
#pragma HLS BIND_STORAGE variable=ecg_buf type=RAM_2P impl=BRAM
    static ap_uint<16> wr_ptr    = 0;
    static ap_uint<32> samp_cnt  = 0;   // saturates at ECG_WIN when full

    // ---- Running signal statistics (sliding-window O(1) update) -------------
    static acc64_t run_sum    = 0;      // Σ x[i]
    static acc64_t run_sum_sq = 0;      // Σ x[i]²

    // ---- R-peak detection ---------------------------------------------------
    static ap_uint<16> dead_cnt    = 0; // Remaining dead-time samples
    static ap_uint<32> sample_idx  = 0; // Absolute sample counter
    static ap_uint<32> last_peak   = 0; // sample_idx at last R-peak

    // ---- IBI circular buffer (200 × 16-bit → register file) ----------------
    static ap_int<16> ibi_buf[MAX_BEATS];
#pragma HLS BIND_STORAGE variable=ibi_buf type=RAM_2P impl=BRAM
    static ap_uint<8>  ibi_wr      = 0; // IBI write pointer
    static ap_uint<8>  ibi_cnt     = 0; // valid IBIs in window (≤ MAX_BEATS)
    static ap_int<16>  prev_ibi    = 0; // previous IBI (for successive diffs)

    // ---- Running IBI statistics (updated per beat, O(1)) -------------------
    static acc64_t ibi_sum       = 0;  // Σ IBI[i]
    static acc64_t ibi_sum_sq    = 0;  // Σ IBI[i]²  (→ SDNN)
    static acc64_t diff_sum_sq   = 0;  // Σ (IBI[i] - IBI[i-1])²  (→ RMSSD)
    static ap_uint<8>  diff_cnt  = 0;  // valid successive pairs
    static ap_uint<8>  pnn50_cnt = 0;  // pairs with |diff| > 50 ms

    // ---- Output registers (hold last valid computation) ---------------------
    static ecg_feat_t out_hr    = 0;
    static ecg_feat_t out_std   = 0;
    static ecg_feat_t out_rmssd = 0;
    static ecg_feat_t out_pnn50 = 0;
    static ecg_feat_t out_sdnn  = 0;

    // =========================================================================
    // Processing — gated by sample_valid strobe
    // =========================================================================
    if (sample_valid) {

        // =====================================================================
        // Stage 1 — Slide ring buffer, update running signal statistics
        // =====================================================================
        ap_int<16> old_samp = ecg_buf[wr_ptr];
        ecg_buf[wr_ptr]     = ecg_sample;
        wr_ptr = (wr_ptr == (ap_uint<16>)(ECG_WIN - 1))
                     ? (ap_uint<16>)0
                     : (ap_uint<16>)(wr_ptr + 1);

        if (samp_cnt < (ap_uint<32>)ECG_WIN) {
            // Warmup: accumulate without subtracting (buffer not yet full)
            run_sum    += ecg_sample;
            run_sum_sq += (acc64_t)ecg_sample * ecg_sample;
            samp_cnt++;
        } else {
            // Steady-state: O(1) sliding-window update
            run_sum    += (acc64_t)ecg_sample - old_samp;
            run_sum_sq += (acc64_t)ecg_sample * ecg_sample
                        - (acc64_t)old_samp   * old_samp;
        }

        ap_uint<32> N = (samp_cnt < (ap_uint<32>)ECG_WIN) ? samp_cnt
                                                           : (ap_uint<32>)ECG_WIN;

        // =====================================================================
        // Stage 2 — Signal mean & std (used for threshold and ECG_Std feature)
        // =====================================================================
        ecg_feat_t sig_mean = ecg_feat_t(run_sum)    / ecg_feat_t(N);
        ecg_feat_t sig_var  = ecg_feat_t(run_sum_sq) / ecg_feat_t(N)
                            - sig_mean * sig_mean;
        ecg_feat_t sig_std  = safe_sqrt(sig_var);

        out_std = z_score(sig_std, ECG_STD_MEAN, ECG_STD_STD);

        // =====================================================================
        // Stage 3 — R-peak detection (threshold = mean + 1.5 × σ, dead-time)
        // =====================================================================
        ecg_feat_t threshold = sig_mean + ecg_feat_t(1.5) * sig_std;

        if (dead_cnt > 0) {
            dead_cnt--;
        } else if (ecg_feat_t(ecg_sample) > threshold) {
            // ---- R-peak confirmed -------------------------------------------
            if (last_peak > 0) {
                // IBI in ms = sample_diff × 1000 / ECG_FS
                ap_uint<32> sample_diff = sample_idx - last_peak;
                ap_int<16>  ibi_ms = (ap_int<16>)
                    ((sample_diff * (ap_uint<32>)1000) / (ap_uint<32>)ECG_FS);

                // Slide IBI circular buffer
                ap_int<16> old_ibi = ibi_buf[ibi_wr];
                ibi_buf[ibi_wr]    = ibi_ms;
                ibi_wr = (ibi_wr == (ap_uint<8>)(MAX_BEATS - 1))
                             ? (ap_uint<8>)0
                             : (ap_uint<8>)(ibi_wr + 1);

                if (ibi_cnt < (ap_uint<8>)MAX_BEATS) {
                    // Buffer not yet full: just add
                    ibi_sum    += ibi_ms;
                    ibi_sum_sq += (acc64_t)ibi_ms * ibi_ms;
                    ibi_cnt++;
                } else {
                    // Full: subtract oldest, add newest (O(1) slide)
                    ibi_sum    += (acc64_t)ibi_ms - old_ibi;
                    ibi_sum_sq += (acc64_t)ibi_ms * ibi_ms
                                - (acc64_t)old_ibi * old_ibi;
                }

                // ---- Successive-difference metrics (RMSSD, pNN50) ----------
                if (prev_ibi > 0) {
                    ap_int<16> diff = ibi_ms - prev_ibi;
                    diff_sum_sq += (acc64_t)diff * diff;
                    if (diff < 0) diff = -diff;        // |diff|
                    if (diff > (ap_int<16>)50) pnn50_cnt++;
                    if (diff_cnt < (ap_uint<8>)MAX_DIFFS) diff_cnt++;
                }
                prev_ibi = ibi_ms;
            }
            last_peak = sample_idx;
            dead_cnt  = (ap_uint<16>)DEAD_SAMP;
        }

        // =====================================================================
        // Stage 4 — Compute HRV output features from accumulated IBI stats
        // =====================================================================
        if (ibi_cnt >= (ap_uint<8>)2) {

            // --- Heart Rate [BPM] = 60000 / mean(IBI) -----------------------
            ecg_feat_t mean_ibi = ecg_feat_t(ibi_sum) / ecg_feat_t(ibi_cnt);
            // Explicit guard: avoid division by zero; both branches typed identically.
            ecg_feat_t hr_raw;
            if (mean_ibi > ecg_feat_t(0))
                hr_raw = ecg_feat_t(60000) / mean_ibi;
            else
                hr_raw = ecg_feat_t(0);
            out_hr = z_score(hr_raw, ECG_HR_MEAN, ECG_HR_STD);

            // --- SDNN [ms] = sqrt( var(IBI) ) --------------------------------
            ecg_feat_t ibi_var = ecg_feat_t(ibi_sum_sq) / ecg_feat_t(ibi_cnt)
                               - mean_ibi * mean_ibi;
            out_sdnn = z_score(safe_sqrt(ibi_var), ECG_SDNN_MEAN, ECG_SDNN_STD);

            // --- RMSSD [ms] = sqrt( mean(successive_diffs²) ) ---------------
            if (diff_cnt > 0) {
                ecg_feat_t rmssd_raw = safe_sqrt(
                    ecg_feat_t(diff_sum_sq) / ecg_feat_t(diff_cnt));
                out_rmssd = z_score(rmssd_raw, ECG_RMSSD_MEAN, ECG_RMSSD_STD);

                // --- pNN50 [fraction] = pnn50_count / diff_count -------------
                ecg_feat_t pnn50_raw = ecg_feat_t(pnn50_cnt) / ecg_feat_t(diff_cnt);
                out_pnn50 = z_score(pnn50_raw, ECG_PNN50_MEAN, ECG_PNN50_STD);
            }
        }

        sample_idx++;
    }

    // =========================================================================
    // Drive outputs from registered state — always valid (last known value)
    // =========================================================================
    *feat_hr    = out_hr;
    *feat_std   = out_std;
    *feat_rmssd = out_rmssd;
    *feat_pnn50 = out_pnn50;
    *feat_sdnn  = out_sdnn;
}
