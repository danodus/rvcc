/* rvcc C compiler - ARMv7 ISA encoder */

/* ARMv7 opcodes */
typedef enum {
	ar_and = 0,
	ar_eor = 1,
	ar_sub = 2,
	ar_rsb = 3,
	ar_add = 4,
	ar_adc = 5,
	ar_sbc = 6,
	ar_rsc = 7,
	ar_tst = 8,
	ar_teq = 9,
	ar_cmp = 10,
	ar_cmn = 11,
	ar_orr = 12,
	ar_mov = 13,
	ar_bic = 14,
	ar_mvn = 15
} ar_op;

/* ARMv7 conditions */
typedef enum {
	ac_eq = 0,
	ac_ne = 1,
	ac_cs = 2,
	ac_cc = 3,
	ac_mi = 4,
	ac_pl = 5,
	ac_vs = 6,
	ac_vc = 7,
	ac_hi = 8,
	ac_ls = 9,
	ac_ge = 10,
	ac_lt = 11,
	ac_gt = 12,
	ac_le = 13,
	ac_al = 14
} ar_cond;

/* ARMv7 registers */
typedef enum {
	a_r0 = 0,
	a_r1 = 1,
	a_r2 = 2,
	a_r3 = 3,
	a_r4 = 4,
	a_r5 = 5,
	a_r6 = 6,
	a_r7 = 7,
	a_r8 = 8,
	a_r9 = 9,
	a_r10 = 10,
	a_s0 = 11, /* r11 */
	a_r12 = 12,
	a_sp = 13,
	a_lr = 14,
	a_pc = 15
} a_reg;

ar_cond a_get_cond(il_op op)
{
	switch (op) {
	case op_equals:
		return ac_eq;
	case op_not_equals:
		return ac_ne;
	case op_less_than:
		return ac_lt;
	case op_greater_eq_than:
		return ac_ge;
	case op_greater_than:
		return ac_gt;
	case op_less_eq_than:
		return ac_le;
	default:
		error("Unsupported condition IL op");
	}
	return ac_al;
}

int a_extract_bits(int imm, int i_start, int i_end, int d_start, int d_end)
{
	int v;

	if (d_end - d_start != i_end - i_start || i_start > i_end || d_start > d_end)
		error("Invalid bit copy");

	v = imm >> i_start;
	v = v & ((2 << (i_end - i_start)) - 1);
	v = v << d_start;
	return v;
}

int a_encode(ar_cond cond, int opcode, int rn, int rd, int op2)
{
	return (cond << 28) + (opcode << 20) + (rn << 16) + (rd << 12) + op2;
}

int a_swi()
{
	return a_encode(ac_al, 240, 0, 0, 0);
}

int a_mov(ar_cond cond, int io, int opcode, int s, int rn, int rd, int op2)
{
	int shift = 0;
	if (op2 > 255) {
		shift = 16; /* full rotation */
		while ((op2 & 3) == 0) {
			/* we can shift by two bits */
			op2 = op2 >> 2;
			shift -= 1;
		}
		if (op2 > 255)
			error("Unable to represent value"); /* value spans more than 8 bits */
	}
	return a_encode(cond, s + (opcode << 1) + (io << 5), rn, rd, (shift << 8) + (op2 & 255));
}

int a_and_i(ar_cond cond, a_reg rd, a_reg rs, int imm)
{
	return a_mov(cond, 1, ar_and, 0, rs, rd, imm);
}

int a_or_i(ar_cond cond, a_reg rd, a_reg rs, int imm)
{
	return a_mov(cond, 1, ar_orr, 0, rs, rd, imm);
}

int a_and_r(ar_cond cond, a_reg rd, a_reg rs, a_reg rm)
{
	return a_mov(cond, 0, ar_and, 0, rs, rd, rm);
}

int a_or_r(ar_cond cond, a_reg rd, a_reg rs, a_reg rm)
{
	return a_mov(cond, 0, ar_orr, 0, rs, rd, rm);
}

int a_movw(ar_cond cond, a_reg rd, int imm)
{
	return a_encode(cond, 48, 0, rd, 0) + a_extract_bits(imm, 0, 11, 0, 11) + a_extract_bits(imm, 12, 15, 16, 19);
}

