// =============================================================================
// adc_frontend.v — Dual-Channel XADC Front-End (ECG + EDA)
// Target: Xilinx Artix-7 (XC7A100T-CSG324)
// =============================================================================
//
// Uses the built-in XADC to continuously sample two differential channels:
//   VAUXP[0]/VAUXN[0]  →  ECG electrode (after instrumentation amp + DC bias)
//   VAUXP[1]/VAUXN[1]  →  EDA (GSR) electrode (after signal conditioning)
//
// Both channels sampled at the WESAD-matching 700 Hz output rate.
// XADC runs its internal sequencer much faster (~97 ksps); we latch the most
// recent conversion and pulse sample_valid at exactly 700 Hz from a counter.
//
// Signal conditioning notes (external hardware required):
//   ECG:  AD8232 instrumentation amp (gain ≈ 100×, DC-biased to 0.5 V)
//         Output: 0 – 1 V differential into VAUXP[0]/VAUXN[0]
//   EDA:  Resistive divider with 0.5 V excitation; output: 0.1 – 0.9 V
//         Input to VAUXP[1]/VAUXN[1]
//
// XADC result format: 12 MSBs in DO[15:4], sign-extended to 16-bit output.
//
// ADCCLKDIV = 5  →  ADC clock = 200 MHz / 10 = 20 MHz (max 26 MHz ✓)
// Conversion time ≈ 26 / 20 MHz = 1.3 µs per channel
// Sequencer period ≈ 2.6 µs for 2 channels  →  ~385 ksps >> 700 Hz
// =============================================================================

