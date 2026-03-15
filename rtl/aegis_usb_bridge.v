// =============================================================================
// aegis_usb_bridge.v — CY68013A FX2LP Slave-FIFO ↔ AXI-Stream bridge
// =============================================================================
//
// Connects the Aegis-Chip CC-CBF pipeline to the board's CY68013A EZ-USB
// FX2LP chip via its synchronous slave-FIFO interface.
//
// Data flow per transaction:
//   1. Wait for EP2 not-empty (usb_flaga = 1)
//   2. Read  WORDS_PER_FRAME(16) × 16-bit words from EP2 into rx_buf
//   3. Stream rx_buf one word at a time via AXI-Stream master
//      → feeds aegis_uart_bridge → aegis_shield
//   4. Collect WORDS_PER_FRAME(16) output words from aegis_shield into tx_buf
//   5. Write TX_PACKET_WORDS(256) × 16-bit words to EP6:
//      words  0..15  = real shield output (tx_buf)
//      words 16..255 = zero padding
//      → fills a 512-byte USB packet so FX2LP auto-commits it to the host
//   6. Return to step 1
//
// NOTE: The CY68013A on this board has no PKTEND wired to the FPGA.
// The FX2LP only commits IN data to the USB host when its 512-byte FIFO
// is completely full. We pad every EP6 write to exactly 512 bytes.
//
// CY68013A pin assignments (from vendor usb.xdc):
//   usb_fifoaddr[1:0] : endpoint select  00=EP2 (host→FPGA), 10=EP6 (FPGA→host)
//   usb_slcs          : chip-select, active-low  (tied permanently low)
//   usb_sloe          : output-enable, active-low (FX2LP drives FD during read)
//   usb_slrd          : read  strobe,  active-low (advances EP2 FIFO pointer)
//   usb_slwr          : write strobe,  active-low (writes into EP6 FIFO)
//   usb_fd[15:0]      : bidirectional 16-bit data bus
//   usb_flaga         : 1 = EP2 FIFO not empty  (data available for FPGA)
//   usb_flagb         : EP4 status, unused here
//   usb_flagc         : 1 = EP6 FIFO not full   (FPGA may write)
//
// Timing at sys_clk_core = 100 MHz:
//   OE_SETUP_CYCS = 5   50 ns  (FX2LP min: ~10 ns)
//   RD_PULSE_CYCS = 5   50 ns  (FX2LP min: 18.7 ns)
//   RD_HOLD_CYCS  = 3   30 ns  inter-word gap (OE stays asserted)
//   WR_SETUP_CYCS = 3   30 ns  data-setup before SLWR↓
//   WR_PULSE_CYCS = 5   50 ns  (FX2LP min: 18.7 ns)
//   WR_HOLD_CYCS  = 3   30 ns  data-hold after SLWR↑
// =============================================================================

