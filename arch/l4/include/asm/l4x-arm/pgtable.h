#pragma once

unsigned long l4x_set_pte(struct mm_struct *mm,
                          unsigned long addr,
                          pte_t old, pte_t pteval);

void l4x_pte_clear(struct mm_struct *mm, unsigned long addr, pte_t pteval);
