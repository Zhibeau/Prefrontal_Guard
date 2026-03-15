# Aegis-Chip: Burn & Test Guide

**Goal:** Program the bitstream onto the AX7102 board, set the anxiety
level using the four push-buttons, then verify the CC-CBF steering-vector
calculation over UART — without needing an LLM or biosensors.

---

## Prerequisites

### Hardware
| Item | Notes |
|------|-------|
| AX7102 board | Powered via 12 V DC barrel or USB |
| Micro-USB cable (×2) | One for JTAG (FTDI), one for UART (CP2102) |
| PC running Linux | (Ubuntu 20.04+ recommended) |

### Software
```bash
# Vivado 2025.2 (already installed)
source /home/midu/vivado/2025.2/Vivado/settings64.sh

# Python serial library
pip install pyserial
```

---

## Part 1 — Programming the Bitstream

### 1.1 Confirm the bitstream exists
```bash
ls -lh impl/aegis_chip.bit
# Expected: ~1.02 MB, dated today
```

### 1.2 Connect the JTAG cable
Plug the **JTAG Micro-USB** cable into the board's JTAG port (J1 on the
core board, labelled **TCK TDO TDI TMS** on the silkscreen).
Power the board. The **PWR LED** (red) should light up.

### 1.3 Program via Vivado Hardware Manager (GUI)

1. Launch Vivado:
   ```bash
   vivado &
   ```
2. **Flow Navigator → Open Hardware Manager → Open Target → Auto Connect**
3. The device `xc7a100t_0` appears in the Hardware panel.
4. Right-click → **Program Device**
5. Browse to `impl/aegis_chip.bit`, click **Program**.
6. The **DONE LED** (blue) lights up within ~3 seconds → programming successful.

### 1.4 Program via Vivado TCL console (faster, no GUI)
```tcl
# Paste this block into the Vivado TCL console:
open_hw_manager
connect_hw_server
open_hw_target
set_property PROGRAM.FILE \
    {/home/midu/aegis_fpga/impl/aegis_chip.bit} \
    [get_hw_devices xc7a100t_0]
program_hw_devices [get_hw_devices xc7a100t_0]
refresh_hw_device [get_hw_devices xc7a100t_0]
puts "Programming complete."
```

### 1.5 Program via command-line (no Vivado GUI at all)
```bash
export PATH=/home/midu/vivado/2025.2/Vivado/bin:$PATH

cat > /tmp/program.tcl << 'EOF'
open_hw_manager
connect_hw_server
open_hw_target
set_property PROGRAM.FILE \
    {/home/midu/aegis_fpga/impl/aegis_chip.bit} \
    [get_hw_devices xc7a100t_0]
program_hw_devices [get_hw_devices xc7a100t_0]
close_hw_manager
exit
EOF

vivado -mode batch -source /tmp/program.tcl
```

### 1.6 Verify programming
After programming:
- **DONE LED** is lit (blue) → FPGA configured successfully.
- **PWR LED** is lit (red) → board has power.
- If DONE does not light, check JTAG cable and repeat.

---

## Part 2 — Setting Anxiety Level with Buttons

### Button map (expansion board, active-low, 3.3 V)

```
 ┌────────────────────────────────────────────────┐
 │  KEY1    KEY2    KEY3    KEY4                   │
 │  (B18)   (B17)   (A16)   (A15)                 │
 │  ×512    ×256    ×128    ×64    ← weight        │
 │                                                 │
 │  LED1    LED2    LED3    LED4   ← mirrors key   │
 │  (C17)   (D17)   (V20)   (U20)                 │
 └────────────────────────────────────────────────┘
```

**anxiety_level = (KEY1×8 + KEY2×4 + KEY3×2 + KEY4×1) × 64**

