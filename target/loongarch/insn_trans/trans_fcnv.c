/*
 * LoongArch translate functions
 *
 * Copyright (c) 2021 Loongson Technology Corporation Limited
 *
 * SPDX-License-Identifier: LGPL-2.1+
 */

TRANS(fcvt_s_d, gen_f2, gen_helper_fcvt_s_d)
TRANS(fcvt_d_s, gen_f2, gen_helper_fcvt_d_s)
TRANS(ftintrm_w_s, gen_f2, gen_helper_ftintrm_w_s)
TRANS(ftintrm_w_d, gen_f2, gen_helper_ftintrm_w_d)
TRANS(ftintrm_l_s, gen_f2, gen_helper_ftintrm_l_s)
TRANS(ftintrm_l_d, gen_f2, gen_helper_ftintrm_l_d)
TRANS(ftintrp_w_s, gen_f2, gen_helper_ftintrp_w_s)
TRANS(ftintrp_w_d, gen_f2, gen_helper_ftintrp_w_d)
TRANS(ftintrp_l_s, gen_f2, gen_helper_ftintrp_l_s)
TRANS(ftintrp_l_d, gen_f2, gen_helper_ftintrp_l_d)
TRANS(ftintrz_w_s, gen_f2, gen_helper_ftintrz_w_s)
TRANS(ftintrz_w_d, gen_f2, gen_helper_ftintrz_w_d)
TRANS(ftintrz_l_s, gen_f2, gen_helper_ftintrz_l_s)
TRANS(ftintrz_l_d, gen_f2, gen_helper_ftintrz_l_d)
TRANS(ftintrne_w_s, gen_f2, gen_helper_ftintrne_w_s)
TRANS(ftintrne_w_d, gen_f2, gen_helper_ftintrne_w_d)
TRANS(ftintrne_l_s, gen_f2, gen_helper_ftintrne_l_s)
TRANS(ftintrne_l_d, gen_f2, gen_helper_ftintrne_l_d)
TRANS(ftint_w_s, gen_f2, gen_helper_ftint_w_s)
TRANS(ftint_w_d, gen_f2, gen_helper_ftint_w_d)
TRANS(ftint_l_s, gen_f2, gen_helper_ftint_l_s)
TRANS(ftint_l_d, gen_f2, gen_helper_ftint_l_d)
TRANS(ffint_s_w, gen_f2, gen_helper_ffint_s_w)
TRANS(ffint_s_l, gen_f2, gen_helper_ffint_s_l)
TRANS(ffint_d_w, gen_f2, gen_helper_ffint_d_w)
TRANS(ffint_d_l, gen_f2, gen_helper_ffint_d_l)
TRANS(frint_s, gen_f2, gen_helper_frint_s)
TRANS(frint_d, gen_f2, gen_helper_frint_d)
