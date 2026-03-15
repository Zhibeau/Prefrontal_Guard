// =============================================================================
// btn_anxiety.v — 4-Button Anxiety Level Encoder
// =============================================================================
// Converts the four user push-buttons on the AX7102 expansion board into a
// 16-bit anxiety_level value consumed by the CC-CBF engine.
//
// Encoding (KEY1 = MSB, KEY4 = LSB):
//
//   KEY1  KEY2  KEY3  KEY4  |  4-bit  |  anxiety_level
//   ----  ----  ----  ----  |---------|---------------
//     0     0     0     0   |  0000   |      0   (calm, no correction)
//     0     0     0     1   |  0001   |     64
//     0     0     1     0   |  0010   |    128
//     0     0     1     1   |  0011   |    192
//     0     1     0     0   |  0100   |    256
//     0     1     0     1   |  0101   |    320
//     0     1     1     0   |  0110   |    384
//     0     1     1     1   |  0111   |    448
//     1     0     0     0   |  1000   |    512  ← press KEY1 alone
//     1     0     0     1   |  1001   |    576
//     1     0     1     0   |  1010   |    640
//     1     0     1     1   |  1011   |    704
//     1     1     0     0   |  1100   |    768
//     1     1     0     1   |  1101   |    832
//     1     1     1     0   |  1110   |    896
//     1     1     1     1   |  1111   |    960  ← press all 4 (max stress)
//
// anxiety_level = {key_pressed[3:0], 6'b0}  (== 4-bit value × 64)
//
// Hardware characteristics (from AX7102_UG.pdf p.54-55):
//   Buttons  : active-LOW (key_n=0 when pressed, 3.3V pull-up to Vcc)
//   LEDs     : active-LOW (led_n=0 turns LED on)
//   Both sets: Bank 14/15, LVCMOS33
//
// Debouncing:
//   Each button is double-flopped (metastability) then debounced with a
//   DEBOUNCE_MS counter. A new state is accepted only after the input has
//   been stable for the full debounce period. Prevents glitching on anxiety_level.
// =============================================================================

`timescale 1ns / 1ps

module btn_anxiety #(
    parameter CLK_FREQ    = 200_000_000,
    parameter DEBOUNCE_MS = 20           // 20 ms bounce suppression
)(
    input  wire        clk,
    input  wire        rst_n,

    // ---- Push-buttons (active-low, 3.3 V pull-up) ---------------------------
    // key_n[0] = KEY1 (B18),  key_n[1] = KEY2 (B17)
    // key_n[2] = KEY3 (A16),  key_n[3] = KEY4 (A15)
    input  wire [3:0]  key_n,

    // ---- User LEDs (active-low, mirror the debounced key state) -------------
    // led_n[0] = LED1 (C17),  led_n[1] = LED2 (D17)
    // led_n[2] = LED3 (V20),  led_n[3] = LED4 (U20)
    output wire [3:0]  led_n,

    // ---- Anxiety level output [0, 960] in steps of 64 -----------------------
    output wire [15:0] anxiety_level
);

// ---------------------------------------------------------------------------
// Derived parameters
// ---------------------------------------------------------------------------
// DEBOUNCE_CYCLES = 200_000_000 × 20 / 1000 = 4_000_000
// 2²² = 4_194_304 > 4_000_000  → 22-bit counter is sufficient.
localparam integer DEBOUNCE_CYCLES = CLK_FREQ / 1000 * DEBOUNCE_MS; // 4_000_000

// ---------------------------------------------------------------------------
// Stage 1 — 2-flop synchroniser (eliminates metastability from button pads)
// Reset to 4'hF (all released = all 1, active-low)
// ---------------------------------------------------------------------------
reg [3:0] sync0, sync1;
always @(posedge clk or negedge rst_n) begin
    if (!rst_n) begin
        sync0 <= 4'hF;
        sync1 <= 4'hF;
    end else begin
        sync0 <= key_n;
        sync1 <= sync0;
    end
end
wire [3:0] key_sync = sync1;   // synchronised, metastability-free

// ---------------------------------------------------------------------------
// Stage 2 — Per-key debounce counter
// For each key: when key_sync changes from stable, start a counter.
// Accept the new value only after DEBOUNCE_CYCLES consecutive cycles.
// ---------------------------------------------------------------------------
genvar i;
generate
    for (i = 0; i < 4; i = i + 1) begin : GEN_DEBOUNCE

        reg [21:0] cnt;
        reg        stable;   // debounced output (active-low: 0=pressed, 1=released)

        always @(posedge clk or negedge rst_n) begin
            if (!rst_n) begin
                cnt    <= 22'd0;
                stable <= 1'b1;   // released on reset
            end else if (key_sync[i] == stable) begin
                // Input matches current stable value — nothing changing, reset counter
                cnt <= 22'd0;
            end else if (cnt == DEBOUNCE_CYCLES - 1) begin
                // Input has been different for the full debounce period — accept it
                stable <= key_sync[i];
                cnt    <= 22'd0;
            end else begin
                cnt <= cnt + 22'd1;
            end
        end

        // LED mirrors button: both are active-low, so a direct connection is correct.
        // key pressed (stable=0) → led_n=0 → LED ON
        // key released (stable=1) → led_n=1 → LED OFF
        assign led_n[i]     = stable;

    end
endgenerate

// ---------------------------------------------------------------------------
// Stage 3 — Encode 4 debounced key states into anxiety_level
//
//   key_pressed[i] = 1 when key i is held down (active-high internal signal)
//   anxiety_level  = {key_pressed[0..3], 6'b0}
//                  = key_pressed_4bit × 64
//
// KEY1 (key_n[0]) → bit 3 of the 4-bit word (highest weight = +512)
// KEY4 (key_n[3]) → bit 0 of the 4-bit word (lowest  weight = +64)
// ---------------------------------------------------------------------------
wire [3:0] key_pressed;
assign key_pressed[0] = ~GEN_DEBOUNCE[0].stable;   // KEY1
assign key_pressed[1] = ~GEN_DEBOUNCE[1].stable;   // KEY2
assign key_pressed[2] = ~GEN_DEBOUNCE[2].stable;   // KEY3
assign key_pressed[3] = ~GEN_DEBOUNCE[3].stable;   // KEY4

// Concatenate: {KEY1,KEY2,KEY3,KEY4} as bits [3:0] of the 10-bit scale.
// Shift left 6 → multiply by 64.
//   0000 → 0,  0001 → 64,  ...  1111 → 960
assign anxiety_level = {6'b0, key_pressed[0], key_pressed[1],
                               key_pressed[2], key_pressed[3], 6'b0};

endmodule
