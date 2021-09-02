/*
 * LoongArch translate functions
 *
 * Copyright (c) 2021 Loongson Technology Corporation Limited
 *
 * SPDX-License-Identifier: LGPL-2.1+
 */

static bool gen_ll(DisasContext *ctx, arg_fmt_rdrjsi14 *a,
                   void (*func)(TCGv, TCGv, int))
{
    TCGv dest = gpr_dst(ctx, a->rd);
    TCGv src1 = gpr_src(ctx, a->rj, EXT_NONE);
    TCGv t0 = tcg_temp_new();

    tcg_gen_addi_tl(t0, src1, a->si14 << 2);
    func(dest, t0, ctx->mem_idx);
    tcg_gen_st_tl(t0, cpu_env, offsetof(CPULoongArchState, lladdr));
    tcg_gen_st_tl(dest, cpu_env, offsetof(CPULoongArchState, llval));
    tcg_temp_free(t0);
    return true;
}

static bool gen_sc(DisasContext *ctx, arg_fmt_rdrjsi14 *a, MemOp mop)
{
    TCGv dest = gpr_dst(ctx, a->rd);
    TCGv src1 = gpr_src(ctx, a->rj, EXT_NONE);
    TCGv src2 = gpr_src(ctx, a->rd, EXT_NONE);
    TCGv t0 = tcg_temp_new();

    TCGLabel *l1 = gen_new_label();
    TCGLabel *done = gen_new_label();

    tcg_gen_addi_tl(t0, src1, a->si14 << 2);
    tcg_gen_brcond_tl(TCG_COND_EQ, t0, cpu_lladdr, l1);
    tcg_gen_movi_tl(dest, 0);
    tcg_gen_br(done);

    gen_set_label(l1);
    /* generate cmpxchg */
    tcg_gen_atomic_cmpxchg_tl(t0, cpu_lladdr, cpu_llval,
                              src2, ctx->mem_idx, mop);
    tcg_gen_setcond_tl(TCG_COND_EQ, dest, t0, cpu_llval);
    gen_set_label(done);
    tcg_temp_free(t0);
    return true;
}

static bool gen_am(DisasContext *ctx, arg_fmt_rdrjrk *a, DisasExtend ext,
                   void (*func)(TCGv, TCGv, TCGv, TCGArg, MemOp),
                   MemOp mop)
{
    ctx->dst_ext = ext;
    TCGv dest = gpr_dst(ctx, a->rd);
    TCGv addr = gpr_src(ctx, a->rj, EXT_NONE);
    TCGv val = gpr_src(ctx, a->rk, EXT_NONE);

    if ((a->rd != 0) && ((a->rj == a->rd) || (a->rk == a->rd))) {
        qemu_log("%s: waring, register equal\n", __func__);
        return false;
    }

    func(dest, addr, val, ctx->mem_idx, mop);

    if (ctx->dst_ext) {
        gen_set_gpr(ctx, a->rd, dest);
    }
    return true;
}

static bool gen_am_db(DisasContext *ctx, arg_fmt_rdrjrk *a, DisasExtend ext,
                      void (*func)(TCGv, TCGv, TCGv, TCGArg, MemOp),
                      MemOp mop)
{
    ctx->dst_ext = ext;
    TCGv dest = gpr_dst(ctx, a->rd);
    TCGv addr = gpr_src(ctx, a->rj, EXT_NONE);
    TCGv val = gpr_src(ctx, a->rk, EXT_NONE);

    if ((a->rd != 0) && ((a->rj == a->rd) || (a->rk == a->rd))) {
        qemu_log("%s: waring, register equal\n", __func__);
        return false;
    }

    gen_loongarch_sync(0x10);

    func(dest, addr, val, ctx->mem_idx, mop);

    if (ctx->dst_ext) {
        gen_set_gpr(ctx, a->rd, dest);
    }
    return true;
}

