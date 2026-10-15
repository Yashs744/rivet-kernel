#pragma once

#include "kernel/trap.h"

void handle_syscall(struct trap_frame *trap_frame);