`timescale 1ns / 1ps

module aegis_usb_bridge #(
    parameter integer WORDS_PER_FRAME  = 16,  // RX: words read from EP2 / collected from shield
    parameter integer TX_PACKET_WORDS  = 256, // EP6 write count: 256 × 2 = 512 bytes (fills USB packet)
    parameter integer OE_SETUP_CYCS   = 5,
    parameter integer RD_PULSE_CYCS   = 5,
    parameter integer RD_HOLD_CYCS    = 3,
    parameter integer WR_SETUP_CYCS   = 3,
    parameter integer WR_PULSE_CYCS   = 5,
    parameter integer WR_HOLD_CYCS    = 3
)(
    input  wire        clk,
    input  wire        rst_n,

    // ---- CY68013A FX2LP slave FIFO interface --------------------------------
    output reg  [1:0]  usb_fifoaddr,
    output wire        usb_slcs,         // chip-select (tied low)
    output reg         usb_sloe,         // output enable, active-low
    output reg         usb_slrd,         // read  strobe,  active-low
    output reg         usb_slwr,         // write strobe,  active-low
    inout  wire [15:0] usb_fd,           // bidirectional data bus
    input  wire        usb_flaga,        // 1 = EP2 not empty
    input  wire        usb_flagb,        // unused (EP4 status)
    input  wire        usb_flagc,        // 1 = EP6 not full

    // ---- AXI-Stream master → aegis_uart_bridge (shield input) --------------
    output reg  [15:0] m_axis_tdata,
    output reg         m_axis_tvalid,
    input  wire        m_axis_tready,

    // ---- AXI-Stream slave  ← aegis_shield output ----------------------------
    input  wire [15:0] s_axis_tdata,
    input  wire        s_axis_tvalid,
    output reg         s_axis_tready,

    // ---- Status pulses (one clock wide) -------------------------------------
    output reg         rx_frame_done,    // 16 words received from EP2
    output reg         tx_frame_done,    // 16 words sent to EP6

    // ---- Temporary bring-up debug -------------------------------------------
    output wire        dbg_flaga_sync,
    output wire        dbg_left_idle
);

// -----------------------------------------------------------------------------
// Chip-select tied permanently low (always selected)
// -----------------------------------------------------------------------------
assign usb_slcs = 1'b0;

// -----------------------------------------------------------------------------
// Bidirectional data bus tri-state
// -----------------------------------------------------------------------------
reg [15:0] usb_fd_out;
reg        usb_fd_oe;

assign usb_fd = usb_fd_oe ? usb_fd_out : 16'bz;

// -----------------------------------------------------------------------------
// Internal word buffers — small, 16 × 16 = 256 bits each
// -----------------------------------------------------------------------------
reg [15:0] rx_buf [0:WORDS_PER_FRAME-1];
reg [15:0] tx_buf [0:WORDS_PER_FRAME-1];

// -----------------------------------------------------------------------------
// Counters and timing
// -----------------------------------------------------------------------------
reg [3:0] rx_word_cnt;    // EP2 read index,       0 … WORDS_PER_FRAME-1
reg [3:0] rx_stream_cnt;  // AXI master word index, 0 … WORDS_PER_FRAME-1
reg [3:0] tx_collect_cnt; // shield output index,  0 … WORDS_PER_FRAME-1
reg [7:0] tx_word_cnt;    // EP6 write index,       0 … TX_PACKET_WORDS-1
reg [7:0] timer;          // intra-state timing counter (must cover 35-cycle FX2 strobes)

// 2-FF metastability synchronisers for asynchronous FX2LP flag inputs
reg flaga_meta, flaga_sync;
reg flagc_meta, flagc_sync;
wire usb_flaga_s = flaga_sync;
wire usb_flagc_s = flagc_sync;

assign dbg_flaga_sync = usb_flaga_s;
assign dbg_left_idle  = (state != S_IDLE);

// -----------------------------------------------------------------------------
// State encoding
// -----------------------------------------------------------------------------
localparam [3:0]
    S_IDLE          = 4'd0,
    S_EP2_OE_SETUP  = 4'd1,   // assert SLOE, wait OE_SETUP_CYCS (first word)
    S_EP2_RD_ASSERT = 4'd2,   // assert SLRD, wait RD_PULSE_CYCS, capture FD
    S_EP2_RD_HOLD   = 4'd3,   // deassert SLRD, wait RD_HOLD_CYCS
    S_EP2_OE_DEASS  = 4'd4,   // deassert SLOE after all words read
    S_STREAM_OUT    = 4'd5,   // push rx_buf words to AXI-Stream master
    S_COLLECT       = 4'd6,   // capture aegis_shield output into tx_buf
    S_EP6_WAIT      = 4'd7,   // wait for EP6 not-full
    S_EP6_WR_SETUP  = 4'd8,   // drive data bus, wait WR_SETUP_CYCS
    S_EP6_WR_ASSERT = 4'd9,   // assert SLWR, wait WR_PULSE_CYCS
    S_EP6_WR_HOLD   = 4'd10,  // deassert SLWR, wait WR_HOLD_CYCS
    S_EP6_WR_NEXT   = 4'd11;  // advance TX word index or return to IDLE

reg [3:0] state;

integer j;

// -----------------------------------------------------------------------------
// 2-FF synchronisers for FX2LP flag inputs
// -----------------------------------------------------------------------------
always @(posedge clk or negedge rst_n) begin
    if (!rst_n) begin
        flaga_meta <= 1'b0; flaga_sync <= 1'b0;
        flagc_meta <= 1'b0; flagc_sync <= 1'b0;
    end else begin
        flaga_meta <= usb_flaga; flaga_sync <= flaga_meta;
        flagc_meta <= usb_flagc; flagc_sync <= flagc_meta;
    end
end

// -----------------------------------------------------------------------------
// Main FSM
// -----------------------------------------------------------------------------
always @(posedge clk or negedge rst_n) begin
    if (!rst_n) begin
        state          <= S_IDLE;
        usb_fifoaddr   <= 2'b00;
        usb_sloe       <= 1'b1;
        usb_slrd       <= 1'b1;
        usb_slwr       <= 1'b1;
        usb_fd_oe      <= 1'b0;
        usb_fd_out     <= 16'd0;
        m_axis_tdata   <= 16'd0;
        m_axis_tvalid  <= 1'b0;
        s_axis_tready  <= 1'b0;
        rx_word_cnt    <= 4'd0;
        rx_stream_cnt  <= 4'd0;
        tx_collect_cnt <= 4'd0;
        tx_word_cnt    <= 8'd0;
        timer          <= 5'd0;
        rx_frame_done  <= 1'b0;
        tx_frame_done  <= 1'b0;
        for (j = 0; j < WORDS_PER_FRAME; j = j + 1) begin
            rx_buf[j] <= 16'd0;
            tx_buf[j] <= 16'd0;
        end
    end else begin
        // Default: clear one-cycle pulses
        rx_frame_done <= 1'b0;
        tx_frame_done <= 1'b0;

        case (state)

            // ------------------------------------------------------------------
            // IDLE: deassert all strobes; wait for EP2 data
            // ------------------------------------------------------------------
            S_IDLE: begin
                usb_fifoaddr  <= 2'b00;
                usb_sloe      <= 1'b1;
                usb_slrd      <= 1'b1;
                usb_slwr      <= 1'b1;
                usb_fd_oe     <= 1'b0;
                m_axis_tvalid <= 1'b0;
                s_axis_tready <= 1'b0;
                rx_word_cnt   <= 4'd0;
                timer         <= 5'd0;

                if (usb_flaga_s) begin       // EP2 has at least one word (synchronised)
                    state <= S_EP2_OE_SETUP;
                end
            end

            // ------------------------------------------------------------------
            // EP2_OE_SETUP: assert SLOE so FX2LP drives the FD bus.
            // Only needed once; subsequent words reuse the asserted OE.
            // ------------------------------------------------------------------
            S_EP2_OE_SETUP: begin
                usb_fifoaddr <= 2'b00;
                usb_sloe     <= 1'b0;   // FX2LP drives FD
                usb_slrd     <= 1'b1;

                if (timer == OE_SETUP_CYCS - 1) begin
                    timer <= 5'd0;
                    state <= S_EP2_RD_ASSERT;
                end else begin
                    timer <= timer + 1'b1;
                end
            end

            // ------------------------------------------------------------------
            // EP2_RD_ASSERT: pulse SLRD low; capture FD on the last cycle.
            // SLOE stays asserted (low) throughout all 16 reads.
            // ------------------------------------------------------------------
            S_EP2_RD_ASSERT: begin
                usb_sloe <= 1'b0;
                usb_slrd <= 1'b0;   // assert read strobe → FIFO pointer advances

                if (timer == RD_PULSE_CYCS - 1) begin
                    rx_buf[rx_word_cnt] <= usb_fd;   // capture stable data
                    timer <= 5'd0;
                    state <= S_EP2_RD_HOLD;
                end else begin
                    timer <= timer + 1'b1;
                end
            end

            // ------------------------------------------------------------------
            // EP2_RD_HOLD: deassert SLRD; inter-word gap (OE stays low).
            // After gap, either read next word or deassert OE and stream out.
            // ------------------------------------------------------------------
            S_EP2_RD_HOLD: begin
                usb_sloe <= 1'b0;   // keep SLOE for continuous read
                usb_slrd <= 1'b1;   // deassert read strobe

                if (timer == RD_HOLD_CYCS - 1) begin
                    timer <= 5'd0;
                    if (rx_word_cnt == WORDS_PER_FRAME - 1) begin
                        rx_frame_done <= 1'b1;
                        state         <= S_EP2_OE_DEASS;
                    end else begin
                        rx_word_cnt <= rx_word_cnt + 1'b1;
                        state       <= S_EP2_RD_ASSERT; // OE already asserted
                    end
                end else begin
                    timer <= timer + 1'b1;
                end
            end

            // ------------------------------------------------------------------
            // EP2_OE_DEASS: release FD bus back to FPGA control.
            // ------------------------------------------------------------------
            S_EP2_OE_DEASS: begin
                usb_sloe      <= 1'b1;
                usb_slrd      <= 1'b1;
                rx_stream_cnt <= 4'd0;
                state         <= S_STREAM_OUT;
            end

            // ------------------------------------------------------------------
            // STREAM_OUT: forward rx_buf → AXI-Stream master one word at a time.
            //
            // Protocol:
            //   - When tvalid=0: load first word, assert tvalid.
            //   - When tvalid=1 and tready=1 (handshake):
            //       last word  → deassert tvalid, go to COLLECT.
            //       other word → pre-load next word, keep tvalid.
            //   - When tvalid=1 and tready=0: hold, wait for consumer.
            // ------------------------------------------------------------------
            S_STREAM_OUT: begin
                if (m_axis_tvalid && m_axis_tready) begin
                    // Handshake: this word accepted
                    if (rx_stream_cnt == WORDS_PER_FRAME - 1) begin
                        m_axis_tvalid  <= 1'b0;
                        tx_collect_cnt <= 4'd0;
                        s_axis_tready  <= 1'b1;
                        state          <= S_COLLECT;
                    end else begin
                        rx_stream_cnt <= rx_stream_cnt + 1'b1;
                        m_axis_tdata  <= rx_buf[rx_stream_cnt + 4'd1]; // pre-load
                        // tvalid stays 1'b1
                    end
                end else if (!m_axis_tvalid) begin
                    // Entry: present word 0 and assert tvalid
                    m_axis_tdata  <= rx_buf[4'd0];
                    m_axis_tvalid <= 1'b1;
                end
                // else: tvalid=1, tready=0 → hold current tdata/tvalid
            end

            // ------------------------------------------------------------------
            // COLLECT: receive aegis_shield output words via AXI-Stream slave.
            // s_axis_tready is held high; words captured into tx_buf.
            // ------------------------------------------------------------------
            S_COLLECT: begin
                s_axis_tready <= 1'b1;

                if (s_axis_tvalid && s_axis_tready) begin
                    tx_buf[tx_collect_cnt] <= s_axis_tdata;

                    if (tx_collect_cnt == WORDS_PER_FRAME - 1) begin
                        s_axis_tready <= 1'b0;
                        tx_word_cnt   <= 8'd0;
                        state         <= S_EP6_WAIT;
                    end else begin
                        tx_collect_cnt <= tx_collect_cnt + 1'b1;
                    end
                end
            end

            // ------------------------------------------------------------------
            // EP6_WAIT: wait for EP6 FIFO to have space before writing.
            // ------------------------------------------------------------------
            S_EP6_WAIT: begin
                usb_fifoaddr <= 2'b10;   // select EP6
                usb_slwr     <= 1'b1;    // keep write strobe deasserted while waiting
                usb_fd_oe    <= 1'b0;    // release FD bus until the next word is launched
                timer        <= 5'd0;

                if (usb_flagc_s) begin   // EP6 not full → start write (synchronised)
                    state <= S_EP6_WR_SETUP;
                end
            end

            // ------------------------------------------------------------------
            // EP6_WR_SETUP: drive FD bus with tx word, wait data-setup time.
            // ------------------------------------------------------------------
            S_EP6_WR_SETUP: begin
                usb_fifoaddr <= 2'b10;
                usb_fd_oe    <= 1'b1;                      // FPGA drives FD
                // words 0..15: real shield output; words 16..255: zero padding
                usb_fd_out   <= (tx_word_cnt < WORDS_PER_FRAME)
                                ? tx_buf[tx_word_cnt[3:0]] : 16'd0;
                usb_slwr     <= 1'b1;

                if (timer == WR_SETUP_CYCS - 1) begin
                    timer <= 5'd0;
                    state <= S_EP6_WR_ASSERT;
                end else begin
                    timer <= timer + 1'b1;
                end
            end

            // ------------------------------------------------------------------
            // EP6_WR_ASSERT: assert SLWR low for WR_PULSE_CYCS cycles.
            // ------------------------------------------------------------------
            S_EP6_WR_ASSERT: begin
                usb_fd_out <= (tx_word_cnt < WORDS_PER_FRAME)
                             ? tx_buf[tx_word_cnt[3:0]] : 16'd0;  // hold stable
                usb_slwr   <= 1'b0;                 // write strobe

                if (timer == WR_PULSE_CYCS - 1) begin
                    timer <= 5'd0;
                    state <= S_EP6_WR_HOLD;
                end else begin
                    timer <= timer + 1'b1;
                end
            end

            // ------------------------------------------------------------------
            // EP6_WR_HOLD: deassert SLWR; hold data for WR_HOLD_CYCS cycles.
            // ------------------------------------------------------------------
            S_EP6_WR_HOLD: begin
                usb_slwr <= 1'b1;   // deassert write strobe

                if (timer == WR_HOLD_CYCS - 1) begin
                    timer <= 5'd0;
                    state <= S_EP6_WR_NEXT;
                end else begin
                    timer <= timer + 1'b1;
                end
            end

            // ------------------------------------------------------------------
            // EP6_WR_NEXT: advance TX counter; loop or finish.
            // Loops 256 times (TX_PACKET_WORDS) to fill a 512-byte USB packet.
            // Re-check FLAGC before every word so a new packet is only launched
            // when EP6 has actually re-armed after the previous write.
            // ------------------------------------------------------------------
            S_EP6_WR_NEXT: begin
                if (tx_word_cnt == TX_PACKET_WORDS - 1) begin
                    usb_fd_oe     <= 1'b0;   // release FD bus
                    tx_frame_done <= 1'b1;
                    state         <= S_IDLE;
                end else begin
                    tx_word_cnt <= tx_word_cnt + 8'd1;
                    state       <= S_EP6_WAIT;
                end
            end

            default: state <= S_IDLE;

        endcase
    end
end

// Suppress unused-port warnings for flagb
wire _unused_flagb = usb_flagb;

endmodule
