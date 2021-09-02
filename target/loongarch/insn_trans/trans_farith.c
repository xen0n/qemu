/*
 * LoongArch translate functions
 *
 * Copyright (c) 2021 Loongson Technology Corporation Limited
 *
 * SPDX-License-Identifier: LGPL-2.1+
 */

static bool gen_f3(DisasContext *ctx, arg_fmt_fdfjfk *a,
                   void (*func)(TCGv, TCGv_env, TCGv, TCGv))
{
    func(cpu_fpr[a->fd], cpu_env, cpu_fpr[a->fj], cpu_fpr[a->fk]);
    return true;
}

static bool gen_muladd(DisasContext *ctx, arg_fmt_fdfjfkfa *a,
                       void (*func)(TCGv, TCGv_env, TCGv, TCGv, TCGv, TCGv_i32),
                       int flag)
{
    TCGv_i32 tflag = tcg_constant_i32(flag);
    func(cpu_fpr[a->fd], cpu_env, cpu_fpr[a->fj],
         cpu_fpr[a->fk], cpu_fpr[a->fa], tflag);
    return true;
}

static bool trans_fcopysign_s(DisasContext *ctx, arg_fmt_fdfjfk *a)
{
    tcg_gen_deposit_i64(cpu_fpr[a->fd], cpu_fpr[a->fk], cpu_fpr[a->fj], 0, 31);
    return true;
}

static bool trans_fcopysign_d(DisasContext *ctx, arg_fmt_fdfjfk *a)
{
    tcg_gen_deposit_i64(cpu_fpr[a->fd], cpu_fpr[a->fk], cpu_fpr[a->fj], 0, 63);
    return true;
}

TRANS(fadd_s, gen_f3, gen_helper_fadd_s)
TRANS(fadd_d, gen_f3, gen_helper_fadd_d)
TRANS(fsub_s, gen_f3, gen_helper_fsub_s)
TRANS(fsub_d, gen_f3, gen_helper_fsub_d)
TRANS(fmul_s, gen_f3, gen_helper_fmul_s)
TRANS(fmul_d, gen_f3, gen_helper_fmul_d)
TRANS(fdiv_s, gen_f3, gen_helper_fdiv_s)
TRANS(fdiv_d, gen_f3, gen_helper_fdiv_d)
TRANS(fmax_s, gen_f3, gen_helper_fmax_s)
TRANS(fmax_d, gen_f3, gen_helper_fmax_d)
TRANS(fmin_s, gen_f3, gen_helper_fmin_s)
TRANS(fmin_d, gen_f3, gen_helper_fmin_d)
TRANS(fmaxa_s, gen_f3, gen_helper_fmaxa_s)
TRANS(fmaxa_d, gen_f3, gen_helper_fmaxa_d)
TRANS(fmina_s, gen_f3, gen_helper_fmina_s)
TRANS(fmina_d, gen_f3, gen_helper_fmina_d)
TRANS(fscaleb_s, gen_f3, gen_helper_fscaleb_s)
TRANS(fscaleb_d, gen_f3, gen_helper_fscaleb_d)
TRANS(fabs_s, gen_f2, gen_helper_fabs_s)
TRANS(fabs_d, gen_f2, gen_helper_fabs_d)
TRANS(fneg_s, gen_f2, gen_helper_fneg_s)
TRANS(fneg_d, gen_f2, gen_helper_fneg_d)
TRANS(fsqrt_s, gen_f2, gen_helper_fsqrt_s)
TRANS(fsqrt_d, gen_f2, gen_helper_fsqrt_d)
TRANS(frecip_s, gen_f2, gen_helper_frecip_s)
TRANS(frecip_d, gen_f2, gen_helper_frecip_d)
TRANS(frsqrt_s, gen_f2, gen_helper_frsqrt_s)
TRANS(frsqrt_d, gen_f2, gen_helper_frsqrt_d)
TRANS(flogb_s, gen_f2, gen_helper_flogb_s)
TRANS(flogb_d, gen_f2, gen_helper_flogb_d)
TRANS(fclass_s, gen_f2, gen_helper_fclass_s)
TRANS(fclass_d, gen_f2, gen_helper_fclass_d)
TRANS(fmadd_s, gen_muladd, gen_helper_fmuladd_s, 0)
TRANS(fmadd_d, gen_muladd, gen_helper_fmuladd_d, 0)
TRANS(fmsub_s, gen_muladd, gen_helper_fmuladd_s, float_muladd_negate_c)
TRANS(fmsub_d, gen_muladd, gen_helper_fmuladd_d, float_muladd_negate_c)
TRANS(fnmadd_s, gen_muladd, gen_helper_fmuladd_s,
      float_muladd_negate_product | float_muladd_negate_c)
TRANS(fnmadd_d, gen_muladd, gen_helper_fmuladd_d,
      float_muladd_negate_product | float_muladd_negate_c)
TRANS(fnmsub_s, gen_muladd, gen_helper_fmuladd_s, float_muladd_negate_product)
TRANS(fnmsub_d, gen_muladd, gen_helper_fmuladd_d, float_muladd_negate_product)
