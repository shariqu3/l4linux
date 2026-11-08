#pragma once

#include <l4/sys/utcb.h>

void l4x_setup_l4_aux_thread_utcb(l4_cap_idx_t thread, l4_utcb_t *target_utcb, l4_utcb_t *calling_utcb);
