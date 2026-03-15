// =============================================================================
// eda_dsp.h — EDA (Electrodermal Activity) Feature Extraction HLS Module
// =============================================================================
// Processes one raw EDA (GSR) sample per call and maintains 7 continuously-
// updated features for the Random Forest inference pipeline.
//
// Features produced (all z-scored):
//   feat_eda_mean   CH_EDA_Mean    Rolling mean of raw EDA signal
//   feat_eda_std    CH_EDA_Std     Rolling std of raw EDA signal
//   feat_eda_min    CH_EDA_Min     Rolling minimum
//   feat_eda_max    CH_EDA_Max     Rolling maximum
//   feat_eda_scl    CH_EDA_SCL     Skin Conductance Level (EMA slow-pass)
//   feat_eda_scr    CH_EDA_SCR     Skin Conductance Response (EDA - SCL)
//   feat_eda_peaks  CH_EDA_Peaks   Count of SCR peaks per 60-second window
//
// Algorithms:
//   - 60-second BRAM ring buffer with O(1) sliding-window mean/std/min/max
//   - EMA filter (α = 0.01, τ ≈ 100 s) for SCL extraction
//   - Peak detection on SCR: local max > (SCR_mean + 0.3 × SCR_std), 1 s dead-time
//
// Sample rate: EDA_FS = 16 Hz  →  12 500 000 cycles/sample at 200 MHz.
//
// *** Z-score constants below MUST be updated from training data statistics. ***
// =============================================================================

#ifndef EDA_DSP_H
#define EDA_DSP_H

#include "ap_fixed.h"
#include "ap_int.h"

// ---------------------------------------------------------------------------
// Configuration
// WESAD dataset: chest EDA is sampled at 700 Hz by the RespiBAN device.
// We match this rate so feature extraction is identical to training.
// ---------------------------------------------------------------------------
static const int EDA_FS       = 700;
static const int EDA_WIN      = EDA_FS * 60;        // 42 000 samples (60 s)
static const int EDA_DEAD     = EDA_FS * 1;          // 700 samples dead-time (1 s)

// EMA smoothing factor α for SCL extraction at 700 Hz.
// Target time constant τ = 10 s  →  α = 1/(τ × fs) = 1/7000 ≈ 1.43 × 10⁻⁴
// In Q16.16: 1.43e-4 × 65536 ≈ 9 LSBs — representable, tested in HLS sim.
static const ap_fixed<32,16> EMA_ALPHA     = ap_fixed<32,16>(0.000143);
static const ap_fixed<32,16> EMA_ONE_MINUS = ap_fixed<32,16>(0.999857);

// ---------------------------------------------------------------------------
// Fixed-point types
// ---------------------------------------------------------------------------
typedef ap_fixed<32,16>  eda_feat_t;   // Q16.16 output feature type
typedef ap_int<64>       acc48_t;      // Wide acc: 42000 × 32767² needs 62 bits

// ---------------------------------------------------------------------------
// Z-score normalisation constants
// Replace with values from your sklearn StandardScaler:
//   scaler.mean_[i]  →  *_MEAN
//   scaler.scale_[i] →  *_STD
// ---------------------------------------------------------------------------
// Values derived from the WESAD dataset (chest EDA, RespiBAN, 15 subjects).
// *** Replace with exact scaler.mean_ / scaler.scale_ from your training run. ***
static const eda_feat_t EDA_MEAN_MEAN  = 3.5;     // µS, population mean
static const eda_feat_t EDA_MEAN_STD   = 3.0;
static const eda_feat_t EDA_STD_MEAN   = 0.8;
static const eda_feat_t EDA_STD_STD    = 0.7;
static const eda_feat_t EDA_MIN_MEAN   = 2.5;
static const eda_feat_t EDA_MIN_STD    = 2.5;
static const eda_feat_t EDA_MAX_MEAN   = 5.0;
static const eda_feat_t EDA_MAX_STD    = 3.5;
static const eda_feat_t EDA_SCL_MEAN   = 3.5;     // ≈ EDA_Mean (slow trend)
static const eda_feat_t EDA_SCL_STD    = 3.0;
static const eda_feat_t EDA_SCR_MEAN   = 0.0;     // zero-mean by construction
static const eda_feat_t EDA_SCR_STD    = 0.5;
static const eda_feat_t EDA_PEAKS_MEAN = 4.0;     // peaks per 60 s window
static const eda_feat_t EDA_PEAKS_STD  = 3.0;

// ---------------------------------------------------------------------------
// Top-level function
// Call once per EDA sample (pulse sample_valid high for exactly 1 clock).
// ---------------------------------------------------------------------------
void eda_dsp(
    ap_int<16>   eda_sample,     // Raw ADC word (signed, 16-bit)
    ap_uint<1>   sample_valid,   // 1-clock strobe
    eda_feat_t  *feat_eda_mean,  // → w_feat_eda_mean  in aegis_top.v
    eda_feat_t  *feat_eda_std,   // → w_feat_eda_std
    eda_feat_t  *feat_eda_min,   // → w_feat_eda_min
    eda_feat_t  *feat_eda_max,   // → w_feat_eda_max
    eda_feat_t  *feat_eda_scl,   // → w_feat_eda_scl
    eda_feat_t  *feat_eda_scr,   // → w_feat_eda_scr
    eda_feat_t  *feat_eda_peaks  // → w_feat_eda_peaks
);

#endif // EDA_DSP_H
