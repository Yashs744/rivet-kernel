# Register names and roles
The CPU has 32 integer registers numbered x0–x31. Names like ra, a0 and s1 are ABI names,
nicknames from the calling convention:

| Register  | ABI Name  | Role                      | Preserved Across Calls? | Saver  |
| --------- | --------- | ------------------------- | ----------------------- | ------ |
| `x0`      | `zero`    | Always `0`                | N/A                     | -      |
| `x1`      | `ra`      | Return address            | No                      | Caller |
| `x2`      | `sp`      | Stack pointer             | Yes                     | Callee |
| `x3`      | `gp`      | Global pointer            | N/A                     | -      |
| `x4`      | `tp`      | Thread pointer            | N/A                     | -      |
| `x5–x7`   | `t0–t2`   | Temporaries               | No                      | Caller |
| `x8`      | `s0 / fp` | Saved / frame pointer     | Yes                     | Callee |
| `x9`      | `s1`      | Saved                     | Yes                     | Callee |
| `x10–x17` | `a0–a7`   | Arguments / return values | No                      | Caller |
| `x18–x27` | `s2–s11`  | Saved                     | Yes                     | Callee |
| `x28–x31` | `t3–t6`   | Temporaries               | No                      | Caller |

### What are callee-saved registers ?
Callee-saved registers are registers that a called function must restore before returning.

Common Examples Across Architectures
- x86-64: RBX, EBP, ESP, R12-R15
- RISC-V: sp, s0-s11, tp, gp

### What are caller-saved registers ?
Caller-saved registers are registers that a function (the “caller”) must save and restore 
itself if it wants their values preserved across a function call.

Common Examples Across Architectures
- x86-64: RAX, RCX, RDX, R8, R9, R10, R11
- RISC-V: Temporary registers (t0–t6) and argument registers (a0–a7)