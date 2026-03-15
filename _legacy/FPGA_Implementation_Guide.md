# FPGA Implementation Guide: Aegis-Chip Safety ML

## Introduction
The machine learning phase is complete. The model has been exported to a lightweight C-header format (`rf_biological_arousal.h` or `rf_cc_cbf_model.h`). This document outlines what the FPGA (via Vitis HLS) actually needs to do in real-time to drive the model.

## 1. The Core Execution Loop
The Random Forest model does **not** take "raw sensor data" as inputs. It expects mathematically extracted "Features". 
You should implement this as a **Continuous Streaming (DSP)** pipeline:
1. **Streaming Buffer:** Maintain a rolling 60-second circular buffer in BRAM (populated continuously as new sensor ticks arrive).
2. **Continuous Feature Extraction:** Re-calculate the 12 statistical features continuously. Fast features (like `EDA_Max`) update instantly as new data enters the buffer. Slow features (like `HRV_pNN50`) update with every new detected heartbeat.
3. **Inference (ML step):** The Random Forest C-header `predict()` function can be executed at whatever clock rate the system requires (e.g., every 1 second, or synchronized to the LLM token generation rate).
4. **Trigger:** Receive the output float `[0.0 to 1.0]`. If it breaches the safety threshold (e.g., `> 0.8`), output a hardware interrupt to the LLM to restrict content.

## 2. FPGA DSP Requirements (Feature Extraction)
The real engineering challenge on the FPGA is not the ML model (which is just simple `if/else` statements), but writing the optimized C++ / Verilog to extract the features from the raw arrays.

You must implement these hardware blocks:

### A. The EDA Block (Sweat)
Given a 1D array of EDA voltage readings:
* **`EDA_Mean`, `EDA_Min`, `EDA_Max`, `EDA_Std`**: Standard statistical aggregators.
* **Exponential Moving Average (EMA) Filters**: You must implement a slow Low-Pass filter (e.g., `alpha = 0.01`) to isolate the **`SCL`** (Skin Conductance Level) trend line from the raw array. 
* **Peak Detection**: Calculate the remainder `SCR = Raw - SCL`. Find local peaks in the SCR array that exceed an adaptive threshold (`mean + 0.3*std`). Count the total `EDA_Peaks`.

### B. The ECG Block (Heartbeat)
Given a 1D array of raw ECG voltage (e.g. 700Hz):
* **Peak Detection Thresholding:** Find the R-peaks in the QRS complex. A rolling threshold `mean + 1.5 * std` is recommended. Ensure a "dead-time" or minimal-distance constraint (`e.g., 0.3 * SampleRate`) to avoid double-counting the same heartbeat.
* **IBI (Inter-Beat Interval):** Subtract the index of adjacent peaks and convert to milliseconds to get an array of IBIs.
* **`ECG_HR` (Heart Rate):** `60000.0 / Mean(IBI array)`.
* **Heart Rate Variability (HRV) Metrics:** 
  * `ECG_SDNN`: Standard deviation of the valid IBIs.
  * `ECG_RMSSD`: Root Mean Square of Successive Differences between adjacent IBIs.
  * `ECG_pNN50`: Count the number of adjacent IBIs that differ by more than 50 milliseconds, and divide by the total number of IBIs. This is the single most important heart-rate metric!

## 3. Optimizing for Vitis HLS
* **Fixed-Point Math:** The random forest model currently uses single-precision `float`. You can drastically shrink the LUT / FF usage on the FPGA by mapping the thresholds inside the C header to fixed-point arithmetic (e.g., `ap_fixed<16,8>`). 
* **Pipelining:** Ensure the feature extraction `for`-loops (especially the EMA filtering and Peak iteration) are unrolled or pipelined (`#pragma HLS PIPELINE`) to hit cycle constraints.
* **Circular Buffers:** Use circular arrays/ring buffers for the 60-second sliding window overlap so the hardware doesn't waste cycles copying large blocks of memory every 10 seconds.
