/*
 * LoongArch translate functions
 *
 * Copyright (c) 2021 Loongson Technology Corporation Limited
 *
 * SPDX-License-Identifier: LGPL-2.1+
 */

static bool trans_break(DisasContext *ctx, arg_break *a)
{
    generate_exception(ctx, EXCP_BREAK);
    return true;
}

static bool trans_syscall(DisasContext *ctx, arg_syscall *a)
{
    generate_exception(ctx, EXCP_SYSCALL);
    return true;
}

static bool trans_asrtle_d(DisasContext *ctx, arg_asrtle_d * a)
{
    TCGv src1 = gpr_src(ctx, a->rj, EXT_NONE);
    TCGv src2 = gpr_src(ctx, a->rk, EXT_NONE);

    gen_helper_asrtle_d(cpu_env, src1, src2);
    return true;
}

static bool trans_asrtgt_d(DisasContext *ctx, arg_asrtgt_d * a)
{
    TCGv src1 = gpr_src(ctx, a->rj, EXT_NONE);
    TCGv src2 = gpr_src(ctx, a->rk, EXT_NONE);

    gen_helper_asrtgt_d(cpu_env, src1, src2);
    return true;
}

static bool trans_rdtimel_w(DisasContext *ctx, arg_rdtimel_w *a)
{
    tcg_gen_movi_tl(cpu_gpr[a->rd], 0);
    return true;
}

static bool trans_rdtimeh_w(DisasContext *ctx, arg_rdtimeh_w *a)
{
    tcg_gen_movi_tl(cpu_gpr[a->rd], 0);
    return true;
}

static bool trans_rdtime_d(DisasContext *ctx, arg_rdtime_d *a)
{
    tcg_gen_movi_tl(cpu_gpr[a->rd], 0);
    return true;
}

static bool trans_cpucfg(DisasContext *ctx, arg_cpucfg *a)
{
    TCGv dest = gpr_dst(ctx, a->rd);
    TCGv src1 = gpr_src(ctx, a->rj, EXT_NONE);

    gen_helper_cpucfg(dest, cpu_env, src1);
    return true;
}

static bool gen_crc(DisasContext *ctx, arg_fmt_rdrjrk *a,
                    void (*func)(TCGv, TCGv, TCGv, TCGv),
                    TCGv tsz)
{
    ctx->dst_ext = EXT_SIGN;
    TCGv dest = gpr_dst(ctx, a->rd);
    TCGv src1 = gpr_src(ctx, a->rj, EXT_NONE);
    TCGv src2 = gpr_src(ctx, a->rk, EXT_NONE);

    func(dest, src2, src1, tsz);

    gen_set_gpr(ctx, a->rd, dest);
    return true;
}

TRANS(crc_w_b_w, gen_crc, gen_helper_crc32, tcg_constant_tl(1))
TRANS(crc_w_h_w, gen_crc, gen_helper_crc32, tcg_constant_tl(2))
TRANS(crc_w_w_w, gen_crc, gen_helper_crc32, tcg_constant_tl(4))
TRANS(crc_w_d_w, gen_crc, gen_helper_crc32, tcg_constant_tl(8))
TRANS(crcc_w_b_w, gen_crc, gen_helper_crc32c, tcg_constant_tl(1))
TRANS(crcc_w_h_w, gen_crc, gen_helper_crc32c, tcg_constant_tl(2))
TRANS(crcc_w_w_w, gen_crc, gen_helper_crc32c, tcg_constant_tl(4))
TRANS(crcc_w_d_w, gen_crc, gen_helper_crc32c, tcg_constant_tl(8))
