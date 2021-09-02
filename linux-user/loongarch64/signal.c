/*
 * LoongArch emulation of Linux signals
 *
 * Copyright (c) 2021 Loongson Technology Corporation Limited
 *
 * SPDX-License-Identifier: LGPL-2.1+
 */

#include "qemu/osdep.h"
#include "qemu.h"
#include "signal-common.h"
#include "linux-user/trace.h"

struct target_sigcontext {
    uint64_t   sc_pc;
    uint64_t   sc_gpr[32];
    uint64_t   sc_fpr[32];
    uint64_t   sc_fcc;
    uint32_t   sc_fcsr;
    uint32_t   sc_flags;
};

struct sigframe {
    uint32_t sf_ass[4];             /* argument save space for o32 */
    uint32_t sf_code[2];            /* signal trampoline */
    struct target_sigcontext sf_sc;
    target_sigset_t sf_mask;
};

struct target_ucontext {
    target_ulong tuc_flags;
    target_ulong tuc_link;
    target_stack_t tuc_stack;
    target_ulong pad0;
    struct target_sigcontext tuc_mcontext;
    target_sigset_t tuc_sigmask;
};

struct target_rt_sigframe {
    uint32_t rs_ass[4];            /* argument save space for o32 */
    uint32_t rs_code[2];           /* signal trampoline */
    struct target_siginfo rs_info;
    struct target_ucontext rs_uc;
};

/* Install trampoline to jump back from signal handler */
static inline int install_sigtramp(unsigned int *tramp, unsigned int syscall)
{
    int err = 0;

    /*
     * Set up the return code ...
     *
     *         li      a7, 139
     *         syscall
     */

    __put_user(0x03822c0b, tramp + 0);  /* ori  a7, a7, 0x8b */
    __put_user(0x002b0000, tramp + 1);  /* syscall 0 */
    return err;
}

static inline void setup_sigcontext(CPULoongArchState *env,
                                    struct target_sigcontext *sc)
{
    int i;

    __put_user(env->pc, &sc->sc_pc);

    __put_user(0, &sc->sc_gpr[0]);
    for (i = 1; i < 32; ++i) {
        __put_user(env->gpr[i], &sc->sc_gpr[i]);
    }

    for (i = 0; i < 32; ++i) {
        __put_user(env->fpr[i], &sc->sc_fpr[i]);
    }
}

static inline void
restore_sigcontext(CPULoongArchState *env, struct target_sigcontext *sc)
{
    int i;

    __get_user(env->pc, &sc->sc_pc);

    for (i = 1; i < 32; ++i) {
        __get_user(env->gpr[i], &sc->sc_gpr[i]);
    }

    for (i = 0; i < 32; ++i) {
        __get_user(env->fpr[i], &sc->sc_fpr[i]);
    }
}

/*
 * Determine which stack to use..
 */
static inline abi_ulong
get_sigframe(struct target_sigaction *ka, CPULoongArchState *env,
             size_t frame_size)
{
    unsigned long sp;

    sp = target_sigsp(get_sp_from_cpustate(env) - 32, ka);

    return (sp - frame_size) & ~7;
}

void setup_rt_frame(int sig, struct target_sigaction *ka,
                    target_siginfo_t *info,
                    target_sigset_t *set, CPULoongArchState *env)
{
    struct target_rt_sigframe *frame;
    abi_ulong frame_addr;
    int i;

    frame_addr = get_sigframe(ka, env, sizeof(*frame));
    trace_user_setup_rt_frame(env, frame_addr);
    if (!lock_user_struct(VERIFY_WRITE, frame, frame_addr, 0)) {
        goto give_sigsegv;
    }

    install_sigtramp(frame->rs_code, TARGET_NR_rt_sigreturn);

    tswap_siginfo(&frame->rs_info, info);

    __put_user(0, &frame->rs_uc.tuc_flags);
    __put_user(0, &frame->rs_uc.tuc_link);
    target_save_altstack(&frame->rs_uc.tuc_stack, env);

    setup_sigcontext(env, &frame->rs_uc.tuc_mcontext);

    for (i = 0; i < TARGET_NSIG_WORDS; i++) {
        __put_user(set->sig[i], &frame->rs_uc.tuc_sigmask.sig[i]);
    }

    env->gpr[4] = sig;
    env->gpr[5] = frame_addr + offsetof(struct target_rt_sigframe, rs_info);
    env->gpr[6] = frame_addr + offsetof(struct target_rt_sigframe, rs_uc);
    env->gpr[3] = frame_addr;
    env->gpr[1] = frame_addr + offsetof(struct target_rt_sigframe, rs_code);

    env->pc = env->gpr[20] = ka->_sa_handler;
    unlock_user_struct(frame, frame_addr, 1);
    return;

give_sigsegv:
    unlock_user_struct(frame, frame_addr, 1);
    force_sigsegv(sig);
}

long do_rt_sigreturn(CPULoongArchState *env)
{
    struct target_rt_sigframe *frame;
    abi_ulong frame_addr;
    sigset_t blocked;

    frame_addr = env->gpr[3];
    trace_user_do_rt_sigreturn(env, frame_addr);
    if (!lock_user_struct(VERIFY_READ, frame, frame_addr, 1)) {
        goto badframe;
    }

    target_to_host_sigset(&blocked, &frame->rs_uc.tuc_sigmask);
    set_sigmask(&blocked);

    restore_sigcontext(env, &frame->rs_uc.tuc_mcontext);
    target_restore_altstack(&frame->rs_uc.tuc_stack, env);

    unlock_user_struct(frame, frame_addr, 0);
    return -TARGET_QEMU_ESIGRETURN;

badframe:
    unlock_user_struct(frame, frame_addr, 0);
    force_sig(TARGET_SIGSEGV);
    return -TARGET_QEMU_ESIGRETURN;
}