`timescale 1ns / 1ps

module adc_frontend (
    input  wire sys_clk_200m,
    input  wire reset_n,

    // ---- Differential analog inputs (route to XADC-dedicated I/O pads) -----
    input  wire vauxp_ecg,      // VAUXP[0] — ECG channel
    input  wire vauxn_ecg,      // VAUXN[0]
    input  wire vauxp_eda,      // VAUXP[1] — EDA channel
    input  wire vauxn_eda,      // VAUXN[1]

    // ---- Digitised outputs at 700 Hz ----------------------------------------
    output reg  [15:0] ecg_sample,   // Signed 16-bit (12 MSBs valid, [15:4])
    output reg         ecg_valid,    // 1-clock strobe at 700 Hz
    output reg  [15:0] eda_sample,   // Signed 16-bit (12 MSBs valid, [15:4])
    output reg         eda_valid     // 1-clock strobe at 700 Hz
);

// =============================================================================
// 1. 700 Hz strobe counter
//    200 000 000 / 700 = 285 714.28...  →  use 285 714 (error < 0.001%)
// =============================================================================
localparam integer SAMP_DIV = 285_714;

reg [17:0] samp_cnt = 0;
reg        samp_tick = 0;

always @(posedge sys_clk_200m) begin
    if (!reset_n) begin
        samp_cnt  <= 0;
        samp_tick <= 0;
    end else if (samp_cnt == SAMP_DIV - 1) begin
        samp_cnt  <= 0;
        samp_tick <= 1;
    end else begin
        samp_cnt  <= samp_cnt + 1'b1;
        samp_tick <= 0;
    end
end

// =============================================================================
// 2. XADC primitive — dual-channel continuous sequencer
//
//  INIT_40 [15:8]=0x90: CAVG=1 (calib averaging), AVG[1:0]=01 (16 samples)
//  INIT_40 [7:0] =0x00: unipolar, no power-down
//  INIT_41       =0x2EF0: enable all alarms, sequencer mode = continuous
//  INIT_42 [7:3] =0x04: ADCCLKDIV=5 (bits 7:3 encode div-1, so div=5→0b00101)
//  INIT_48       =0x4701: enable internal channels + VAUX0
//  INIT_49       =0x0002: enable VAUX1
//  INIT_4A/4B    =0x0003: unipolar for both aux channels
// =============================================================================
wire        xadc_busy;
wire [4:0]  xadc_channel;
wire        xadc_eoc;
wire        xadc_eos;
wire        xadc_drdy;
wire [15:0] xadc_do;

// VAUXP/VAUXN buses (16 channels): wire our two channels in; tie rest to 0
wire [15:0] vauxp_bus = {{14{1'b0}}, vauxp_eda, vauxp_ecg};
wire [15:0] vauxn_bus = {{14{1'b0}}, vauxn_eda, vauxn_ecg};

XADC #(
    // ---- Configuration register 0 -------------------------------------------
    // [15:12]=CAVG_CAL  [11:8]=AVG(01=16)  [7:0]=channel/bipolar/etc.
    .INIT_40 (16'h9000),

    // ---- Configuration register 1 (alarm enables, sequencer mode) -----------
    // SEQUENCER mode = 10 (continuous channel sequencer)
    .INIT_41 (16'h2EF0),

    // ---- Configuration register 2 (clock divider) ---------------------------
    // ADCCLKDIV[4:0] = 5  (bits 7:3)  →  ADC clock = 200 MHz / 10 = 20 MHz
    .INIT_42 (16'h0500),

    // ---- Channel sequence register 0: internal + VAUX0 ----------------------
    .INIT_48 (16'h4701),     // Temp, VCCINT, VCCAUX, VAUX0 enabled
    .INIT_49 (16'h0002),     // VAUX1 enabled

    // ---- Averaging for aux channels -----------------------------------------
    .INIT_4A (16'h0003),     // 16-sample avg on VAUX0, VAUX1
    .INIT_4B (16'h0000),

    // ---- Acquisition extension (extra settling for EDA electrode) -----------
    .INIT_4C (16'h0000),
    .INIT_4D (16'h0000),
    .INIT_4E (16'h0000),
    .INIT_4F (16'h0000),
    .INIT_50 (16'hB5ED),     // Temperature upper alarm threshold
    .INIT_51 (16'h57E4),     // VCCINT upper alarm threshold
    .INIT_52 (16'hA147),     // VCCAUX upper alarm threshold
    .INIT_53 (16'hCA33),     // Over-temperature limit
    .INIT_54 (16'hA93A),     // Temperature lower alarm threshold
    .INIT_55 (16'h52C6),     // VCCINT lower alarm threshold
    .INIT_56 (16'h9555),     // VCCAUX lower alarm threshold
    .INIT_57 (16'hAE4E),     // Over-temperature reset threshold
    .INIT_58 (16'hE000),     // VBRAM upper alarm threshold
    .INIT_5C (16'h5999),     // VBRAM lower alarm threshold
    .SIM_MONITOR_FILE ("design.txt"),   // Unused in synthesis
    .SIM_DEVICE       ("7SERIES")
) u_xadc (
    .DCLK       (sys_clk_200m),
    .RESET      (~reset_n),

    // External conversion trigger — not used (sequencer drives conversions)
    .CONVST     (1'b0),
    .CONVSTCLK  (1'b0),

    // Dedicated VP/VN channel — left unconnected (tied to GND internally)
    .VP         (1'b0),
    .VN         (1'b0),

    // Auxiliary channels
    .VAUXP      (vauxp_bus),
    .VAUXN      (vauxn_bus),

    // DRP — read-only (no dynamic reconfiguration)
    .DEN        (1'b0),
    .DWE        (1'b0),
    .DADDR      (7'h00),
    .DI         (16'h0000),
    .DO         (xadc_do),
    .DRDY       (xadc_drdy),

    // Status
    .BUSY       (xadc_busy),
    .CHANNEL    (xadc_channel),
    .EOC        (xadc_eoc),
    .EOS        (xadc_eos),
    .ALM        (),     // Alarm flags — left open
    .OT         (),     // Over-temperature — left open

    // Unused ports
    .MUXADDR    (),
    .JTAGLOCKED (),
    .JTAGMODIFIED(),
    .JTAGBUSY   ()
);

// =============================================================================
// 3. Latch most-recent XADC result per channel
//    Channel encoding (XADC UG480 Table 2-4):
//      5'h10 = VAUX[0]  →  ECG
//      5'h11 = VAUX[1]  →  EDA
// =============================================================================
reg [15:0] ecg_latch = 16'h0;
reg [15:0] eda_latch = 16'h0;

always @(posedge sys_clk_200m) begin
    if (xadc_drdy) begin
        case (xadc_channel)
            5'h10: ecg_latch <= xadc_do;   // VAUX[0] = ECG
            5'h11: eda_latch <= xadc_do;   // VAUX[1] = EDA
            default: ;
        endcase
    end
end

// =============================================================================
// 4. Output latched samples at the 700 Hz strobe tick
//    The 12-bit ADC result occupies DO[15:4]; DO[3:0] are always 0.
//    We pass the full 16-bit word (upper 12 bits valid) to the HLS DSP modules,
//    which treat it as a signed ap_int<16> (positive half of 0–1 V range).
// =============================================================================
always @(posedge sys_clk_200m) begin
    ecg_valid <= 1'b0;
    eda_valid <= 1'b0;

    if (samp_tick) begin
        ecg_sample <= ecg_latch;
        eda_sample <= eda_latch;
        ecg_valid  <= 1'b1;
        eda_valid  <= 1'b1;
    end
end

endmodule
