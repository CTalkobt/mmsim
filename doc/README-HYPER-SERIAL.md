# HyperSerial Device

`HyperSerial` is a virtual debugging device used to log character output from a running simulation to a host-side file. It is primarily used for unit tests and verifying CPU execution flow on the 45GS02 (MEGA65).

---

## 1. Register Interface

The device is typically mapped at `\$D6C0` in the MEGA65 memory map.

| Address | Mode | Description |
|---------|------|-------------|
| **\$D6C0** | Read | **Status**: Returns `\$03` (Tx and Rx Ready). |
| **\$D6C1** | Write | **Data**: Writing a byte here appends it to `hyper_serial.log`. |
| **\$D6CF** | Write | **Exit Trigger**: Writing `\$42` signals a halt to the simulator. |

---

## 2. Usage

To print a string from assembly:

```assembly
print_loop:
    lda string,x
    beq done
    sta $D6C1    ; Output to HyperSerial
    inx
    jmp print_loop
string: .text "Hello from 45GS02" .byte 0
done:
    lda #$42
    sta $D6CF    ; Trigger Exit
```

---

## 3. Configuration

In the machine JSON, the device is registered with the type `hyper_serial`.

```json
{ "type": "hyper_serial", "name": "HyperSerial", "baseAddr": "0xD6C0" }
```

The output file defaults to `hyper_serial.log` in the current working directory.