| Buttons pressed | Binary | anxiety_level | B = 5000−(6000×anx>>10) | Meaning |
|-----------------|--------|---------------|------------------------|---------|
| none            | 0000   | 0             | 5000 | Fully calm — very large safe zone |
| KEY4 only       | 0001   | 64            | 4625 | Almost calm |
| KEY3+KEY4       | 0011   | 192           | 3875 | Mild arousal |
| KEY2 only       | 0100   | 256           | 3500 | Low stress |
| KEY1 only       | 1000   | 512           | 2000 | **Medium stress** (default) |
| KEY1+KEY2       | 1100   | 768           | 500  | High stress |
| KEY1+KEY2+KEY3  | 1110   | 896           | -250 | ⚠ B < 0: *always* correcting |
| all four        | 1111   | 960           | -625 | ⚠ Maximum — barrier always triggered |

> **LEDs confirm your selection:** each LED lights up when its button is held.
> The CC-CBF engine reads the new level within one debounce window (~20 ms).

### Quick-reference: key combinations to press before each test
```bash
# Test at anxiety=512 (KEY1):  press and hold KEY1
python3 test_aegis.py /dev/ttyUSB1 --anxiety 512

# Test at anxiety=0 (calm):  release all buttons
python3 test_aegis.py /dev/ttyUSB1 --anxiety 0

# Test at anxiety=960 (max):  press all 4 buttons
python3 test_aegis.py /dev/ttyUSB1 --anxiety 960

# Scan all 16 levels interactively:
python3 test_aegis.py /dev/ttyUSB1 --scan
```

---

## Part 3 — Connecting the UART

### 2.1 Plug in the UART cable
Plug the **CP2102 Micro-USB** cable into the board's **UART port** (J3 on
the expansion board, labelled **UART**).

### 2.2 Find the serial port
```bash
ls /dev/ttyUSB*
# Typical output:
#   /dev/ttyUSB0   ← JTAG (FTDI)
#   /dev/ttyUSB1   ← UART (CP2102)  ← use this one
```
> If only one port appears, the JTAG cable is not connected. Both are needed.

### 2.3 Grant permission (one-time)
```bash
sudo usermod -aG dialout $USER
# Log out and back in, or run:
sudo chmod a+rw /dev/ttyUSB1
```

---

## Part 4 — Running the Tests

### 3.1 Sanity check — UART loopback (optional)
Before testing the FPGA logic, verify UART wiring works. Connect a
jumper wire from **UART_TXD (Y11)** to **UART_RXD (Y12)** on the
expansion header CON2 (pins 21 and 23).

```bash
python3 test_aegis.py /dev/ttyUSB1 --loopback
# Expected: [PASS] Received exactly what was sent
```
Remove the jumper before the next step.

### 3.2 Full CC-CBF test suite
```bash
python3 test_aegis.py /dev/ttyUSB1
```

**Expected output:**
```
====================================================================
  Aegis-Chip CC-CBF Test Harness
  Port: /dev/ttyUSB1   Baud: 115200   anxiety_level=512
  B = 5000 − ((6000×512)>>10) = 2000
  W = [64, 128, 96, 112, 80, 72, 104, 88]
      [120, 60, 92, 116, 76, 84, 100, 68]
====================================================================

[PASS] Safe: zero vector  (h = +2000)
           x_t = [0, 0, 0, 0, 0, 0, …]   h = 2000
           u_t = [0, 0, 0, 0, 0, 0, …]   (5.8 ms)

[PASS] Safe: all +1       (h = +3460)
           ...

[PASS] Violation: all −100 (h = −144000)
           u_t = [9000, 18000, 13500, 15750, …]   (5.8 ms)

...

====================================================================
Results: 10 passed, 0 failed, 0 timeouts  / 10 total

✓ All tests passed — CC-CBF computation verified on hardware.
```

### 3.3 Run a single custom vector
```bash
# Zero vector (safe)
python3 test_aegis.py /dev/ttyUSB1 --single "0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0"

# Large violation
python3 test_aegis.py /dev/ttyUSB1 --single "-100,-100,-100,-100,-100,-100,-100,-100,-100,-100,-100,-100,-100,-100,-100,-100"
```

