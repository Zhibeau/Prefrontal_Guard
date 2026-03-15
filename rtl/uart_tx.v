// =============================================================================
// uart_tx.v — UART Transmitter with AXI-Stream Slave Input
// =============================================================================
// Accepts 16-bit words from an AXI-Stream Slave interface, splits them into
// two 8-bit bytes (little-endian), and serialises each byte as a standard
// 8-N-1 UART frame at 115200 baud on a 200 MHz clock.
//
// Byte ordering (little-endian, must match uart_rx.v):
//   First  transmitted byte ← s_axis_tdata[7:0]  (low byte)
//   Second transmitted byte ← s_axis_tdata[15:8] (high byte)
//
// Each UART frame: [START=0] [D0..D7 LSB-first] [STOP=1]
//                  ← 1 bit  ←    8 bits       ← 1 bit  = 10 bits total
//
// State machine:
//   TX_IDLE  → assert tready, wait for tvalid handshake
//   TX_START → drive txd=0 for exactly CLKS_PER_BIT cycles
//   TX_DATA  → serialise 8 bits, LSB first, one per CLKS_PER_BIT cycles
//   TX_STOP  → drive txd=1 for exactly CLKS_PER_BIT cycles;
//              if byte 0 done → reload byte 1, loop back to TX_START
//              if byte 1 done → return to TX_IDLE
// =============================================================================

`timescale 1ns / 1ps

module uart_tx #(
    parameter CLK_FREQ     = 200_000_000,
    parameter BAUD_RATE    = 115_200,
    // Derived: 200_000_000 / 115_200 = 1736 cycles per bit
    parameter CLKS_PER_BIT = CLK_FREQ / BAUD_RATE   // 1736
)(
    input  wire        clk,
    input  wire        rst_n,

    // ---- AXI-Stream Slave (16-bit words) ------------------------------------
    input  wire [15:0] s_axis_tdata,
    input  wire        s_axis_tvalid,
    output reg         s_axis_tready,

    // ---- UART physical output -----------------------------------------------
    output reg         txd             // idle high
);

// -----------------------------------------------------------------------------
// State encoding
// -----------------------------------------------------------------------------
localparam [1:0]
    TX_IDLE  = 2'd0,
    TX_START = 2'd1,
    TX_DATA  = 2'd2,
    TX_STOP  = 2'd3;

// -----------------------------------------------------------------------------
// Internal registers
// -----------------------------------------------------------------------------
reg [1:0]  state;
reg [11:0] clk_cnt;     // bit-period counter (max 1735, fits in 11 bits)
reg [2:0]  bit_idx;     // counts 0..7 for data bits
reg [15:0] data_buf;    // captured 16-bit word, held for both byte transmissions
reg [7:0]  tx_byte;     // byte currently being serialised
reg        byte_cnt;    // 0 = transmitting low byte, 1 = transmitting high byte

// -----------------------------------------------------------------------------
// Main FSM
// -----------------------------------------------------------------------------
always @(posedge clk or negedge rst_n) begin
    if (!rst_n) begin
        state         <= TX_IDLE;
        clk_cnt       <= 12'd0;
        bit_idx       <= 3'd0;
        data_buf      <= 16'd0;
        tx_byte       <= 8'd0;
        byte_cnt      <= 1'b0;
        txd           <= 1'b1;          // UART idle = high
        s_axis_tready <= 1'b1;          // ready immediately after reset
    end else begin

        case (state)

            // ------------------------------------------------------------------
            // TX_IDLE: assert tready. When master presents valid data, capture
            // the 16-bit word, deassert tready, and start transmitting byte 0.
            //
            // AXI-Stream handshake: transfer occurs the cycle BOTH tvalid AND
            // tready are high. tready goes low one cycle later (registered),
            // which is spec-compliant — the transfer has already completed.
            // ------------------------------------------------------------------
            TX_IDLE: begin
                txd           <= 1'b1;
                clk_cnt       <= 12'd0;
                bit_idx       <= 3'd0;
                byte_cnt      <= 1'b0;
                s_axis_tready <= 1'b1;

                if (s_axis_tvalid) begin
                    data_buf      <= s_axis_tdata;
                    tx_byte       <= s_axis_tdata[7:0]; // byte 0 = low byte
                    s_axis_tready <= 1'b0;              // deassert next cycle
                    state         <= TX_START;
                end
            end

            // ------------------------------------------------------------------
            // TX_START: drive txd low for exactly one bit period.
            // ------------------------------------------------------------------
            TX_START: begin
                txd <= 1'b0;    // start bit

                if (clk_cnt == CLKS_PER_BIT - 1) begin
                    clk_cnt <= 12'd0;
                    state   <= TX_DATA;
                end else begin
                    clk_cnt <= clk_cnt + 1'b1;
                end
            end

            // ------------------------------------------------------------------
            // TX_DATA: serialise 8 bits, LSB first.
            // txd is driven from tx_byte[0] continuously throughout the state.
            // At the end of each bit period the shift register rotates right;
            // the new tx_byte[0] is picked up on the very next clock cycle,
            // giving exactly CLKS_PER_BIT cycles of stable txd per bit.
            // ------------------------------------------------------------------
            TX_DATA: begin
                txd <= tx_byte[0];  // LSB-first; stable for CLKS_PER_BIT cycles

                if (clk_cnt == CLKS_PER_BIT - 1) begin
                    clk_cnt <= 12'd0;
                    tx_byte <= {1'b0, tx_byte[7:1]};    // logical right shift

                    if (bit_idx == 3'd7) begin
                        bit_idx <= 3'd0;
                        state   <= TX_STOP;
                    end else begin
                        bit_idx <= bit_idx + 1'b1;
                    end
                end else begin
                    clk_cnt <= clk_cnt + 1'b1;
                end
            end

            // ------------------------------------------------------------------
            // TX_STOP: drive txd high for one stop-bit period.
            //   byte_cnt == 0 → byte 0 done; load high byte, go back to START.
            //   byte_cnt == 1 → byte 1 done; entire word sent, go to IDLE.
            // ------------------------------------------------------------------
            TX_STOP: begin
                txd <= 1'b1;    // stop bit

                if (clk_cnt == CLKS_PER_BIT - 1) begin
                    clk_cnt <= 12'd0;

                    if (byte_cnt == 1'b0) begin
                        // Finished byte 0 — load byte 1 (high byte) and loop
                        tx_byte  <= data_buf[15:8];
                        byte_cnt <= 1'b1;
                        state    <= TX_START;
                    end else begin
                        // Finished byte 1 — word complete, return to IDLE
                        byte_cnt <= 1'b0;
                        state    <= TX_IDLE;
                    end
                end else begin
                    clk_cnt <= clk_cnt + 1'b1;
                end
            end

            default: begin
                state <= TX_IDLE;
                txd   <= 1'b1;
            end

        endcase
    end
end

endmodule
