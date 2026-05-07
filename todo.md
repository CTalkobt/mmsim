# Completed Items

## UI / Debugging Features
[x] Pop-up Modal Calculator
  [x] - Base conversion
  [x] - Basic Calculator Operations
    ( + - * / ^(power) square root, x^2, nth root, x^n, 
      sin, cos, sec, csc, tan, atn
      arc sin, arc cos.... 
      negate, ... 
    )
  [x] - 1's complement, 2's compliment
  [x] - Float / Integer mode. 
  [x] - Memory mode (8 results) 
  [x] - Ans (aka Rcl 0) button - Recall prior result. 
  [x] - Rcl n button to recall nth prior result. 
  [x] - Set precision display for int or floating point.

## Toolchain Migration
[x] Migrate 45GS02 test suite from KickAssembler (.asm) to ca45 (.s format)
  [x] - Remove legacy KickAssembler .asm files
  [x] - Convert test programs to .s format
  [x] - Update Makefile test targets
  [x] - Update validate.py for ca45 assembler
  [x] - Update documentation with ca45 project link