TRANS(ll_w, gen_ll, tcg_gen_qemu_ld32s)
TRANS(sc_w, gen_sc, MO_TESL)
TRANS(ll_d, gen_ll, tcg_gen_qemu_ld64)
TRANS(sc_d, gen_sc, MO_TEQ)
TRANS(amswap_w, gen_am, EXT_SIGN, tcg_gen_atomic_xchg_tl, MO_TESL)
TRANS(amswap_d, gen_am, EXT_NONE, tcg_gen_atomic_xchg_tl, MO_TEQ)
TRANS(amadd_w, gen_am, EXT_SIGN, tcg_gen_atomic_fetch_add_tl, MO_TESL)
TRANS(amadd_d, gen_am, EXT_NONE, tcg_gen_atomic_fetch_add_tl, MO_TEQ)
TRANS(amand_w, gen_am, EXT_SIGN, tcg_gen_atomic_fetch_and_tl, MO_TESL)
TRANS(amand_d, gen_am, EXT_NONE, tcg_gen_atomic_fetch_and_tl, MO_TEQ)
TRANS(amor_w, gen_am, EXT_SIGN, tcg_gen_atomic_fetch_or_tl, MO_TESL)
TRANS(amor_d, gen_am, EXT_NONE, tcg_gen_atomic_fetch_or_tl, MO_TEQ)
TRANS(amxor_w, gen_am, EXT_SIGN, tcg_gen_atomic_fetch_xor_tl, MO_TESL)
TRANS(amxor_d, gen_am, EXT_NONE, tcg_gen_atomic_fetch_xor_tl, MO_TEQ)
TRANS(ammax_w, gen_am, EXT_SIGN, tcg_gen_atomic_fetch_smax_tl, MO_TESL)
TRANS(ammax_d, gen_am, EXT_NONE, tcg_gen_atomic_fetch_smax_tl, MO_TEQ)
TRANS(ammin_w, gen_am, EXT_SIGN, tcg_gen_atomic_fetch_smin_tl, MO_TESL)
TRANS(ammin_d, gen_am, EXT_NONE, tcg_gen_atomic_fetch_smin_tl, MO_TEQ)
TRANS(ammax_wu, gen_am, EXT_SIGN, tcg_gen_atomic_fetch_umax_tl, MO_TESL)
TRANS(ammax_du, gen_am, EXT_NONE, tcg_gen_atomic_fetch_umax_tl, MO_TEQ)
TRANS(ammin_wu, gen_am, EXT_SIGN, tcg_gen_atomic_fetch_umin_tl, MO_TESL)
TRANS(ammin_du, gen_am, EXT_NONE, tcg_gen_atomic_fetch_umin_tl, MO_TEQ)
TRANS(amswap_db_w, gen_am_db, EXT_SIGN, tcg_gen_atomic_xchg_tl, MO_TESL)
TRANS(amswap_db_d, gen_am_db, EXT_NONE, tcg_gen_atomic_xchg_tl, MO_TEQ)
TRANS(amadd_db_w, gen_am_db, EXT_SIGN, tcg_gen_atomic_fetch_add_tl, MO_TESL)
TRANS(amadd_db_d, gen_am_db, EXT_NONE, tcg_gen_atomic_fetch_add_tl, MO_TEQ)
TRANS(amand_db_w, gen_am_db, EXT_SIGN, tcg_gen_atomic_fetch_and_tl, MO_TESL)
TRANS(amand_db_d, gen_am_db, EXT_NONE, tcg_gen_atomic_fetch_and_tl, MO_TEQ)
TRANS(amor_db_w, gen_am_db, EXT_SIGN, tcg_gen_atomic_fetch_or_tl, MO_TESL)
TRANS(amor_db_d, gen_am_db, EXT_NONE, tcg_gen_atomic_fetch_or_tl, MO_TEQ)
TRANS(amxor_db_w, gen_am_db, EXT_SIGN, tcg_gen_atomic_fetch_xor_tl, MO_TESL)
TRANS(amxor_db_d, gen_am_db, EXT_NONE, tcg_gen_atomic_fetch_xor_tl, MO_TEQ)
TRANS(ammax_db_w, gen_am_db, EXT_SIGN, tcg_gen_atomic_fetch_smax_tl, MO_TESL)
TRANS(ammax_db_d, gen_am_db, EXT_NONE, tcg_gen_atomic_fetch_smax_tl, MO_TEQ)
TRANS(ammin_db_w, gen_am_db, EXT_SIGN, tcg_gen_atomic_fetch_smin_tl, MO_TESL)
TRANS(ammin_db_d, gen_am_db, EXT_NONE, tcg_gen_atomic_fetch_smin_tl, MO_TEQ)
TRANS(ammax_db_wu, gen_am_db, EXT_SIGN, tcg_gen_atomic_fetch_umax_tl, MO_TESL)
TRANS(ammax_db_du, gen_am_db, EXT_NONE, tcg_gen_atomic_fetch_umax_tl, MO_TEQ)
TRANS(ammin_db_wu, gen_am_db, EXT_SIGN, tcg_gen_atomic_fetch_umin_tl, MO_TESL)
TRANS(ammin_db_du, gen_am_db, EXT_NONE, tcg_gen_atomic_fetch_umin_tl, MO_TEQ)
