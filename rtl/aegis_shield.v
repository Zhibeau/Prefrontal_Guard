// =============================================================================
// aegis_shield.v — DEBUG STUB (constant-vector loopback)
// =============================================================================
// Drains 16 input words then outputs one of 4 constant 16-word vectors
// selected by KEY1 (anx_in[9]) and KEY2 (anx_in[8]):
//
//   KEY1=0 KEY2=0  →  all  0           (no buttons)
//   KEY1=0 KEY2=1  →  all  1111        (KEY2 only)
//   KEY1=1 KEY2=0  →  [1,2,3,...,16]   (KEY1 only)
//   KEY1=1 KEY2=1  →  all  32767       (KEY1+KEY2)
//
// Keeps identical port list so aegis_top_usb.v is untouched.
// =============================================================================

`timescale 1ns / 1ps

module aegis_shield (
    input  wire        ap_clk,
    input  wire        ap_rst_n,

    input  wire [15:0] anx_in,

    // x_t AXI-Stream slave (input — drain and ignore)
    input  wire [15:0] x_t_TDATA,
    input  wire        x_t_TVALID,
    output reg         x_t_TREADY,
    input  wire [1:0]  x_t_TKEEP,
    input  wire [1:0]  x_t_TSTRB,
    input  wire [0:0]  x_t_TLAST,

    // u_t AXI-Stream master (output — constant vector)
    output reg  [15:0] u_t_TDATA,
    output reg         u_t_TVALID,
    input  wire        u_t_TREADY,
    output wire [1:0]  u_t_TKEEP,
    output reg  [0:0]  u_t_TLAST
);

assign u_t_TKEEP = 2'b11;

// Suppress unused-port warnings
wire _unused = &{1'b0, x_t_TDATA, x_t_TKEEP, x_t_TSTRB, x_t_TLAST};

// ---------------------------------------------------------------------------
// Constant word selector:  vec_sel = {KEY1, KEY2} = anx_in[9:8]
// Snapshot at drain completion so the key state is latched per frame.
// ---------------------------------------------------------------------------
reg [1:0] vec_sel;

function [15:0] const_word;
    input [1:0] sel;
    input [3:0] idx;  // 0..15
    case (sel)
        2'd0: const_word = 16'd0;               // all zeros
        2'd1: const_word = 16'd1111;            // all 1111
        2'd2: const_word = {12'd0, idx} + 1;   // 1, 2, 3, … 16
        2'd3: const_word = 16'd32767;           // all max
        default: const_word = 16'd0;
    endcase
endfunction

// ---------------------------------------------------------------------------
// State machine
// ---------------------------------------------------------------------------
localparam [0:0] S_DRAIN = 1'b0,
                 S_TX    = 1'b1;

reg        state;
reg [3:0]  idx;

always @(posedge ap_clk or negedge ap_rst_n) begin
    if (!ap_rst_n) begin
        state        <= S_DRAIN;
        idx          <= 4'd0;
        vec_sel      <= 2'd0;
        x_t_TREADY   <= 1'b1;
        u_t_TVALID   <= 1'b0;
        u_t_TDATA    <= 16'd0;
        u_t_TLAST    <= 1'b0;
    end else begin
        case (state)

            // ------------------------------------------------------------------
            // DRAIN: accept (and discard) 16 input words
            // ------------------------------------------------------------------
            S_DRAIN: begin
                x_t_TREADY <= 1'b1;
                u_t_TVALID <= 1'b0;

                if (x_t_TVALID && x_t_TREADY) begin
                    if (idx == 4'd15) begin
                        vec_sel    <= anx_in[9:8];  // latch key state
                        idx        <= 4'd0;
                        x_t_TREADY <= 1'b0;
                        state      <= S_TX;
                    end else begin
                        idx <= idx + 4'd1;
                    end
                end
            end

            // ------------------------------------------------------------------
            // TX: stream 16 constant words
            // ------------------------------------------------------------------
            S_TX: begin
                u_t_TVALID <= 1'b1;
                u_t_TDATA  <= const_word(vec_sel, idx);
                u_t_TLAST  <= (idx == 4'd15) ? 1'b1 : 1'b0;

                if (u_t_TVALID && u_t_TREADY) begin
                    if (idx == 4'd15) begin
                        idx        <= 4'd0;
                        u_t_TVALID <= 1'b0;
                        u_t_TLAST  <= 1'b0;
                        x_t_TREADY <= 1'b1;
                        state      <= S_DRAIN;
                    end else begin
                        idx <= idx + 4'd1;
                    end
                end
            end

            default: state <= S_DRAIN;
        endcase
    end
end

endmodule
