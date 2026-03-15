# Aegis-Chip USB Mini-USB Handover

**Prepared for:** Incoming engineer modifying the transport path  
**Date:** 2026-03-14  
**Board:** ALINX AX7102 Rev 1.1  
**Target change:** stop using the Ethernet RJ45 path and use the board's **miniUSB UART port** instead

---

## 1. Executive summary

The current active top-level, `rtl/aegis_top.v`, is a **UART-in, Ethernet-out** integration:

```text
USB/CP2102 UART input -> uart_rx -> aegis_uart_bridge -> aegis_shield -> UDP/Ethernet output
```

The user now wants to use the **miniUSB port instead of the RJ45 port**.

### The most important hardware fact
The AX7102 miniUSB port in this project is **not a native USB 2.0 interface into the FPGA fabric**.
It is a **CP2102 USB-to-UART bridge**.

That means the practical transport change is:

```text
USB serial (CP2102) input/output over UART1
```

not:

```text
native USB 2.0 protocol implemented in FPGA
```

So the colleague should think of this task as:

> **replace Ethernet output with UART output on the CP2102 miniUSB port**

not as a USB PHY/device-core project.

---

## 2. Recommended target architecture

### Recommended simplest architecture
Use the same miniUSB/CP2102 serial link for both:
- sending the 16-word input vector into the FPGA
- receiving the 16-word steering vector back from the FPGA

```text
Host PC / Raspberry Pi
   -> USB cable
   -> board miniUSB (CP2102)
   -> UART1 RX/TX
   -> FPGA CC-CBF engine
   -> UART1 TX/RX
   -> same USB cable back to host
```

This is basically a return to the **original UART loopback-style architecture**, but with the current pure-Verilog `aegis_shield` and the current 100 MHz/timing-closed control path.

### Why this is the right choice
- already supported by the board hardware
- already supported by existing RTL modules: `uart_rx.v` and `uart_tx.v`
- already supported by existing host test harness: `test_aegis.py`
- avoids the currently failing Ethernet/PHY bring-up path entirely

---

## 3. Board / port truth

### miniUSB path
The miniUSB UART path is via the **CP2102GM** bridge.

From the repo constraints and docs:
- `UART1_RXD = Y12` — FPGA receives data from CP2102
- `UART1_TXD = Y11` — FPGA transmits data to CP2102
- I/O standard: `LVCMOS33`

This is already reflected in:
- `constraints/aegis_ax7102.xdc`
- `BURN_AND_TEST.md`
- `HANDOVER.md`

### Important limitation
If someone asks for “USB 2.0 miniUSB output” in the strict sense, the answer is:
- **the current board path exposed in this repo is USB-UART via CP2102**
- **not** direct USB signaling from FPGA logic

So no USB device core, descriptors, endpoints, ULPI/UTMI, etc. are needed for the requested change.

---

## 4. Current code status

### Active top-level
Current top-level is:
- `rtl/aegis_top.v`

It currently:
- accepts input on `uart_rxd`
- runs the CC-CBF pipeline
- writes the result into RAM for UDP transmission
- drives Ethernet GMII/UDP logic
- **does not currently send the CC-CBF result back out on UART**

### Current UART output state
In `rtl/aegis_top.v`, UART TX is effectively disabled with:

```verilog
assign uart_txd = 1'b1;
```

So one of the main code changes is to replace that with a real `uart_tx` instance again.

### Existing reusable modules
These modules are already present and usable:
- `rtl/uart_rx.v`
- `rtl/uart_tx.v`
- `rtl/aegis_uart_bridge.v`
- `rtl/aegis_shield.v`

### Existing host-side reusable tool
The old UART golden-model harness is already implemented:
- `test_aegis.py`

It already matches the transport/protocol we want:
- TX to FPGA: `16 x INT16`, 32 bytes total, little-endian
- RX from FPGA: `16 x INT16`, 32 bytes total, little-endian
- UART: `115200`, `8N1`

That is a huge advantage: the software side does **not** need to be invented from scratch.

---

## 5. What to remove or bypass

For the USB/miniUSB pivot, the colleague should stop depending on the Ethernet output path.

### Can be bypassed or removed from the data path
In `rtl/aegis_top.v`, these are Ethernet-only for the current design intent:
- PHY reset / MDIO management logic
- `gmii_arbi`
- `udp`
- payload RAM used for Ethernet TX
- Ethernet LEDs/debug signals
- GMII top-level outputs if building a UART-only image

### Keep if a dual-mode debug build is desired
If the colleague wants a dual-mode debug top later, they can keep Ethernet logic under a compile switch or separate top.
But for the requested task, the simplest path is:

> **remove Ethernet from the active payload path and restore UART TX as the output transport**

---

## 6. Recommended RTL modification plan

### Goal
Take the output of `aegis_shield` and serialize it back over `uart_txd` through `uart_tx.v`.

### Existing pipeline up to shield output
Current useful pipeline already exists:

```text
uart_rx
-> aegis_uart_bridge
-> aegis_shield
```

### Proposed new output path
Instead of:

```text
aegis_shield -> vector_to_ram -> UDP/Ethernet
```

use:

```text
aegis_shield -> uart_tx
```

### Important protocol detail
`uart_tx.v` already expects:
- 16-bit words on AXI-stream-like handshake
- little-endian serialization on the wire

`aegis_shield` already emits:
- 16-bit words
- valid/ready handshake

So the colleague should be able to connect the shield output to `uart_tx` with little or no format conversion.

---

## 7. Concrete code changes expected

### In `rtl/aegis_top.v`

