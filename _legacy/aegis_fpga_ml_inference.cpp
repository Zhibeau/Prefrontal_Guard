#include <hls_math.h>
#include "ap_fixed.h"

// System and Configuration definitions based on Python Training & Guide
#define CHEST_SR 700
#define WINDOW_SEC 60
#define BUFFER_SIZE (WINDOW_SEC * CHEST_SR) // 42000
#define MIN_DIST_SAMPLES (0.3f * CHEST_SR)   // 210

// Define fixed-point types to minimize LUT/FF usage on Artix-7
typedef ap_fixed<16, 8> data_t;
typedef ap_fixed<32, 16> calc_t;

// External predictive model function (Random Forest exported to C header)
#include "rf_biological_arousal.h"

/**
 * Top-Level FPGA module for Aegis-Chip Safety ML pipeline
 * Handles streaming sensory entry, sliding window DSP processing, 
 * random forest ML inference, and safe action triggering into Vitis HLS.
 * 
 * @param ecg_in Continuous ECG data stream point (700Hz)
 * @param eda_in Continuous EDA data stream point (700Hz or 4Hz mapped)
 * @param trigger_inference Trigger the computationally expensive feature extraction and inference
 * @param interrupt_trigger High if safety threshold >0.8 is breached
 * @param risk_score Output float indicating current stress/arousal prediction
 */
