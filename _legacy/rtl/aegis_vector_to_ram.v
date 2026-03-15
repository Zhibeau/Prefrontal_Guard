`timescale 1ns / 1ps

module aegis_vector_to_ram #(
    parameter integer DIM = 16,
    parameter integer RAM_WORDS = 8
)(
    input  wire        clk,
    input  wire        rst_n,

    input  wire [15:0] s_axis_tdata,
    input  wire        s_axis_tvalid,
    output wire        s_axis_tready,

    output reg         ram_wr_en,
    output reg  [8:0]  ram_wr_addr,
    output reg  [31:0] ram_wr_data,
    output reg         frame_written
);

reg [15:0] first_word;
reg [3:0]  word_idx;

assign s_axis_tready = 1'b1;

always @(posedge clk or negedge rst_n) begin
    if (!rst_n) begin
        first_word    <= 16'd0;
        word_idx      <= 4'd0;
        ram_wr_en     <= 1'b0;
        ram_wr_addr   <= 9'd0;
        ram_wr_data   <= 32'd0;
        frame_written <= 1'b0;
    end else begin
        ram_wr_en     <= 1'b0;
        frame_written <= 1'b0;

        if (s_axis_tvalid && s_axis_tready) begin
            if (!word_idx[0]) begin
                first_word <= s_axis_tdata;
                word_idx   <= word_idx + 1'b1;
            end else begin
                ram_wr_addr <= {5'd0, word_idx[3:1]};
                ram_wr_data <= {
                    first_word[7:0],
                    first_word[15:8],
                    s_axis_tdata[7:0],
                    s_axis_tdata[15:8]
                };
                ram_wr_en <= 1'b1;

                if (word_idx == DIM-1) begin
                    word_idx      <= 4'd0;
                    frame_written <= 1'b1;
                end else begin
                    word_idx <= word_idx + 1'b1;
                end
            end
        end
    end
end

endmodule