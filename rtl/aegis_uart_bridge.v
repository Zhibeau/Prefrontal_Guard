`timescale 1ns / 1ps

module aegis_uart_bridge #(
    parameter integer DIM = 16
)(
    input  wire        clk,
    input  wire        rst_n,

    input  wire [15:0] s_axis_tdata,
    input  wire        s_axis_tvalid,
    output reg         s_axis_tready,

    output reg  [15:0] m_axis_tdata,
    output reg         m_axis_tvalid,
    input  wire        m_axis_tready,

    output reg         frame_captured,
    output reg         frame_replayed
);

localparam [0:0] ST_CAPTURE = 1'b0,
                 ST_REPLAY  = 1'b1;

reg        state;
reg [3:0]  capture_idx;
reg [3:0]  replay_idx;
reg [15:0] frame_buf [0:DIM-1];

integer i;

always @(posedge clk or negedge rst_n) begin
    if (!rst_n) begin
        state          <= ST_CAPTURE;
        capture_idx    <= 4'd0;
        replay_idx     <= 4'd0;
        s_axis_tready  <= 1'b1;
        m_axis_tdata   <= 16'd0;
        m_axis_tvalid  <= 1'b0;
        frame_captured <= 1'b0;
        frame_replayed <= 1'b0;
        for (i = 0; i < DIM; i = i + 1) begin
            frame_buf[i] <= 16'd0;
        end
    end else begin
        frame_captured <= 1'b0;
        frame_replayed <= 1'b0;

        case (state)
            ST_CAPTURE: begin
                s_axis_tready <= 1'b1;
                m_axis_tvalid <= 1'b0;

                if (s_axis_tvalid && s_axis_tready) begin
                    frame_buf[capture_idx] <= s_axis_tdata;

                    if (capture_idx == DIM-1) begin
                        capture_idx    <= 4'd0;
                        replay_idx     <= 4'd0;
                        s_axis_tready  <= 1'b0;
                        m_axis_tdata   <= frame_buf[0];
                        m_axis_tvalid  <= 1'b1;
                        frame_captured <= 1'b1;
                        state          <= ST_REPLAY;
                    end else begin
                        capture_idx <= capture_idx + 1'b1;
                    end
                end
            end

            ST_REPLAY: begin
                s_axis_tready <= 1'b0;
                m_axis_tvalid <= 1'b1;
                // Do NOT unconditionally update tdata here — the current
                // registered value was pre-loaded in ST_CAPTURE (word 0) or
                // on the previous handshake (word N+1).  Overwriting every
                // cycle causes an off-by-one: the shield receives word 0
                // twice and never receives word 15.

                if (m_axis_tvalid && m_axis_tready) begin
                    if (replay_idx == DIM-1) begin
                        replay_idx     <= 4'd0;
                        capture_idx    <= 4'd0;
                        s_axis_tready  <= 1'b1;
                        m_axis_tvalid  <= 1'b0;
                        frame_replayed <= 1'b1;
                        state          <= ST_CAPTURE;
                    end else begin
                        // Pre-load next word so it is stable on the next cycle
                        m_axis_tdata <= frame_buf[replay_idx + 1'b1];
                        replay_idx   <= replay_idx + 1'b1;
                    end
                end
            end

            default: begin
                state         <= ST_CAPTURE;
                s_axis_tready <= 1'b1;
                m_axis_tvalid <= 1'b0;
            end
        endcase
    end
end

endmodule