int a_movt(ar_cond cond, a_reg rd, int imm)
{
	imm = imm >> 16;
	return a_encode(cond, 52, 0, rd, 0) + a_extract_bits(imm, 0, 11, 0, 11) + a_extract_bits(imm, 12, 15, 16, 19);
}

int a_mov_i(ar_cond cond, a_reg rd, int imm)
{
	return a_mov(cond, 1, ar_mov, 0, 0, rd, imm);
}

int a_mov_r(ar_cond cond, a_reg rd, a_reg rs)
{
	return a_mov(cond, 0, ar_mov, 0, 0, rd, rs);
}

int a_srl(ar_cond cond, a_reg rd, a_reg rm, a_reg rs)
{
	return a_encode(cond, 0 + (ar_mov << 1) + (0 << 5), 0, rd, rm + (1 << 4) + (1 << 5) + (rs << 8));
}

int a_sll(ar_cond cond, a_reg rd, a_reg rm, a_reg rs)
{
	return a_encode(cond, 0 + (ar_mov << 1) + (0 << 5), 0, rd, rm + (1 << 4) + (0 << 5) + (rs << 8));
}

int a_add_i(ar_cond cond, a_reg rd, a_reg rs, int imm)
{
	if (imm >= 0)
		return a_mov(cond, 1, ar_add, 0, rs, rd, imm);
	else
		return a_mov(cond, 1, ar_sub, 0, rs, rd, -imm);
}

int a_sub_i(ar_cond cond, a_reg rd, a_reg rs, int imm)
{
	return a_mov(cond, 1, ar_sub, 0, rs, rd, imm);
}

int a_add_r(ar_cond cond, a_reg rd, a_reg rs, a_reg ro)
{
	return a_mov(cond, 0, ar_add, 0, rs, rd, ro);
}

int a_sub_r(ar_cond cond, a_reg rd, a_reg rs, a_reg ro)
{
	return a_mov(cond, 0, ar_sub, 0, rs, rd, ro);
}

int a_zero(int rd)
{
	return a_mov_i(ac_al, rd, 0);
}

int a_nop()
{
	return a_mov_r(ac_al, a_r8, a_r8);
}

int a_transfer(ar_cond cond, int l, int size, a_reg rn, a_reg rd, int ofs)
{
	int opcode = 64 + 16 + 8 + l;
	if (size == 1)
		opcode += 4;
	if (ofs < 0) {
		opcode -= 8;
		ofs = -ofs;
	}
	return a_encode(cond, opcode, rn, rd, ofs & 4095);
}

int a_lw(ar_cond cond, a_reg rd, a_reg rn, int ofs)
{
	return a_transfer(cond, 1, 4, rn, rd, ofs);
}

int a_lb(ar_cond cond, a_reg rd, a_reg rn, int ofs)
{
	return a_transfer(cond, 1, 1, rn, rd, ofs);
}

int a_sw(ar_cond cond, a_reg rd, a_reg rn, int ofs)
{
	return a_transfer(cond, 0, 4, rn, rd, ofs);
}

int a_sb(ar_cond cond, a_reg rd, a_reg rn, int ofs)
{
	return a_transfer(cond, 0, 1, rn, rd, ofs);
}

int a_bx(ar_cond cond, a_reg rd)
{
	return a_encode(cond, 18, 15, 15, rd + 241);
}

int a_b(ar_cond cond, int ofs)
{
	int o = (ofs - 8) >> 2;
	return a_encode(cond, 160, 0, 0, 0) + (o & 16777215);
}

int a_bl(ar_cond cond, int ofs)
{
	int o = (ofs - 8) >> 2;
	return a_encode(cond, 176, 0, 0, 0) + (o & 16777215);
}

int a_mul(ar_cond cond, a_reg rd, a_reg r1, a_reg r2)
{
	return a_encode(cond, 0, rd, 0, (r1 << 8) + 144 + r2);
}

int a_rsb_i(ar_cond cond, a_reg rd, int imm, a_reg rn)
{
	return a_mov(cond, 1, ar_rsb, 0, rd, rn, imm);
}

int a_cmp_r(ar_cond cond, a_reg rd, a_reg r1, a_reg r2)
{
	return a_mov(cond, 0, ar_cmp, 1, r1, rd, r2);
}

int a_teq(a_reg rd)
{
	return a_mov(ac_al, 1, ar_teq, 1, rd, 0, 0);
}