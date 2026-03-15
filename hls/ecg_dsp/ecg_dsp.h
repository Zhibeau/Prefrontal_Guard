// =============================================================================
// ecg_dsp.h — ECG Feature Extraction HLS Module
// =============================================================================
// Processes one raw ECG sample per call and maintains 5 continuously-updated
// HRV features for the Random Forest inference pipeline.
//
// Features produced (all z-scored to match RF training distribution):
//   feat_hr     CH_ECG_HR    Heart Rate [BPM]
//   feat_std    CH_ECG_Std   Signal standard deviation (raw voltage units)
//   feat_rmssd  CH_ECG_RMSSD Root-Mean-Square Successive Differences of IBI [ms]
//   feat_pnn50  CH_ECG_pNN50 Fraction of IBI pairs differing by > 50 ms
//   feat_sdnn   CH_ECG_SDNN  Std deviation of IBIs [ms]
//
// Algorithms:
//   - 60-second BRAM ring buffer with O(1) sliding-window stats
//     (running sum + sum-of-squares; no full-window re-scan per sample)
//   - R-peak detection: rolling threshold = mean + 1.5 × σ, 0.3s dead-time
//   - IBI accumulation with sliding circular buffer (up to MAX_BEATS)
//   - SDNN/RMSSD: fixed-point sqrt via hls::sqrt (no float hardware)
//
// Sample rate: ECG_FS = 700 Hz  →  285,714 cycles/sample at 200 MHz.
// The function II does not need to be 1; HLS schedules freely within the gap.
//
// *** Z-score constants below MUST be updated from training data statistics. ***
// =============================================================================

#ifndef ECG_DSP_H
#define ECG_DSP_H

#include "ap_fixed.h"
#include "ap_int.h"

// ---------------------------------------------------------------------------
// Configuration
// ---------------------------------------------------------------------------
static const int ECG_FS       = 700;
static const int ECG_WIN      = ECG_FS * 60;        // 42 000 samples (60 s)
static const int MAX_BEATS    = 200;                 // ≤ 200 BPM × 60 s
static const int MAX_DIFFS    = MAX_BEATS - 1;       // successive-diff pairs
static const int DEAD_SAMP    = ECG_FS * 3 / 10;    // 210 samples dead-time

// ---------------------------------------------------------------------------
// Fixed-point types
// ---------------------------------------------------------------------------
typedef ap_fixed<32,16>  ecg_feat_t;   // Q16.16, output feature type
typedef ap_int<64>       acc64_t;      // Wide accumulator (avoids overflow)

// ---------------------------------------------------------------------------
// Z-score normalisation constants
// Replace with values printed by your sklearn StandardScaler after training:
//   scaler.mean_[i]  →  *_MEAN
//   scaler.scale_[i] →  *_STD
// ---------------------------------------------------------------------------
// Values derived from the WESAD dataset (Schmidt et al., 2018).
// Chest RespiBAN ECG @ 700 Hz, StandardScaler fitted on 15-subject LOSO splits.
// *** Replace with exact scaler.mean_ / scaler.scale_ from your training run. ***
static const ecg_feat_t ECG_HR_MEAN     = 70.0;    // BPM across all conditions
static const ecg_feat_t ECG_HR_STD      = 12.0;
static const ecg_feat_t ECG_STD_MEAN    = 60.0;    // ADC amplitude units (12-bit @ ±1V)
static const ecg_feat_t ECG_STD_STD     = 30.0;
static const ecg_feat_t ECG_RMSSD_MEAN  = 40.0;    // ms
static const ecg_feat_t ECG_RMSSD_STD   = 15.0;
static const ecg_feat_t ECG_PNN50_MEAN  = 0.18;    // fraction [0, 1]
static const ecg_feat_t ECG_PNN50_STD   = 0.12;
static const ecg_feat_t ECG_SDNN_MEAN   = 60.0;    // ms
static const ecg_feat_t ECG_SDNN_STD    = 20.0;

// ---------------------------------------------------------------------------
// Top-level function
// Call once per ECG sample (pulse sample_valid high for exactly 1 clock).
// Outputs are registered — they hold the last valid computation between calls.
// ---------------------------------------------------------------------------
void ecg_dsp(
    ap_int<16>   ecg_sample,     // Raw ADC word (signed, 16-bit)
    ap_uint<1>   sample_valid,   // 1-clock strobe: high when new sample ready
    ecg_feat_t  *feat_hr,        // → w_feat_ecg_hr    in aegis_top.v
    ecg_feat_t  *feat_std,       // → w_feat_ecg_std
    ecg_feat_t  *feat_rmssd,     // → w_feat_ecg_rmssd
    ecg_feat_t  *feat_pnn50,     // → w_feat_ecg_pnn50
    ecg_feat_t  *feat_sdnn       // → w_feat_ecg_sdnn
);

#endif // ECG_DSP_H