### 3.4 Interactive mode — explore manually
```bash
python3 test_aegis.py /dev/ttyUSB1 --interactive
```
```
Interactive mode  (type 'quit' to exit)
Enter 16 comma-separated INT16 values for x_t.
anxiety_level=512, B=2000

x_t> -50,-50,-50,-50,-50,-50,-50,-50,-50,-50,-50,-50,-50,-50,-50,-50
  h        = -70000
  expected = [4375, 8750, 6562, ...]
  received = [4375, 8750, 6562, ...]   (5.8 ms)  PASS

x_t> quit
```

---

## Part 5 — What Each Test Proves

> h values shown for **anxiety=512 (KEY1 pressed), B=2000**. Use `--anxiety N` to
> adjust; the test script recomputes h and expected u_t automatically.

| Test case | h (anxiety=512) | Expected u_t | What it verifies |
|-----------|---------|--------------|-----------------|
| Zero vector | +2000 | all 0 | Safe-zone branch, no spurious corrections |
| All +1 | +3460 | all 0 | Positive projections stay safe |
| Near barrier (x[0]=−31) | +16 | all 0 | Exact barrier boundary (h→0⁺) |
| Max INT16 (all +32767) | >> 0 | all 0 | No overflow in accumulator |
| All −2 | −920 | [57,115,86,…] | Proportional correction at small violation |
| All −100 | −144000 | [9000,18000,…] | Large steering at deep violation |
| Asymmetric x[0]=−500 | −30000 | [1875,3750,…] | Only W[0]-weighted component active |
| All −1000 | −1438000 | [32767,32767,…] | INT16 saturation clamp working |
| Alternating +10/−20 | −920 | same as all −2 | Mixed-sign inputs |
| Impulse x[7]=−200 | −15600 | [0,..,975,..] | Single-element violation |

### Button-level effect on a fixed violation vector (all x=-30)

```bash
# Run this to walk through all 16 levels interactively:
python3 test_aegis.py /dev/ttyUSB1 --scan
```

| anxiety | B | h (all x=−30) | Safe? | Effect on LLM |
|---------|---|---------------|-------|---------------|
| 0       | 5000 | +1200 | ✓ Yes | No correction |
| 256     | 3500 | −300  | ✗ No  | Small steering |
| 512     | 2000 | −1800 | ✗ No  | Medium steering |
| 768     | 500  | −3300 | ✗ No  | Strong steering |
| 960     | −625 | −4725 | ✗ No  | Maximum steering |

---

## Part 6 — Troubleshooting

### TIMEOUT on all tests
```
Cause: FPGA not responding

Checks:
  1. DONE LED is lit after programming
  2. Both USB cables connected (JTAG + UART separate ports)
  3. Correct port: ls /dev/ttyUSB*  (UART is usually the higher number)
  4. Port permission: sudo chmod a+rw /dev/ttyUSBX
  5. Baud rate in uart_rx.v: CLKS_PER_BIT = 200_000_000 / 115_200 = 1736
```

### TIMEOUT only on receive (TX LED blinks, no RX)
```
Cause: FPGA receives bytes but aegis_shield does not send u_t back.

Checks:
  • uart_rx needs exactly 32 bytes (16 words × 2 bytes) to trigger aegis_shield.
    If your terminal echoed extra bytes, the framing is off.
  • Re-program the bitstream and retry fresh.
  • Verify aegis_shield u_t AXI-Stream connects to uart_tx in aegis_top.v.
```

### Wrong u_t values (not all-zero mismatches)
```
Cause: Constants mismatch between FPGA and Python script.

Checks:
  • anxiety_level: rtl/aegis_top.v line with 'assign anxiety_level'
    must equal ANXIETY = 512 in test_aegis.py
  • W[] array: hls/aegis_shield/aegis_shield.h must match W in test_aegis.py
  • If you changed any HLS constants, re-run:
      vitis-run --mode hls --tcl scripts/build_all_hls.tcl
    then re-synthesise and re-program.
```

