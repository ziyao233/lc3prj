/*
 *	lc3prj - lc3emu
 *	By MIT License.
 *	Copyright (c) 2023 Ziyao. All rights reserved.
 */

#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<ctype.h>
#include<stdint.h>

#define CC_N 4
#define CC_Z 2
#define CC_P 1

#define CC_NS 2
#define CC_ZS 1
#define CC_PS 0

typedef uint16_t Word;
typedef int16_t SWord;
Word mReg[8];
Word mMem[65536];
Word mPC = 0x3000, mCC = CC_Z;

#define inst_op(inst)		((inst) >> 12)
#define inst_dr(inst)		(((inst) >> 9) & 7)
#define inst_sr1(inst)		(((inst) >> 6) & 7)
#define inst_sr2(inst)		(((inst) >> 0) & 7)
#define inst_base(inst)		inst_sr1(inst)
#define inst_imm(inst)		((inst) & (1 << 5))
#define inst_imm5(inst)		(((inst) >> 0) & 0x1f)
#define inst_off6(inst)		(((inst) >> 0) & 0x3f)
#define inst_imm9(inst)		(((inst) >> 0) & 0x01ff)
#define inst_imm11(inst)	(((inst) >> 0) & 0x07ff)
#define inst_brflag(inst)	inst_dr(inst)

static inline Word
setcc(Word result)
{
	SWord t = result;
	mCC = ((t > 0) << CC_PS) |
	      ((t == 0) << CC_ZS) |
	      ((t < 0) << CC_NS);
	return result;
}

enum Instruction_Opcode {
	OP_BR = 0, OP_ADD, OP_LD, OP_ST, OP_JSR, OP_AND, OP_LDR, OP_STR, OP_RTI,
	OP_NOT, OP_LDI, OP_STI, OP_JMP, OP_ILLEGAL, OP_LEA, OP_TRAP
};

#define sext5(i)	((i) & 0x10 ? -(((i) ^ 0x1f) + 1) : (i))
#define sext6(i)	((i) & 0x20 ? -(((i) ^ 0x3f) + 1) : (i))
#define sext9(i)	((i) & 0x0100 ? -(((i) ^ 0x01ff) + 1) : (i))
#define sext11(i)	((i) & 0x0400 ? -(((i) ^ 0x07ff) + 1) : (i))

static inline int
do_instruction(void)
{
	Word inst = mMem[mPC++];
	Word t;
	switch (inst_op(inst)) {
	case OP_BR:
		if (mCC & inst_brflag(inst))
			mPC += sext9(inst_imm9(inst));
		break;
	case OP_ADD:
		mReg[inst_dr(inst)] = setcc(mReg[inst_sr1(inst)] +
					    (inst_imm(inst) ?
					    	sext5(inst_imm5(inst)) :
						mReg[inst_sr2(inst)]));
		break;
	case OP_LD:
		mReg[inst_dr(inst)] = setcc(mMem[mPC + sext9(inst_imm9(inst))]);
		break;
	case OP_ST:
		mMem[mPC + sext9(inst_imm9(inst))] = mReg[inst_dr(inst)];
		break;
	case OP_JSR:
		mReg[7] = mPC;
		mPC = inst & (1 << 11) ? mPC + sext11(inst_imm11(inst)) :
					 mReg[inst_base(inst)];
		break;
	case OP_AND:
		mReg[inst_dr(inst)] = setcc(mReg[inst_sr1(inst)] &
					    (inst_imm(inst) ?
					    	sext5(inst_imm5(inst)) :
						mReg[inst_sr2(inst)]));
		break;
	case OP_LDR:
		mReg[inst_dr(inst)] = setcc(mMem[mReg[inst_base(inst)] +
						 sext6(inst_off6(inst))]);
		break;
	case OP_STR:
		mMem[mReg[inst_base(inst) + sext6(inst_off6(inst))]] =
			mReg[inst_dr(inst)];
		break;
	case OP_RTI:
		break;
	case OP_NOT:
		mReg[inst_dr(inst)] = setcc(~inst_sr1(inst));
		break;
	case OP_LDI:
		mReg[inst_dr(inst)] = setcc(mMem[mMem[mPC + sext9(inst_imm9(inst))]]);
		break;
	case OP_STI:
		mMem[mMem[mPC + sext9(inst_imm9(inst))]] = mReg[inst_dr(inst)];
		break;
	case OP_JMP:
		mPC += sext11(inst_imm11(inst));
		break;
	case OP_LEA:
		mReg[inst_dr(inst)] = mPC + sext9(inst_imm9(inst));
		break;
	case OP_TRAP:
		switch(inst & 0xff) {
		case 21:
			putchar(mReg[0] & 0xff);
			break;
		case 23:
			mReg[0] = getchar() & 0xff;
			break;
		case 25:
			return 1;
		}
	}
	return 0;
}

static inline void
vm_loop(void)
{
	while (1) {
		if (do_instruction())
			break;
	}
	return;
}

int
main(int argc, const char *argv[])
{
	if (argc != 2) {
		fprintf(stderr, "Usage:\n %s Memory_Image\n", argv[0]);
		return -1;
	}

	FILE *image = fopen(argv[1], "rb");
	if (!image) {
		fprintf(stderr, "Cannot open memory image %s\n", argv[1]);
		return -1;
	}
	fread(mMem, 1, 65536, image);
	fclose(image);

	vm_loop();

	return 0;
}
