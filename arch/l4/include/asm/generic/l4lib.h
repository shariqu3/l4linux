#ifndef __INCLUDE__ASM_L4__GENERIC__L4LIB_H__
#define __INCLUDE__ASM_L4__GENERIC__L4LIB_H__

#include <linux/stringify.h>

#ifndef MODULE
#ifdef CONFIG_ARM

//#define L4X_ASSUME_READONLY_TEXT

#ifdef L4X_ASSUME_READONLY_TEXT
#define L4XROWC(ro, rw) ro
#else
#define L4XROWC(ro, rw) rw
#endif

#define L4_EXTERNAL_FUNC(func) \
	asm(".section \".data..l4externals.str\"                        \n" \
	    "9: .string \"" __stringify(func) "\"                       \n" \
	    ".previous                                                  \n" \
	    \
	    "7: .long 9b                                                \n" \
	    \
	    L4XROWC(".section \".data..l4externals.jmptbl\"", "")      "\n" \
	    ".p2align 2\n" \
	    "8: .long " __stringify(func##_resolver) "                  \n" \
	    L4XROWC(".previous\n", "")                                 "\n" \
	    L4XROWC("6: .long 8b", "")                                 "\n" \
	    \
	    ".globl " __stringify(func) "                               \n" \
	    ".weak " __stringify(func) "                                \n" \
	    ".type " __stringify(func) ", #function                     \n" \
	    __stringify(func) ":    " L4XROWC("ldr ip, 6b", "")  "      \n" \
	    "                       ldr pc, " L4XROWC("[ip]", "8b") "   \n" \
	    ".size " __stringify(func) ", .-" __stringify(func)        "\n" \
	    ".type " __stringify(func##_resolver) ", #function          \n" \
	    __stringify(func##_resolver) ":" L4XROWC("", "adr ip, 8b") "\n" \
	    "                                push {ip}                  \n" \
	    "                                ldr ip, 7b                 \n" \
	    "                                push {ip}                  \n" \
            "                                ldr ip, [pc]               \n" \
	    "                                ldr pc, [ip]               \n" \
	    "                           .long __l4_external_resolver    \n" \
	    ".size " __stringify(func##_resolver) ", .-" __stringify(func##_resolver) "\n" \
	   )

#define L4_EXTERNAL_FUNC_VARGS(func) L4_EXTERNAL_FUNC(func)

#elif defined(CONFIG_ARM64)

#define L4_EXTERNAL_FUNC(func) \
	asm(".section \".data..l4externals.str\"                        \n" \
	    "9: .string \"" __stringify(func) "\"                       \n" \
	    ".previous                                                  \n" \
	    \
	    "7: .quad 9b                                                \n" \
	    \
	    ".section \".data..l4externals.jmptbl\""                   "\n" \
	    ".p2align 3                                                 \n" \
	    ".globl " __stringify(func##_jmptbl) "                      \n" \
	    __stringify(func##_jmptbl) ":                               \n" \
	    "8: .quad " __stringify(func##_resolver) "                  \n" \
	    ".previous                                                  \n" \
	    "2: .quad 8b"                                              "\n" \
	    \
	    ".globl " __stringify(func) "                               \n" \
	    ".weak " __stringify(func) "                                \n" \
	    ".type " __stringify(func) ", #function                     \n" \
	    __stringify(func) ": adr x9, 2b                             \n" \
	    "                    ldr x9, [x9]                           \n" \
	    "                    ldr x9, [x9]                           \n" \
	    "                    br x9                                  \n" \
	    ".size " __stringify(func) ", .-" __stringify(func)        "\n" \
	    ".type " __stringify(func##_resolver) ", #function          \n" \
	    __stringify(func##_resolver) ": sub sp, sp, #16             \n" \
	    "                               adr x9, 2b                  \n" \
	    "                               ldr x9, [x9]                \n" \
	    "                               str x9, [sp, #0]            \n" \
	    "                               adr x9, 7b                  \n" \
	    "                               ldr x9, [x9]                \n" \
	    "                               str x9, [sp, #8]            \n" \
	    "                               adr x9, 4f                  \n" \
	    "                               ldr x9, [x9]                \n" \
	    "                               ldr x9, [x9]                \n" \
	    "                               br x9                       \n" \
	    "                           4: .quad __l4_external_resolver \n" \
	    ".size " __stringify(func##_resolver) ", .-" __stringify(func##_resolver) "\n" \
	   )

#define L4_EXTERNAL_FUNC_VARGS(func) L4_EXTERNAL_FUNC(func)

#else

#include <asm/nospec-branch.h>

#ifdef CONFIG_X86_32
#define PSTOR ".long"
#define RAX_SETUP ""
#define PUSH_IMM "pushl "
#define L4X_ENDBR ""
#else
#define PSTOR ".quad"
#define RAX_SETUP "xor %eax,%eax; "
#define PUSH_IMM "pushq "
/* The resolver is reached via an indirect jump through the jump table, so
 * it needs an ENDBR for IBT. */
#define L4X_ENDBR "endbr64\n"
#endif
#define L4_EXTERNAL_FUNC_GEN(func, call_prolog) \
	asm(".section \".data..l4externals.str\"                        \n" \
	    "9: .string \"" __stringify(func) "\"                       \n" \
	    ".previous                                                  \n" \
	    \
	    ".section \".data..l4externals.symtab\"                     \n" \
	    "7: "PSTOR" 9b                                              \n" \
	    ".previous                                                  \n" \
	    \
	    ".section \".data..l4externals.jmptbl\"                     \n" \
	    "8: "PSTOR" " __stringify(func##_resolver) "                \n" \
	    ".previous                                                  \n" \
	    \
	    ".section \"" __stringify(.text.l4externals.fu##nc) "\"     \n" \
	    ".globl " __stringify(func) "                               \n" \
	    ".weak " __stringify(func) "                                \n" \
	    ".type " __stringify(func) ", @function                     \n" \
	    __stringify(func) ":" L4X_ENDBR call_prolog                     \
	                          ANNOTATE_RETPOLINE_SAFE              "\n" \
	                          "jmp *8b \n"                              \
	    ".size " __stringify(func) ", .-" __stringify(func)        "\n" \
	    ".type " __stringify(func##_resolver) ", @function          \n" \
	    __stringify(func##_resolver) ": " L4X_ENDBR                   \
	    "                               " PUSH_IMM "$8b             \n" \
            "                               " PUSH_IMM "$7b             \n" \
	                                    ANNOTATE_RETPOLINE_SAFE         \
	    "                               jmp *__l4_external_resolver \n" \
	    ".size " __stringify(func##_resolver) ", .-" __stringify(func##_resolver) "\n" \
	    /* The resolver modifies the stack before the tail call. */      \
	    ".pushsection .discard.func_stack_frame_non_standard, \"aw\"\n" \
	    PSTOR " " __stringify(func##_resolver) "                    \n" \
	    ".popsection                                                \n" \
	    ".previous                                                  \n" \
	   )

#define L4_EXTERNAL_FUNC(func)       L4_EXTERNAL_FUNC_GEN(func, "")
#define L4_EXTERNAL_FUNC_VARGS(func) L4_EXTERNAL_FUNC_GEN(func, RAX_SETUP)

#endif
#endif /* MODULE */

#define L4_EXTERNAL_FUNC_AND_EXPORT(func) \
		L4_EXTERNAL_FUNC(func); \
		EXPORT_SYMBOL(func)

#endif /* __INCLUDE__ASM_L4__GENERIC__L4LIB_H__ */
