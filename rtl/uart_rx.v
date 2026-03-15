// =============================================================================
// uart_rx.v — UART Receiver with AXI-Stream Master Output
// =============================================================================
// Receives 8-bit UART bytes at 115200 baud on a 200 MHz clock, assembles
// every TWO consecutive bytes into one 16-bit little-endian word, and
// presents it on an AXI-Stream Master interface.
//
// Byte ordering (little-endian):
//   First  received byte → m_axis_tdata[7:0]  (low byte)
//   Second received byte → m_axis_tdata[15:8] (high byte)
//
// CLKS_PER_BIT = 200_000_000 / 115_200 = 1736 (0.006 % baud error, <1 LSB
// drift over a full 10-bit frame — well within UART tolerance).
//
// State machine:
//   S_IDLE     → wait for falling edge (start bit)
//   S_START    → delay CLKS_PER_BIT/2 to centre-align all samples
//   S_DATA     → sample 8 data bits, one per bit period
//   S_STOP     → consume stop bit period
//   S_ASSEMBLE → buffer first byte; on second byte, drive tdata/tvalid
//   S_PRESENT  → hold tvalid until downstream accepts (tready high)
// =============================================================================

`timescale 1ns / 1ps

module uart_rx #(
    parameter CLK_FREQ     = 200_000_000,
    parameter BAUD_RATE    = 115_200,
    // Derived: number of system-clock cycles per UART bit period.
    // 200_000_000 / 115_200 = 1736  (11 bits needed; use 12 for headroom)
    parameter CLKS_PER_BIT = CLK_FREQ / BAUD_RATE   // 1736
)(
    input  wire        clk,
    input  wire        rst_n,

    // ---- UART physical input ------------------------------------------------
    input  wire        rxd,

    // ---- AXI-Stream Master (16-bit words) -----------------------------------
    output reg  [15:0] m_axis_tdata,
    output reg         m_axis_tvalid,
    input  wire        m_axis_tready
);

// -----------------------------------------------------------------------------
// 2-flop synchroniser — eliminates metastability on the async RXD pin.
// Idle state of UART line is logic 1.
// -----------------------------------------------------------------------------
reg rxd_ff0, rxd_ff1;
always @(posedge clk or negedge rst_n) begin
    if (!rst_n) begin
        rxd_ff0 <= 1'b1;
        rxd_ff1 <= 1'b1;
    end else begin
        rxd_ff0 <= rxd;
        rxd_ff1 <= rxd_ff0;
    end
end
wire rxd_s = rxd_ff1;   // synchronised, metastability-free copy

// -----------------------------------------------------------------------------
// State encoding
// -----------------------------------------------------------------------------
localparam [2:0]
    S_IDLE     = 3'd0,
    S_START    = 3'd1,
    S_DATA     = 3'd2,
    S_STOP     = 3'd3,
    S_ASSEMBLE = 3'd4,
    S_PRESENT  = 3'd5;

// -----------------------------------------------------------------------------
// Internal registers
// -----------------------------------------------------------------------------
reg [2:0]  state;
reg [11:0] clk_cnt;     // counts up to CLKS_PER_BIT-1 (max 1735, fits 11 bits)
reg [2:0]  bit_idx;     // counts 0..7 for the 8 data bits
reg [7:0]  rx_byte;     // shift register: accumulates incoming bits LSB-first
reg        byte_cnt;    // 0 = first byte of pair, 1 = second byte of pair
reg [7:0]  byte_buf;    // holds the first byte until the second arrives

// -----------------------------------------------------------------------------
// Main FSM
// -----------------------------------------------------------------------------
always @(posedge clk or negedge rst_n) begin
    if (!rst_n) begin
        state         <= S_IDLE;
        clk_cnt       <= 12'd0;
        bit_idx       <= 3'd0;
        rx_byte       <= 8'd0;
        byte_cnt      <= 1'b0;
        byte_buf      <= 8'd0;
        m_axis_tdata  <= 16'd0;
        m_axis_tvalid <= 1'b0;
    end else begin

        case (state)

            // ------------------------------------------------------------------
            // S_IDLE: line is high (idle). Wait for start-bit falling edge.
            // ------------------------------------------------------------------
            S_IDLE: begin
                clk_cnt <= 12'd0;
                bit_idx <= 3'd0;

                if (rxd_s == 1'b0)      // falling edge → start bit detected
                    state <= S_START;
            end

            // ------------------------------------------------------------------
            // S_START: wait half a bit period, then re-sample.
            // Sampling at the centre of the start bit aligns all subsequent
            // samples to the centre of each data bit (CLKS_PER_BIT later).
            // ------------------------------------------------------------------
            S_START: begin
                if (clk_cnt == (CLKS_PER_BIT / 2) - 1) begin
                    clk_cnt <= 12'd0;
                    if (rxd_s == 1'b0)
                        state <= S_DATA;    // confirmed start bit
                    else
                        state <= S_IDLE;    // glitch / noise, abort
                end else begin
                    clk_cnt <= clk_cnt + 1'b1;
                end
            end

            // ------------------------------------------------------------------
            // S_DATA: sample one bit every CLKS_PER_BIT cycles.
            // UART is LSB-first; shift new bit into MSB, old bits right.
            // After 8 bits rx_byte[0]=bit0 (LSB) ... rx_byte[7]=bit7 (MSB).
            // ------------------------------------------------------------------
            S_DATA: begin
                if (clk_cnt == CLKS_PER_BIT - 1) begin
                    clk_cnt <= 12'd0;
                    rx_byte <= {rxd_s, rx_byte[7:1]};   // LSB-first deserialise

                    if (bit_idx == 3'd7) begin
                        bit_idx <= 3'd0;
                        state   <= S_STOP;
                    end else begin
                        bit_idx <= bit_idx + 1'b1;
                    end
                end else begin
                    clk_cnt <= clk_cnt + 1'b1;
                end
            end

            // ------------------------------------------------------------------
            // S_STOP: consume the stop-bit period (line must be high).
            // We do not verify the stop bit value to be lenient with transmitters
            // that have slight baud-rate skew; just wait out the period.
            // ------------------------------------------------------------------
            S_STOP: begin
                if (clk_cnt == CLKS_PER_BIT - 1) begin
                    clk_cnt <= 12'd0;
                    state   <= S_ASSEMBLE;
                end else begin
                    clk_cnt <= clk_cnt + 1'b1;
                end
            end

            // ------------------------------------------------------------------
            // S_ASSEMBLE: two-byte accumulator.
            //   byte_cnt == 0 → stash first byte, return to IDLE for byte 2.
            //   byte_cnt == 1 → form 16-bit word, assert tvalid, go to PRESENT.
            //
            // Little-endian: first byte  → tdata[7:0]
            //                second byte → tdata[15:8]
            // ------------------------------------------------------------------
            S_ASSEMBLE: begin
                if (byte_cnt == 1'b0) begin
                    byte_buf <= rx_byte;        // save low byte
                    byte_cnt <= 1'b1;
                    state    <= S_IDLE;         // wait for second byte
                end else begin
                    m_axis_tdata  <= {rx_byte, byte_buf};   // {high, low}
                    m_axis_tvalid <= 1'b1;
                    byte_cnt      <= 1'b0;
                    state         <= S_PRESENT;
                end
            end

            // ------------------------------------------------------------------
            // S_PRESENT: hold tvalid/tdata until downstream accepts the word.
            // Only release and return to IDLE once tready is sampled high.
            // ------------------------------------------------------------------
            S_PRESENT: begin
                if (m_axis_tready) begin
                    m_axis_tvalid <= 1'b0;
                    state         <= S_IDLE;
                end
            end

            default: state <= S_IDLE;

        endcase
    end
end

endmodule