#### Keep
- clock generation (`sys_clk_200m -> sys_clk_core`)
- `btn_anxiety`
- `uart_rx`
- `aegis_uart_bridge`
- `aegis_shield`

#### Remove/bypass from the active path
- `aegis_vector_to_ram`
- Ethernet PHY reset logic
- `smi_config`
- `gmii_arbi`
- `udp`
- `dp_ram` for Ethernet payload buffering
- Ethernet activity LED logic

#### Replace
Current stub:

```verilog
assign uart_txd = 1'b1;
```

with a real instance of:

```verilog
uart_tx
```

fed from:
- `shield_out_tdata`
- `shield_out_tvalid`
- `shield_out_tready`

### Suggested connection sketch
Conceptually:

```verilog
uart_tx #(
    .CLK_FREQ  (100_000_000),
    .BAUD_RATE (115_200)
) u_uart_tx (
    .clk            (sys_clk_core),
    .rst_n          (reset_n),
    .s_axis_tdata   (shield_out_tdata),
    .s_axis_tvalid  (shield_out_tvalid),
    .s_axis_tready  (shield_out_tready),
    .txd            (uart_txd)
);
```

This is the shortest path to a working miniUSB transport.

---

## 8. Constraints expectations

### Good news
The UART1 miniUSB/CP2102 constraints are already in place in:
- `constraints/aegis_ax7102.xdc`

Relevant lines already match the miniUSB UART path:
- `uart_rxd -> Y12`
- `uart_txd -> Y11`
- `LVCMOS33`

### Therefore
The colleague probably does **not** need to change pin constraints at all for the miniUSB pivot, as long as they keep using UART1 / CP2102.

---

## 9. Host software / validation plan

### Best starting point
Use:
- `test_aegis.py`

This script already assumes exactly the UART protocol needed for the miniUSB plan.

### Existing protocol
Per `test_aegis.py` and `uart_tx.v` / `uart_rx.v`:
- frame size in: `32 bytes = 16 x int16`
- frame size out: `32 bytes = 16 x int16`
- endianness: little-endian
- baud: `115200`
- framing: `8N1`

### Practical validation
1. Program the new UART-output bitstream
2. Plug in the CP2102 miniUSB cable
3. Find the serial port, e.g. `/dev/ttyUSB1`
4. Run:

```bash
python3 test_aegis.py /dev/ttyUSB1
```

If the colleague keeps the protocol unchanged, this script should work with minimal or no modification.

---

## 10. Architecture caveat for the colleague

### If the user still wants Raspberry Pi in the loop
Be careful: moving to the miniUSB CP2102 transport changes the system architecture.

#### Works well
- one host connected via miniUSB does both TX and RX over one serial link

#### Does not preserve the old split I/O architecture
If the old requirement was:

```text
RPi -> FPGA -> PC
```

then a single miniUSB CP2102 UART link does **not** naturally preserve that split unless:
- the Pi itself is the host connected to miniUSB, and
- the Pi handles both sending inputs and receiving outputs

So the colleague should confirm which of these is desired:

### Option A — simplest and recommended
One host over miniUSB handles both directions.

### Option B — preserve separate source/sink devices
Then miniUSB alone is not a drop-in replacement for Ethernet. Another physical path would be needed.

For the likely user intent, **Option A** is the correct implementation.

---

## 11. Suggested cleanup after the change

If the colleague completes the UART-only pivot, they should consider:
- simplifying `rtl/aegis_top.v`
- removing unused Ethernet signals from the active top
- optionally creating a dedicated top-level such as:
  - `rtl/aegis_top_uart_usb.v`

This is cleaner than keeping a half-disabled Ethernet integration in the main top.

A good outcome would be:
- one stable UART/miniUSB build for practical use
- optionally keep the Ethernet experiment in separate files/scripts for later

---

## 12. Practical implementation checklist

### Must-do
- [ ] restore real `uart_tx` output in `rtl/aegis_top.v`
- [ ] wire `aegis_shield` output directly into `uart_tx`
- [ ] remove or bypass Ethernet output path from the active dataflow
- [ ] keep UART1 on CP2102 miniUSB pins `Y12/Y11`
- [ ] build/program bitstream
- [ ] validate with `test_aegis.py`

### Nice-to-have
- [ ] create a dedicated UART-only top module or branch
- [ ] add a short README or note documenting the transport pivot
- [ ] keep Ethernet-specific experiments isolated from the new active build

---

## 13. File map for the colleague

### Read first
- `HANDOVER.md`
- `TIMING_CLOSURE.md`
- `BURN_AND_TEST.md`

### Edit likely
- `rtl/aegis_top.v`

### Reuse directly
- `rtl/uart_rx.v`
- `rtl/uart_tx.v`
- `rtl/aegis_uart_bridge.v`
- `rtl/aegis_shield.v`
- `test_aegis.py`
- `constraints/aegis_ax7102.xdc`
- `scripts/build_vivado.tcl`

### Ignore for this transport change
- `ethernet_test/`
- Ethernet PHY/GMII debug branches
- UDP listener tooling under `host/`

---

## 14. Bottom line

The miniUSB request should be implemented as:

> **CP2102 UART transport over UART1 (Y12/Y11)**

not as native USB 2.0 logic.

The shortest reliable path is to:
1. keep the current UART receive path
2. remove Ethernet from the output path
3. reconnect `aegis_shield` output to `uart_tx`
4. use `test_aegis.py` as the validation harness

That is the lowest-risk, highest-confidence route.

---

*End of USB handover.*
