// =============================================================================
// rf_anxiety.cpp — HLS Random Forest Inference Wrapper Implementation
// =============================================================================
// Calls the pre-trained 30-tree Random Forest and maps its probability output
// [0.0, 1.0] onto the integer anxiety_level bus [0, 1024].
//
// Fixed-point pipeline:
//   1. Pack 12 ap_fixed<32,16> feature scalars into a local array.
//   2. Call predict() from rf_biological_arousal_fixed.h.
//      — predict() traverses 30 decision trees, accumulates scores, and returns
//        total_score / 30  ∈ [0, 1] using purely ap_fixed<32,16> arithmetic.
//        No float hardware is instantiated; all comparisons and arithmetic are
//        mapped to integer DSP/LUT paths by Vitis HLS.
//   3. Multiply by 1024 (left-shift by 10) and truncate to ap_int<16>.
//
// HLS directives used:
//   INTERFACE ap_none  — all ports are plain wires (no bus protocol).
//   INTERFACE ap_ctrl_none — no ap_start/done/idle; module runs continuously.
//   PIPELINE II=1      — accept new feature vector every clock cycle.
//   ARRAY_PARTITION    — decompose the 12-element array so HLS can schedule
//                        all tree comparisons without BRAM port conflicts.
// =============================================================================

#include "rf_anxiety.h"

// Include the fixed-point RF header. This header was generated from
// rf_biological_arousal.h by substituting:
//     float  →  ap_fixed<32,16>
// using:  perl -pe 's/\bfloat\b/ap_fixed<32,16>/g' \
//              rf_biological_arousal.h > rf_biological_arousal_fixed.h
//
// The function signature after substitution:
//     ap_fixed<32,16> predict(ap_fixed<32,16>* features)
// and returns total_score / 30.0f  ∈ [0, 1].
// Float literal constants (e.g., 3.0616f) are converted to Q16.16 by the
// HLS compiler at elaboration time — zero float logic synthesised.
#include "rf_biological_arousal_fixed.h"

// =============================================================================
// rf_anxiety — top-level HLS entry point
// =============================================================================
void rf_anxiety(
    rf_feat_t   feat_ecg_hr,      // [0]  CH_ECG_HR    Heart Rate z-score
    rf_feat_t   feat_ecg_std,     // [1]  CH_ECG_Std   ECG std-dev z-score
    rf_feat_t   feat_ecg_rmssd,   // [2]  CH_ECG_RMSSD RMSSD z-score
    rf_feat_t   feat_ecg_pnn50,   // [3]  CH_ECG_pNN50 pNN50 z-score
    rf_feat_t   feat_ecg_sdnn,    // [4]  CH_ECG_SDNN  SDNN z-score
    rf_feat_t   feat_eda_mean,    // [5]  CH_EDA_Mean  EDA mean z-score
    rf_feat_t   feat_eda_std,     // [6]  CH_EDA_Std   EDA std-dev z-score
    rf_feat_t   feat_eda_min,     // [7]  CH_EDA_Min   EDA min z-score
    rf_feat_t   feat_eda_max,     // [8]  CH_EDA_Max   EDA max z-score
    rf_feat_t   feat_eda_scl,     // [9]  CH_EDA_SCL   SCL z-score
    rf_feat_t   feat_eda_scr,     // [10] CH_EDA_SCR   SCR z-score
    rf_feat_t   feat_eda_peaks,   // [11] CH_EDA_Peaks EDA peak count z-score
    ap_int<16>  *anxiety_level    // Output: [0, 1024] — 0 = calm, 1024 = max stress
)
{
    // -------------------------------------------------------------------------
    // HLS Interface Directives
    // -------------------------------------------------------------------------
#pragma HLS INTERFACE ap_none port=feat_ecg_hr
#pragma HLS INTERFACE ap_none port=feat_ecg_std
#pragma HLS INTERFACE ap_none port=feat_ecg_rmssd
#pragma HLS INTERFACE ap_none port=feat_ecg_pnn50
#pragma HLS INTERFACE ap_none port=feat_ecg_sdnn
#pragma HLS INTERFACE ap_none port=feat_eda_mean
#pragma HLS INTERFACE ap_none port=feat_eda_std
#pragma HLS INTERFACE ap_none port=feat_eda_min
#pragma HLS INTERFACE ap_none port=feat_eda_max
#pragma HLS INTERFACE ap_none port=feat_eda_scl
#pragma HLS INTERFACE ap_none port=feat_eda_scr
#pragma HLS INTERFACE ap_none port=feat_eda_peaks
#pragma HLS INTERFACE ap_none port=anxiety_level
#pragma HLS INTERFACE ap_ctrl_none port=return

    // Pipeline the full function body — new feature vector accepted every cycle.
#pragma HLS PIPELINE II=1

    // -------------------------------------------------------------------------
    // Step 1: Pack scalar inputs into a flat array.
    // ARRAY_PARTITION complete ensures each element has its own register,
    // eliminating BRAM read-port bottlenecks when predict() accesses multiple
    // features in parallel across the 30 trees.
    // -------------------------------------------------------------------------
    rf_feat_t features[RF_N_FEATURES];
#pragma HLS ARRAY_PARTITION variable=features complete dim=1

    features[0]  = feat_ecg_hr;
    features[1]  = feat_ecg_std;
    features[2]  = feat_ecg_rmssd;
    features[3]  = feat_ecg_pnn50;
    features[4]  = feat_ecg_sdnn;
    features[5]  = feat_eda_mean;
    features[6]  = feat_eda_std;
    features[7]  = feat_eda_min;
    features[8]  = feat_eda_max;
    features[9]  = feat_eda_scl;
    features[10] = feat_eda_scr;
    features[11] = feat_eda_peaks;

    // -------------------------------------------------------------------------
    // Step 2: Run the Random Forest inference.
    // predict() traverses 30 trees and returns a Q16.16 value in [0, 1]:
    //   0.0  → no biological arousal detected (calm state)
    //   1.0  → maximum arousal / stress detected
    // All arithmetic inside predict() is ap_fixed<32,16> — no float hardware.
    // -------------------------------------------------------------------------
    rf_feat_t score = predict(features);

    // -------------------------------------------------------------------------
    // Step 3: Scale [0.0, 1.0] → [0, 1024] using a fixed-point multiply.
    //
    //   anxiety = round(score × 1024)
    //
    // score × 1024 in Q16.16:
    //   e.g. score = 0.75  (Q16.16: 0x0000C000)
    //        × 1024        → 768.0 (Q16.16: 0x03000000)
    //   truncated to ap_int<16>: 768
    //
    // Saturate to [0, 1024] in case of rounding artefacts above 1.0.
    // -------------------------------------------------------------------------
    rf_feat_t scaled = score * rf_feat_t(ANXIETY_MAX);
    ap_int<32> raw   = (ap_int<32>)scaled;      // drop fractional bits

    if      (raw < 0)            *anxiety_level = 0;
    else if (raw > ANXIETY_MAX)  *anxiety_level = (ap_int<16>)ANXIETY_MAX;
    else                         *anxiety_level = (ap_int<16>)raw;
}
