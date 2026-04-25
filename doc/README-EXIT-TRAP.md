# Exit Trap Device

The `ExitTrap` is a minimal virtual device used to signal a "program complete" or "halt" state to the simulator. It is specifically designed for automated test harnesses where a simulated program needs to inform the host that it has finished execution.

---

## 1. Operation

The device monitors a single memory address. When the magic value `#$42` is written to this address, the device asserts a halt request to the CPU via the `IBus` interface.

---

## 2. Configuration

Mapped at `$D6CF` by default in the MEGA65 test machines.

```json
{ "type": "exit_trap", "name": "ExitTrap", "baseAddr": "0xD6CF" }
```

---

## 3. Interaction

From 6502/45GS02 assembly:

```assembly
; Program logic here...

lda #$42
sta $D6CF  ; Signal emulator to quit
```
