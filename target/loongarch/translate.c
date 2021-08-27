/*
 * LoongArch emulation for QEMU - main translation routines.
 *
 * Copyright (c) 2021 Loongson Technology Corporation Limited
 *
 * SPDX-License-Identifier: LGPL-2.1+
 */

#include "qemu/osdep.h"
#include "cpu.h"
#include "tcg/tcg-op.h"
#include "exec/translator.h"
#include "exec/helper-proto.h"
#include "exec/helper-gen.h"

#include "exec/translator.h"
#include "exec/log.h"
#include "qemu/qemu-print.h"
#include "translate.h"
#include "internals.h"

/* global register indices */
TCGv cpu_gpr[32], cpu_pc;
static TCGv cpu_lladdr, cpu_llval;
TCGv_i32 cpu_fcsr0;
TCGv_i64 cpu_fpr[32];

#define DISAS_STOP       DISAS_TARGET_0

void generate_exception(DisasContext *ctx, int excp)
{
    tcg_gen_movi_tl(cpu_pc, ctx->base.pc_next);
    gen_helper_raise_exception(cpu_env, tcg_constant_i32(excp));
    ctx->base.is_jmp = DISAS_NORETURN;
}

static inline void gen_goto_tb(DisasContext *ctx, int n, target_ulong dest)
{
    if (translator_use_goto_tb(&ctx->base, dest)) {
        tcg_gen_goto_tb(n);
        tcg_gen_movi_tl(cpu_pc, dest);
        tcg_gen_exit_tb(ctx->base.tb, n);
    } else {
        tcg_gen_movi_tl(cpu_pc, dest);
        tcg_gen_lookup_and_goto_ptr();
    }
}

static void loongarch_tr_init_disas_context(DisasContextBase *dcbase,
                                            CPUState *cs)
{
    DisasContext *ctx = container_of(dcbase, DisasContext, base);
    int64_t bound;

    ctx->page_start = ctx->base.pc_first & TARGET_PAGE_MASK;
    ctx->mem_idx = MMU_USER_IDX ;

    /* Bound the number of insns to execute to those left on the page.  */
    bound = -(ctx->base.pc_first | TARGET_PAGE_MASK) / 4;
    ctx->base.max_insns = MIN(ctx->base.max_insns, bound);

    ctx->dst_ext = EXT_NONE;
    ctx->ntemp = 0;
    memset(ctx->temp, 0, sizeof(ctx->temp));

    ctx->zero = tcg_constant_tl(0);
}

static void loongarch_tr_tb_start(DisasContextBase *dcbase, CPUState *cs)
{
}

static void loongarch_tr_insn_start(DisasContextBase *dcbase, CPUState *cs)
{
    DisasContext *ctx = container_of(dcbase, DisasContext, base);

    tcg_gen_insn_start(ctx->base.pc_next);
}

/*
 * Wrappers for getting reg values.
 *
 * The $zero register does not have cpu_gpr[0] allocated -- we supply the
 * constant zero as a source, and an uninitialized sink as destination.
 *
 * Further, we may provide an extension for word operations.
 */
static TCGv temp_new(DisasContext *ctx)
{
    assert(ctx->ntemp < ARRAY_SIZE(ctx->temp));
    return ctx->temp[ctx->ntemp++] = tcg_temp_new();
}

static TCGv gpr_src(DisasContext *ctx, int reg_num, DisasExtend ext)
{
    TCGv t;

    if (reg_num == 0) {
        return ctx->zero;
    }

    switch (ext) {
    case EXT_NONE:
        return cpu_gpr[reg_num];
    case EXT_SIGN:
        t = temp_new(ctx);
        tcg_gen_ext32s_tl(t, cpu_gpr[reg_num]);
        return t;
    case EXT_ZERO:
        t = temp_new(ctx);
        tcg_gen_ext32u_tl(t, cpu_gpr[reg_num]);
        return t;
    }
    g_assert_not_reached();
}

static TCGv gpr_dst(DisasContext *ctx, int reg_num)
{
    if (reg_num == 0 || ctx->dst_ext) {
        return temp_new(ctx);
    }
    return cpu_gpr[reg_num];
}

static void gen_set_gpr(DisasContext *ctx, int reg_num, TCGv t)
{
    if (reg_num != 0) {
        switch (ctx->dst_ext) {
        case EXT_NONE:
            tcg_gen_mov_tl(cpu_gpr[reg_num], t);
            break;
        case EXT_SIGN:
            tcg_gen_ext32s_tl(cpu_gpr[reg_num], t);
            break;
        case EXT_ZERO:
            tcg_gen_ext32u_tl(cpu_gpr[reg_num], t);
            break;
        default:
            g_assert_not_reached();
        }
    }
}