### DONE LED does not light after programming
```
Checks:
  • Board powered? (PWR LED red)
  • Correct device? (get_hw_devices in TCL console)
  • Bitstream for right part? grep "Device" impl/utilization_impl.rpt
    must show xc7a100tifgg484-1L
  • Try power-cycling the board and re-programming.
```

### Serial port not found (`/dev/ttyUSB*` missing)
```bash
# Check if kernel recognised the CP2102:
dmesg | grep -i "cp210\|ttyUSB\|serial" | tail -10

# Install driver if missing (Ubuntu):
sudo apt install linux-modules-extra-$(uname -r)
```

---

## Part 7 — Understanding the Results

The CC-CBF math running on the FPGA:

```
W       = [64, 128, 96, 112, 80, 72, 104, 88, 120, 60, 92, 116, 76, 84, 100, 68]
B       = B_BASE − ((ALPHA × anxiety_level) >> 10)
        = 5000   − ((6000  × 512)           >> 10)  =  2000

h       = B + W · x_t               ← barrier function value

if h ≥ 0:  u_t = [0, 0, …, 0]      ← LLM trajectory is SAFE, no correction
if h < 0:  u_t[i] = clamp₁₆(−h × W[i] >> 10)   ← steer back into safe set
```

**What this means for the full system:**

- `x_t` is a 16-dimensional projection of the LLM's hidden state
- `h` measures how far the LLM is inside the safe semantic region
- `u_t` is the additive correction that forces the LLM back to safety
- `anxiety_level` shrinks the safe region as physiological arousal increases
  (more anxious teenager → smaller allowed region → earlier intervention)

Once your RF inference module replaces `assign anxiety_level = 16'd512`,
the boundary will adapt in real-time to the user's biosignals.

---

## Quick Reference

```bash
# Program bitstream
vivado -mode batch -source /tmp/program.tcl

# Find serial port
ls /dev/ttyUSB*

# Run all 10 tests — no buttons pressed (anxiety=0, calm)
python3 test_aegis.py /dev/ttyUSB1 --anxiety 0

# Run all 10 tests — KEY1 pressed (anxiety=512, medium stress)
python3 test_aegis.py /dev/ttyUSB1 --anxiety 512

# Using --buttons flag (KEY1+KEY2 = 1100 binary = level 12 = 768)
python3 test_aegis.py /dev/ttyUSB1 --buttons 1,1,0,0

# Scan all 16 anxiety levels interactively
python3 test_aegis.py /dev/ttyUSB1 --scan

# UART loopback (wire TX→RX, no FPGA logic involved)
python3 test_aegis.py /dev/ttyUSB1 --loopback

# Single vector at anxiety=960 (all 4 buttons pressed)
python3 test_aegis.py /dev/ttyUSB1 --anxiety 960 \
    --single "-100,-100,-100,-100,-100,-100,-100,-100,-100,-100,-100,-100,-100,-100,-100,-100"

# Interactive exploration at anxiety=512
python3 test_aegis.py /dev/ttyUSB1 --anxiety 512 --interactive
```

### Button → anxiety_level reference card

```
KEY1(×512) KEY2(×256) KEY3(×128) KEY4(×64) │ anxiety_level
─────────────────────────────────────────────┼──────────────
  ○            ○            ○            ○   │      0
  ○            ○            ○            ●   │     64
  ○            ○            ●            ○   │    128
  ○            ○            ●            ●   │    192
  ○            ●            ○            ○   │    256
  ○            ●            ○            ●   │    320
  ○            ●            ●            ○   │    384
  ○            ●            ●            ●   │    448
  ●            ○            ○            ○   │    512   ← KEY1 alone
  ●            ○            ○            ●   │    576
  ●            ○            ●            ○   │    640
  ●            ○            ●            ●   │    704
  ●            ●            ○            ○   │    768
  ●            ●            ○            ●   │    832
  ●            ●            ●            ○   │    896
  ●            ●            ●            ●   │    960   ← all 4
(● = pressed, ○ = released, LEDs mirror the state)
```
