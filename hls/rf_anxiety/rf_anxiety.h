// =============================================================================
// rf_anxiety.h — HLS Random Forest Inference Wrapper
// =============================================================================
// Wraps the pre-trained 30-tree Random Forest (rf_biological_arousal_fixed.h)
// with a clean fixed-point HLS interface.
//
// Inputs:  12 z-scored physiological features as ap_fixed<32,16> (Q16.16).
// Output:  anxiety_level as ap_int<16>, range [0, 1024].
//
// Feature index map (must match rf_biological_arousal.h header comment):
//   [0]  CH_ECG_HR      Heart Rate (BPM, z-scored)
//   [1]  CH_ECG_Std     ECG signal std deviation (z-scored)
//   [2]  CH_ECG_RMSSD   Root Mean Square Successive Differences (z-scored)
//   [3]  CH_ECG_pNN50   Fraction of IBI diffs > 50ms (z-scored)
//   [4]  CH_ECG_SDNN    Std deviation of IBI array (z-scored)
//   [5]  CH_EDA_Mean    EDA mean level (z-scored)
//   [6]  CH_EDA_Std     EDA std deviation (z-scored)
//   [7]  CH_EDA_Min     EDA minimum (z-scored)
//   [8]  CH_EDA_Max     EDA maximum (z-scored)
//   [9]  CH_EDA_SCL     Skin Conductance Level (z-scored)
//   [10] CH_EDA_SCR     Skin Conductance Response amplitude (z-scored)
//   [11] CH_EDA_Peaks   Number of SCR peaks (z-scored)
//
// Fixed-point conversion note:
//   rf_biological_arousal_fixed.h was auto-generated from rf_biological_arousal.h
//   by replacing every occurrence of `float` with `ap_fixed<32,16>`.
//   Float literals (e.g., 3.0616f) are accepted by ap_fixed constructors and
//   are converted to Q16.16 by the HLS compiler at elaboration time — no
//   float arithmetic is synthesised.
// =============================================================================

#ifndef RF_ANXIETY_H
#define RF_ANXIETY_H

#include "ap_fixed.h"
#include "ap_int.h"

// Q16.16 fixed-point type used for all sensor features and RF thresholds.
// Range: [-32768, 32768), precision: ~1.5e-5 — adequate for z-scored features.
typedef ap_fixed<32,16> rf_feat_t;

static const int RF_N_FEATURES  = 12;
static const int RF_N_TREES     = 30;
static const int ANXIETY_MAX    = 1024;     // Output full-scale (maps to RF score = 1.0)

// =============================================================================
// Top-level HLS function declaration
// All ports are ap_none (direct wires) — matches aegis_top.v port map.
// =============================================================================
void rf_anxiety(
    rf_feat_t   feat_ecg_hr,
    rf_feat_t   feat_ecg_std,
    rf_feat_t   feat_ecg_rmssd,
    rf_feat_t   feat_ecg_pnn50,
    rf_feat_t   feat_ecg_sdnn,
    rf_feat_t   feat_eda_mean,
    rf_feat_t   feat_eda_std,
    rf_feat_t   feat_eda_min,
    rf_feat_t   feat_eda_max,
    rf_feat_t   feat_eda_scl,
    rf_feat_t   feat_eda_scr,
    rf_feat_t   feat_eda_peaks,
    ap_int<16>  *anxiety_level
);

#endif // RF_ANXIETY_H