void aegis_ml_inference_pipeline(
    data_t ecg_in, 
    data_t eda_in, 
    bool trigger_inference, 
    bool &interrupt_trigger, 
    float &risk_score
) {
    // Top-level pragmas mapping to AXI-Stream and AXI-Lite interfaces
    #pragma HLS INTERFACE axis port=ecg_in
    #pragma HLS INTERFACE axis port=eda_in
    #pragma HLS INTERFACE s_axilite port=trigger_inference
    #pragma HLS INTERFACE s_axilite port=interrupt_trigger
    #pragma HLS INTERFACE s_axilite port=risk_score
    #pragma HLS INTERFACE s_axilite port=return

    // Static circular buffers allocated in BRAM
    static data_t ecg_buffer[BUFFER_SIZE];
    static data_t eda_buffer[BUFFER_SIZE];
    static int head = 0;

    // 1. Uninterrupted DSP Pipeline: Streaming Buffer
    // Always insert incoming data point into circular buffer to track 60-seconds
    ecg_buffer[head] = ecg_in;
    eda_buffer[head] = eda_in;
    
    head++;
    if (head >= BUFFER_SIZE) {
        head = 0;
    }

    // Decoupled inference: if un-triggered, just return and keep receiving data
    if (!trigger_inference) {
        return;
    }

    // 2. Continuous Feature Extraction Execution over 60s sliding window
    float features[12] = {0.0f};

    calc_t eda_sum = 0;
    calc_t eda_min = 999999;
    calc_t eda_max = -999999;
    calc_t ecg_sum = 0;

    // Pass 1: Mean, Min, Max
    pass1: for (int i = 0; i < BUFFER_SIZE; i++) {
        #pragma HLS PIPELINE II=1
        int idx = head + i;
        if (idx >= BUFFER_SIZE) idx -= BUFFER_SIZE;
        
        data_t eda_val = eda_buffer[idx];
        data_t ecg_val = ecg_buffer[idx];
        
        eda_sum += eda_val;
        if (eda_val < eda_min) eda_min = eda_val;
        if (eda_val > eda_max) eda_max = eda_val;
        ecg_sum += ecg_val;
    }

    calc_t eda_mean = eda_sum / BUFFER_SIZE;
    calc_t ecg_mean = ecg_sum / BUFFER_SIZE;

    // Pass 2: Filter EMA and compute basic Std Dev
    calc_t eda_var_sum = 0;
    calc_t ecg_var_sum = 0;
    
    calc_t scl_val = eda_buffer[head]; // Start filtering from oldest data natively
    calc_t scl_sum = 0;
    
    static calc_t scr_buffer[BUFFER_SIZE]; // Can be mapped to BRAM
    
    pass2: for (int i = 0; i < BUFFER_SIZE; i++) {
        #pragma HLS PIPELINE II=1
        int idx = head + i;
        if (idx >= BUFFER_SIZE) idx -= BUFFER_SIZE;
        
        data_t eda_val = eda_buffer[idx];
        data_t ecg_val = ecg_buffer[idx];
        
        calc_t eda_diff = eda_val - eda_mean;
        eda_var_sum += eda_diff * eda_diff;
        
        calc_t ecg_diff = ecg_val - ecg_mean;
        ecg_var_sum += ecg_diff * ecg_diff;
        
        // Exponential Moving Average filter for Skin Conductance Level (SCL) (alpha = 0.01)
        scl_val = (calc_t)0.01f * eda_val + (calc_t)0.99f * scl_val;
        scl_sum += scl_val;
        
        calc_t scr_val = eda_val - scl_val;
        scr_buffer[i] = scr_val; 
    }

    calc_t eda_std = (calc_t)hls::sqrt((float)(eda_var_sum / BUFFER_SIZE));
    calc_t ecg_std = (calc_t)hls::sqrt((float)(ecg_var_sum / BUFFER_SIZE));
    calc_t scl_mean = scl_sum / BUFFER_SIZE;

    // Pass 3: Skin Conductance Responses (SCR) Variance Metrics
    calc_t scr_sum = 0;
    scr_pass1: for (int i = 0; i < BUFFER_SIZE; i++) {
        #pragma HLS PIPELINE II=1
        scr_sum += scr_buffer[i];
    }
    calc_t scr_mean = scr_sum / BUFFER_SIZE;

    calc_t scr_var_sum = 0;
    scr_pass2: for (int i = 0; i < BUFFER_SIZE; i++) {
        #pragma HLS PIPELINE II=1
        calc_t diff = scr_buffer[i] - scr_mean;
        scr_var_sum += diff * diff;
    }
    
    calc_t scr_std = (calc_t)hls::sqrt((float)(scr_var_sum / BUFFER_SIZE));
    
    // Adaptive Peak Thresholds based on mean/std dynamics
    calc_t scr_threshold = scr_mean + (calc_t)0.3f * scr_std;
    calc_t ecg_threshold = ecg_mean + (calc_t)1.5f * ecg_std;

    // Pass 4: Peak Detection and Heatbeat Variability (HRV) Calculation
    int eda_peaks = 0;
    int ecg_peaks = 0;
    int last_ecg_peak_idx = -1;
    
    float ibi_buffer[500] = {0};
    int ibi_count = 0;

    peak_pass: for (int i = 1; i < BUFFER_SIZE - 1; i++) {
        #pragma HLS PIPELINE II=1
        
        // EDA SCR Peak Detection
        if (scr_buffer[i] > scr_threshold && 
            scr_buffer[i] > scr_buffer[i-1] && 
            scr_buffer[i] > scr_buffer[i+1]) {
            eda_peaks++;
        }
        
        // ECG R-Peak Detection
        int idx = head + i;
        if (idx >= BUFFER_SIZE) idx -= BUFFER_SIZE;
        int idx_prev = head + i - 1;
        if (idx_prev >= BUFFER_SIZE) idx_prev -= BUFFER_SIZE;
        int idx_next = head + i + 1;
        if (idx_next >= BUFFER_SIZE) idx_next -= BUFFER_SIZE;
        
        if (ecg_buffer[idx] > ecg_threshold &&
            ecg_buffer[idx] > ecg_buffer[idx_prev] &&
            ecg_buffer[idx] > ecg_buffer[idx_next]) {
            
            // Heartbeat validated based on spatial frequency constraints
            if (last_ecg_peak_idx == -1 || (i - last_ecg_peak_idx) >= MIN_DIST_SAMPLES) {
                if (last_ecg_peak_idx != -1) {
                    int ibi_samples = i - last_ecg_peak_idx;
                    float ibi_ms = (ibi_samples * 1000.0f) / CHEST_SR;
                    
                    if (ibi_ms >= 300.0f && ibi_ms <= 1500.0f) {
                        if (ibi_count < 500) {
                            ibi_buffer[ibi_count++] = ibi_ms;
                        }
                    }
                }
                last_ecg_peak_idx = i;
                ecg_peaks++;
            }
        }
    }

    // Final calculations HRV metrics 
    float mean_hr = 0, std_hr = 0, rmssd = 0, pnn50 = 0, sdnn = 0;
    if (ibi_count >= 2) {
        float ibi_sum = 0;
        for (int i = 0; i < ibi_count; i++) ibi_sum += ibi_buffer[i];
        float ibi_mean = ibi_sum / ibi_count;
        
        mean_hr = 60000.0f / ibi_mean;
        
        float diff_sq_sum = 0;
        int pnn50_count = 0;
        float ibi_var_sum = 0;
        float hr_var_sum = 0;
        
        hrv_pass: for (int i = 0; i < ibi_count; i++) {
            #pragma HLS PIPELINE II=1
            float hr = 60000.0f / ibi_buffer[i];
            float hr_diff = hr - mean_hr;
            hr_var_sum += hr_diff * hr_diff;
            
            float diff_idx = ibi_buffer[i] - ibi_mean;
            ibi_var_sum += diff_idx * diff_idx;
            
            if (i > 0) {
                float diff = ibi_buffer[i] - ibi_buffer[i-1];
                float abs_diff = (diff < 0) ? -diff : diff;
                diff_sq_sum += diff * diff;
                if (abs_diff > 50.0f) pnn50_count++;
            }
        }
        
        float variance_hr = hr_var_sum / ibi_count;
        float var_diffs = diff_sq_sum / (ibi_count - 1);
        float variance_ibi = ibi_var_sum / ibi_count;

        std_hr = hls::sqrt(variance_hr);
        rmssd = hls::sqrt(var_diffs);
        pnn50 = (float)pnn50_count / (ibi_count - 1);
        sdnn = hls::sqrt(variance_ibi);
    }

    // Assemble the continuous array identically mapped to Python Random Forest feature array:
    // ["CH_ECG_HR", "CH_ECG_Std", "CH_ECG_RMSSD", "CH_ECG_pNN50", "CH_ECG_SDNN",
    //  "CH_EDA_Mean", "CH_EDA_Std", "CH_EDA_Min", "CH_EDA_Max", "CH_EDA_SCL", "CH_EDA_SCR", "CH_EDA_Peaks"]
    features[0] = mean_hr;
    features[1] = std_hr;
    features[2] = rmssd;
    features[3] = pnn50;
    features[4] = sdnn;
    features[5] = (float)eda_mean;
    features[6] = (float)eda_std;
    features[7] = (float)eda_min;
    features[8] = (float)eda_max;
    features[9] = (float)scl_mean;
    features[10] = (float)scr_std;
    features[11] = (float)eda_peaks;

    // 3. Inference Engine Pass (execute compiled tree thresholds)
    float pred = predict(features);
    risk_score = pred;

    // Trigger hardware interrupt output mapping
    if (pred > 0.8f) {
        interrupt_trigger = true;
    } else {
        interrupt_trigger = false;
    }
}
