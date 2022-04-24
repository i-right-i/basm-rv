# basm-rv

A very simple Risc-V RV64 assembler for building flat binaries, done in a single pass.

---

Just clone repo and simply compile like so.

```bash
gcc -c basm_rv.c
gcc -o basm_rv.o
./basm_rv example.s example.bin
```

---

This is still a WIP.  I still have to validate the machine instuctions are created correctly.  Although I should have this done in a couple of days


