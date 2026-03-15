// =============================================================================
// aegis_shield.v — Simple sequential RTL implementation of real shield math
// =============================================================================

`timescale 1ns / 1ps

module aegis_shield (
    input  wire        ap_clk,
    input  wire        ap_rst_n,

    input  wire [15:0] anx_in,

    input  wire [15:0] x_t_TDATA,
    input  wire        x_t_TVALID,
    output wire        x_t_TREADY,
    input  wire [1:0]  x_t_TKEEP,
    input  wire [1:0]  x_t_TSTRB,
    input  wire [0:0]  x_t_TLAST,

    output reg  [15:0] u_t_TDATA,
    output reg         u_t_TVALID,
    input  wire        u_t_TREADY,
    output wire [1:0]  u_t_TKEEP,
    output reg  [0:0]  u_t_TLAST
);

assign u_t_TKEEP = 2'b11;

localparam [2:0] S_DRAIN   = 3'd0,
                 S_DOT_MUL = 3'd1,
                 S_DOT_ACC = 3'd2,
                 S_TX_CALC = 3'd3,
                 S_TX_SEND = 3'd4;

localparam integer DIM    = 16;
localparam integer B_BASE = 5000;
localparam integer ALPHA  = 6000;

reg [2:0] state;
reg [3:0] idx;
reg signed [31:0] x_buf [0:DIM-1];
reg signed [31:0] h_acc;
reg signed [31:0] h_value;

integer j;

assign x_t_TREADY = (state == S_DRAIN);

function signed [31:0] sign_extend16;
    input [15:0] raw;
    begin
        if (raw[15])
            sign_extend16 = $signed({16'hFFFF, raw});
        else
            sign_extend16 = $signed({16'h0000, raw});
    end
endfunction

function signed [31:0] weight_for_idx;
    input [3:0] word_idx;
    begin
        case (word_idx)
            4'd0:  weight_for_idx = 32'sd64;
            4'd1:  weight_for_idx = 32'sd128;
            4'd2:  weight_for_idx = 32'sd96;
            4'd3:  weight_for_idx = 32'sd112;
            4'd4:  weight_for_idx = 32'sd80;
            4'd5:  weight_for_idx = 32'sd72;
            4'd6:  weight_for_idx = 32'sd104;
            4'd7:  weight_for_idx = 32'sd88;
            4'd8:  weight_for_idx = 32'sd120;
            4'd9:  weight_for_idx = 32'sd60;
            4'd10: weight_for_idx = 32'sd92;
            4'd11: weight_for_idx = 32'sd116;
            4'd12: weight_for_idx = 32'sd76;
            4'd13: weight_for_idx = 32'sd84;
            4'd14: weight_for_idx = 32'sd100;
            4'd15: weight_for_idx = 32'sd68;
            default: weight_for_idx = 32'sd0;
        endcase
    end
endfunction

function signed [15:0] saturate16;
    input signed [31:0] value;
    begin
        if (value > 32'sd32767)
            saturate16 = 16'sd32767;
        else if (value < -32'sd32768)
            saturate16 = -16'sd32768;
        else
            saturate16 = value[15:0];
    end
endfunction

function signed [15:0] steer_word;
    input signed [31:0] h_in;
    input [3:0] word_idx;
    reg signed [31:0] scaled;
    begin
        if (h_in >= 0) begin
            steer_word = 16'sd0;
        end else begin
            scaled = (((-h_in) * weight_for_idx(word_idx)) >>> 10);
            steer_word = saturate16(scaled);
        end
    end
endfunction

wire signed [31:0] boundary_term = 32'sd5000 - ((32'sd6000 * $signed({1'b0, anx_in[14:0]})) >>> 10);
wire _unused_ok = &{1'b0, x_t_TKEEP, x_t_TSTRB, x_t_TLAST};

always @(posedge ap_clk or negedge ap_rst_n) begin
    if (!ap_rst_n) begin
        state      <= S_DRAIN;
        idx        <= 4'd0;
        h_acc      <= 32'sd0;
        h_value    <= 32'sd0;
        u_t_TDATA  <= 16'd0;
        u_t_TVALID <= 1'b0;
        u_t_TLAST  <= 1'b0;
        for (j = 0; j < DIM; j = j + 1) begin
            x_buf[j] <= 32'sd0;
        end
    end else begin
        case (state)
            S_DRAIN: begin
                u_t_TVALID <= 1'b0;
                u_t_TDATA  <= 16'd0;
                u_t_TLAST  <= 1'b0;

                if (x_t_TVALID && x_t_TREADY) begin
                    x_buf[idx] <= sign_extend16(x_t_TDATA);

                    if (idx == 4'd15) begin
                        idx   <= 4'd0;
                        h_acc <= boundary_term;
                        state <= S_DOT_MUL;
                    end else begin
                        idx <= idx + 4'd1;
                    end
                end
            end

            S_DOT_MUL: begin
                h_acc  <= h_acc + (x_buf[idx] * weight_for_idx(idx));
                if (idx == 4'd15) begin
                    h_value <= h_acc + (x_buf[idx] * weight_for_idx(idx));
                    idx     <= 4'd0;
                    state   <= S_TX_CALC;
                end else begin
                    idx   <= idx + 4'd1;
                    state <= S_DOT_MUL;
                end
            end

            S_TX_CALC: begin
                u_t_TDATA  <= steer_word(h_value, idx);
                u_t_TVALID <= 1'b0;
                u_t_TLAST  <= (idx == 4'd15);
                state      <= S_TX_SEND;
            end

            S_TX_SEND: begin
                u_t_TVALID <= 1'b1;
                u_t_TLAST  <= (idx == 4'd15);

                if (u_t_TVALID && u_t_TREADY) begin
                    if (idx == 4'd15) begin
                        idx        <= 4'd0;
                        u_t_TVALID <= 1'b0;
                        u_t_TLAST  <= 1'b0;
                        state      <= S_DRAIN;
                    end else begin
                        idx        <= idx + 4'd1;
                        u_t_TVALID <= 1'b0;
                        state      <= S_TX_CALC;
                    end
                end
            end

            default: begin
                state      <= S_DRAIN;
                idx        <= 4'd0;
                h_acc      <= 32'sd0;
                h_value    <= 32'sd0;
                u_t_TDATA  <= 16'd0;
                u_t_TVALID <= 1'b0;
                u_t_TLAST  <= 1'b0;
            end
        endcase
    end
end

endmodule
