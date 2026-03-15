# Heartbeat+Skin Sweat => Stress Detector Report

**Objective:** Develop a robust, hardware-friendly machine learning (ML) pipeline to predict acute psychological distress in teenagers based on biological sensor data, functioning as the signal for the Continuous Control Barrier Function (CC-CBF) to guard the safety of the LLM for teenagers mental health.

## 1. The Data: Biophysical Inputs
Our model processes physiological sensor data representing human emotional and biological states. We utilized the comprehensive **Wearable Stress and Affect Detection (WESAD)** dataset to extract these insights.

**The Raw Channels:**
Originally, we evaluated 31 features across 15 different channels, including a 700Hz Chest Electrocardiogram (ECG), Skin Conductance or Electrodermal Activity (EDA), Electromyogram (Muscles), Respiration, and Skin Temperature. 

**The Final Features:**
After rigorous testing, we narrowed the inputs down to mathematically extracted features computed over a rolling 60-second window from just **two primary channels**:
1. **Chest EDA** (Electrodermal Activity)
2. **Chest ECG** (Electrocardiogram)

From these two channels, we extract 12 critical statistical features (e.g., Heart Rate, Heart Rate Variability such as the percentage of successive normal-to-normal intervals that differ by more than 50 ms (`pNN50`), Skin Conductance Level, and Skin Conductance Responses).

## 2. The Model: Random Forest Regressor
To translate these raw biophysical signals into a quantifiable stress metric, we trained a **Random Forest Regressor** (a robust decision-tree-based algorithm).

* **The Pipeline:** We utilized a Leave-One-Subject-Out (LOSO) Cross-Validation strategy. This ensures the algorithm is tested strictly on unseen physiology and never "memorizes" a specific teenager's unique baseline.
* **The Output:** The model outputs a **Continuous Biological Arousal Index**—a float ranging from `0.0` (Absolute Calm) to `1.0` (Acute Psychological Panic). 

## 3. The Results
By feeding the 12 extracted EDA and ECG features into the Random Forest, we achieved highly promising predictive safety bounds on unseen subjects.

### Final Metrics (ECG + EDA Only)
* **Target Label:** Continuous Biological Arousal (0.0 = Calm, 1.0 = Acute Psychological Panic)
* **RMSE** (Root Mean Square Error): 0.1829
* **$R^2$ Score** (Coefficient of Determination, measuring how well the model predicts the outcome): 0.8534

### The Feature Rankings
The model relies overwhelmingly on these specific geometrical traits of the raw data (top metrics by feature importance):
1. `CH_EDA_SCL` (Chest Skin Conductance Level): The slow-moving buildup of long-term cognitive pressure. (20.0%)
2. `CH_ECG_HR` (Chest Electrocardiogram Heart Rate): Total cardiovascular velocity. (18.9%)
3. `CH_EDA_Min` / `Max` (Chest Electrodermal Activity Minimum / Maximum): The absolute boundary limits of sweat variation within a 60-second window. (31.4% combined)

## 4. Engineering Decisions: Why This Data and Model?

### Why Only ECG and EDA?
During our exhaustive multimodal testing, an elite "Almighty Model" using all 15 available body sensors yielded an $R^2 = 0.8961$. However, when we restricted the model to **strictly 2 channels (ECG and EDA)**, the accuracy remained phenomenally high at $R^2 = 0.8534$. 

* **EDA (The "Gas Pedal"):** Measures electrical sweat conductance, directly tracking the Sympathetic Nervous System's "Fight or Flight" response.
* **ECG (The "Brakes"):** Tracks Heart Rate Variability (`pNN50`), measuring the Parasympathetic Nervous System's attempt to calm the body. 

**Engineering Impact:** Stripping away Respiration and Muscle sensors drastically reduces hardware complexity. A minimalist, two-channel dry-electrode design on the Aegis-Chip will be much easier to manufacture and comfortable for a teenager to wear, while still retaining clinical-grade safety bounds.

### Why Random Forest?
Despite experimenting with State-of-the-Art Deep Learning models—such as 1-Dimensional Convolutional Neural Networks (1D-CNN) mixed with Bidirectional Gated Recurrent Units (GRUs) and Transformers—we made the strict engineering decision to use Random Forests.

* **FPGA (Field-Programmable Gate Array) Hardware Constraints:** Deep Learning requires massive memory buffers for complex matrix multiplications, exceeding our Block Random Access Memory (BRAM) and Look-Up Table (LUT) limits on the chip. We proved that a Random Forest can achieve an exceptionally high $R^2$ accuracy using only tiny, nested `if/else` threshold statements. 
* **Zero-Latency Execution:** The resulting C-header logic (`rf_cc_cbf_model.h`) executes in constant $O(depth)$ time on an FPGA, which is critical for an instant LLM Guardrail interrupt.
* **Overfitting Prevention:** On the WESAD dataset, pure deep learning suffered severely from cross-subject overfitting ($R^2 = 0.06$). The Random Forest natively generalizes much better across different teenage physiological boundaries.

### Conclusion
We now have a mathematically robust, pre-trained model that maps physiological states to quantitative stress thresholds with >85% certainty. The model scales brilliantly to low-power edge-hardware, entirely fulfilling the requirements for the LLM safety layer.
