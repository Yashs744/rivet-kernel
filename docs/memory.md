# Memory

### RAM Layout
Memory version 1 in [memory.c](../kernel/memory.c) uses a single bump allocator that never frees pages.<br>

![physical_ram_layout.png](physical_ram_layout.png)
<div style="text-align: center;">Figure 1. Physical RAM Layout with three processes </div><br>