#include "decode-insns.c.inc"

/* fmt rd rj rk */
static bool gen_r3(DisasContext *ctx, arg_fmt_rdrjrk *a,
                   DisasExtend src1_ext, DisasExtend src2_ext,
                   DisasExtend dst_ext, void (*func)(TCGv, TCGv, TCGv))
{
    ctx->dst_ext = dst_ext;
    TCGv dest = gpr_dst(ctx, a->rd);
    TCGv src1 = gpr_src(ctx, a->rj, src1_ext);
    TCGv src2 = gpr_src(ctx, a->rk, src2_ext);

    func(dest, src1, src2);

    if (ctx->dst_ext) {
        gen_set_gpr(ctx, a->rd, dest);
    }
    return true;
}

#include "insn_trans/trans_arith.c"
#include "insn_trans/trans_shift.c"
#include "insn_trans/trans_bit.c"

static void loongarch_tr_translate_insn(DisasContextBase *dcbase, CPUState *cs)
{
    CPULoongArchState *env = cs->env_ptr;
    DisasContext *ctx = container_of(dcbase, DisasContext, base);

    ctx->opcode = cpu_ldl_code(env, ctx->base.pc_next);

    if (!decode(ctx, ctx->opcode)) {
        qemu_log_mask(LOG_UNIMP, "Error: unkown opcode. 0x%lx: 0x%x\n",
                      ctx->base.pc_next, ctx->opcode);
        generate_exception(ctx, EXCP_INE);
    }

    ctx->dst_ext = EXT_NONE;

    for (int i = ctx->ntemp - 1; i >= 0; --i) {
        tcg_temp_free(ctx->temp[i]);
        ctx->temp[i] = NULL;
    }
    ctx->ntemp = 0;

    ctx->base.pc_next += 4;
}

static void loongarch_tr_tb_stop(DisasContextBase *dcbase, CPUState *cs)
{
    DisasContext *ctx = container_of(dcbase, DisasContext, base);

    switch (ctx->base.is_jmp) {
    case DISAS_STOP:
        tcg_gen_movi_tl(cpu_pc, ctx->base.pc_next);
        tcg_gen_lookup_and_goto_ptr();
        break;
    case DISAS_TOO_MANY:
        gen_goto_tb(ctx, 0, ctx->base.pc_next);
        break;
    case DISAS_NORETURN:
        break;
    default:
        g_assert_not_reached();
    }
}

static void loongarch_tr_disas_log(const DisasContextBase *dcbase, CPUState *cs)
{
    qemu_log("IN: %s\n", lookup_symbol(dcbase->pc_first));
    log_target_disas(cs, dcbase->pc_first, dcbase->tb->size);
}

static const TranslatorOps loongarch_tr_ops = {
    .init_disas_context = loongarch_tr_init_disas_context,
    .tb_start           = loongarch_tr_tb_start,
    .insn_start         = loongarch_tr_insn_start,
    .translate_insn     = loongarch_tr_translate_insn,
    .tb_stop            = loongarch_tr_tb_stop,
    .disas_log          = loongarch_tr_disas_log,
};

void gen_intermediate_code(CPUState *cs, TranslationBlock *tb, int max_insns)
{
    DisasContext ctx;

    translator_loop(&loongarch_tr_ops, &ctx.base, cs, tb, max_insns);
}

void loongarch_translate_init(void)
{
    int i;

    cpu_gpr[0] = NULL;
    for (i = 1; i < 32; i++) {
        cpu_gpr[i] = tcg_global_mem_new(cpu_env,
                                        offsetof(CPULoongArchState, gpr[i]),
                                        regnames[i]);
    }

    for (i = 0; i < 32; i++) {
        int off = offsetof(CPULoongArchState, fpr[i]);
        cpu_fpr[i] = tcg_global_mem_new_i64(cpu_env, off, fregnames[i]);
    }

    cpu_pc = tcg_global_mem_new(cpu_env, offsetof(CPULoongArchState, pc), "pc");
    cpu_fcsr0 = tcg_global_mem_new_i32(cpu_env,
                    offsetof(CPULoongArchState, fcsr0), "fcsr0");
    cpu_lladdr = tcg_global_mem_new(cpu_env,
                    offsetof(CPULoongArchState, lladdr), "lladdr");
    cpu_llval = tcg_global_mem_new(cpu_env,
                    offsetof(CPULoongArchState, llval), "llval");
}

void restore_state_to_opc(CPULoongArchState *env, TranslationBlock *tb,
                          target_ulong *data)
{
    env->pc = data[0];
}
