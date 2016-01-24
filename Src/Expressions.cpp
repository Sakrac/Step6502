#include "stdafx.h"
#include <inttypes.h>
#include "machine.h"

// These are expression tokens in order of precedence (last is highest precedence)

enum ExpOp {
	EO_END,
	EO_NONE = EO_END,

	// values
	EO_VAL8,			// fixed value, 8 bits
	EO_VAL16,			// fixed value, 16 bits
	EO_PC,				// current PC
	EO_A,				// accumulator
	EO_X,				// x reg
	EO_Y,				// y reg
	EO_S,				// stack index
	EO_C,				// carry
	EO_Z,				// zero
	EO_I,				// interrupt
	EO_D,				// decimal
	EO_V,				// overflow
	EO_N,				// negative
	EO_BYTE,			// read byte from memory
	EO_2BYTE,			// read 2 bytes from memory

	// operators
	EO_EQU,				// 1 if left equal to right otherwise 0
	EO_LT,				// 1 if left less than right otherwise 0
	EO_GT,				// 1 if left greater than right otherwise 0
	EO_LTE,				// 1 if left less than or equal to right otherwise 0
	EO_GTE,				// 1 if left greater than or equal to right otherwise 0
	EO_CND,				// &&
	EO_COR,				// ||
	EO_LBR,				// left bracket
	EO_RBR,				// right bracket
	EO_LBC,				// left brace
	EO_RBC,				// right brace
	EO_LPR,				// left parenthesis
	EO_RPR,				// right parenthesis
	EO_ADD,				// +
	EO_SUB,				// -
	EO_MUL,				// *
	EO_DIV,				// /
	EO_AND,				// &
	EO_OR,				// |
	EO_EOR,				// ^
	EO_SHL,				// <<
	EO_SHR,				// >>
	EO_NOT,				// !
	EO_NEG,				// negate value
	EO_ERR,				// Error

	EO_VALUES = EO_VAL8,
	EO_OPER = EO_EQU

};

typedef const wchar_t* ExpStr;

static bool IsAlphabetic(const wchar_t c) {
	return (c>=L'A' && c<=L'Z') || (c>=L'a' && c<=L'z');
}

static bool IsNumeric(const wchar_t c) {
	return c>=L'0' && c<=L'9';
}

static bool IsAlphaNumeric(const wchar_t c) {
	return IsNumeric(c) || IsAlphabetic(c);
}

static bool IsHex(const wchar_t c) {
	return IsNumeric(c) || (c>=L'A' && c<=L'F') || (c>=L'a' && c<='f');
}

static uint32_t GetHex(const wchar_t c) {
	return IsNumeric(c) ? (c-L'0') : ((c>=L'A' && c<=L'F') ? (c-L'A'+10) : (c-L'a'+10));
}

ExpStr SkipWS(ExpStr str) {
	while (*str && *str <= L' ')
		str++;
	return str;
}

uint32_t GetHex(ExpStr &str) {
	uint32_t ret = 0;
	while (IsHex(*str))
		ret = (ret<<4) | GetHex(*str++);
	return ret;
}

uint32_t GetDec(ExpStr &str) {
	uint32_t ret = 0;
	while (IsNumeric(*str))
		ret = ret*10 + (*str++ - L'0');
	return ret;
}

uint32_t GetBin(ExpStr &str) {
	uint32_t ret = 0;
	while (*str==L'0' || *str==L'1')
		ret = ret*2 + (*str++ - L'0');
	return ret;
}

wchar_t ToUp(wchar_t c) {
	if (c>='a' && c<='z')
		return c + 'A' - 'a';
	return c;
}


ExpOp ParseOp(ExpStr &str, uint32_t &v)
{
	str = SkipWS(str);
	switch (wchar_t c = *str++) {
		case 0:	return EO_NONE; // end of operation string
		case L'=': if (*str==L'=') ++str; return EO_EQU;	// = or == are both acceptable equal
		case L'<': if (*str==L'=') { ++str; return EO_LTE; } else if (str[1]==L'<') {	++str; return EO_SHL; } return EO_LT;
		case L'>': if (*str==L'=') { ++str; return EO_GTE; } else if (str[1]==L'>') { ++str; return EO_SHR; } return EO_GT;
		case L'(': return EO_LPR;
		case L')': return EO_RPR;
		case L'{': return EO_LBC;
		case L'}': return EO_RBC;
		case L'[': return EO_LBR;
		case L']': return EO_RBR;
		case L'+': return EO_ADD;
		case L'-': return EO_SUB; // may be negate value => sort out in caller
		case L'*': return EO_MUL;
		case L'/': return EO_DIV;
		case L'&': if (*str==L'&') { ++str; return EO_CND; } return EO_AND;
		case L'|': if (*str==L'|') { ++str; return EO_COR; } return EO_OR;
		case L'^': return EO_EOR;
		case L'!': return EO_NOT;
		case L'$': v = GetHex(str); return v<0x100 ? EO_VAL8 : EO_VAL16;
		case L'%': v = GetBin(str); return v<0x100 ? EO_VAL8 : EO_VAL16;
		default:
			if (IsNumeric(c)) {
				v = GetDec(--str);
				return v<0x100 ? EO_VAL8 : EO_VAL16;
			}
			if (IsAlphabetic(c)) {
				if (!IsAlphaNumeric(*str)) {
					switch (ToUp(c)) {
						case L'A': return EO_A;
						case L'X': return EO_X;
						case L'Y': return EO_Y;
						case L'S': return EO_S;
						case L'C': return EO_C;
						case L'Z': return EO_Z;
						case L'I': return EO_I;
						case L'D': return EO_D;
						case L'V': return EO_V;
						case L'N': return EO_N;
					}
				} else if ((c==L'P' || c==L'p') && (*str==L'C' || *str==L'c') && !IsAlphaNumeric(str[1])) {
					++str;
					return EO_PC;
				}
			}
			break;
	}
	return EO_ERR;
}

#define MAX_EXPR_VALUES 32
#define MAX_EXPR_STACK 32
uint32_t BuildExpression(const wchar_t *Expr, uint8_t *ops, uint32_t max_ops)
{
	ExpOp stack[MAX_EXPR_STACK];
	int num_values = 0;
	uint32_t num_ops = 0;
	int sp = 0;
	ExpOp op = EO_NONE, prev_op = EO_NONE;

	while (num_values<MAX_EXPR_VALUES && num_ops<max_ops && sp<MAX_EXPR_STACK) {
		uint32_t v;
		op = ParseOp(Expr, v);
		if (op == EO_NONE || op == EO_ERR)
			break;
		if (op == EO_SUB && prev_op>=EO_OPER && prev_op != EO_RPR && prev_op!=EO_RBR)
			op = EO_NEG;
		if (op < EO_OPER) {
			ops[num_ops++] = op;
			if (op == EO_VAL8 || op == EO_VAL16) {
				ops[num_ops++] = (uint8_t)v;
				if (op == EO_VAL16)
					ops[num_ops++] = (uint8_t)(v>>8);
			}
		} else if (op == EO_LPR || op == EO_LBR || op == EO_LBC)
			stack[sp++] = op;
		else if (op == EO_RPR || op == EO_RBR || op == EO_RBC) {
			ExpOp opo = op == EO_RPR ? EO_LPR : (op == EO_RBC ? EO_LBC : EO_LBR);
			while (sp && stack[sp-1]!=opo)
				ops[num_ops++] = stack[--sp];
			if (!sp || stack[sp-1]!=opo) {
				op = EO_ERR;
				break;
			}
			sp--; // skip open paren
			if (op==EO_RBR)
				ops[num_ops++] = EO_BYTE;
			else if (op == EO_RBC)
				ops[num_ops++] = EO_2BYTE;
		} else {
			while (sp) {
				ExpOp p = (ExpOp)stack[sp-1];
				if (p==EO_LPR || p==EO_LBR || p==EO_LBC || op>p)
					break;
				ops[num_ops++] = p;
				sp--;
			}
			stack[sp++] = op;
		}
		prev_op = op;
	}
	if (op == EO_NONE) {
		while (sp)
			ops[num_ops++] = stack[--sp];
	}
	ops[num_ops++] = EO_END;
	return num_ops;
}

#define MAX_EXPR_VALUE_DEPTH 32
int EvalExpression(const uint8_t *RPN)
{
	int values[MAX_EXPR_VALUE_DEPTH];
	int i = 0;
	const Regs &r = GetRegs();
	bool err = false;

	while (!err && *RPN) {
		uint8_t c = *RPN++;
		switch (c) {
			case EO_VAL8: values[i++] = *RPN++; break;	// fixed value: 32 bits?
			case EO_VAL16: values[i++] = RPN[0] + (((int)RPN[1])<<8); RPN += 2; break;	// fixed value: 32 bits?
			case EO_PC: values[i++] = r.PC; break;	// current PC
			case EO_A: values[i++] = r.A; break; // accumulator
			case EO_X: values[i++] = r.X; break; // x reg
			case EO_Y: values[i++] = r.Y; break; // y reg
			case EO_S: values[i++] = r.S; break; // stack index
			case EO_C: values[i++] = (r.P&F_C) ? 1 : 0; break; // carry
			case EO_Z: values[i++] = (r.P&F_Z) ? 1 : 0; break; // zero
			case EO_I: values[i++] = (r.P&F_I) ? 1 : 0; break; // interrupt
			case EO_D: values[i++] = (r.P&F_D) ? 1 : 0; break; // decimal
			case EO_V: values[i++] = (r.P&F_V) ? 1 : 0; break; // overflow
			case EO_N: values[i++] = (r.P&F_N) ? 1 : 0; break; // negative
			case EO_BYTE:			// read byte from memory
				if (!(err = i<1))
					values[i-1] = Get6502Byte((uint16_t)values[i-1]);
				break;
			case EO_2BYTE:			// read byte from memory
				if (!(err = i<1))
					values[i-1] = Get6502Byte((uint16_t)values[i-1]) +
						((uint16_t)Get6502Byte(uint16_t(values[i-1]+1))<<8);
				break;
			case EO_EQU:				// 1 if left equal to right otherwise 0
				if (!(err = i<2)) {
					i--;
					values[i-1] = values[i-1] == values[i];
				}
				break;
			case EO_LT:				// 1 if left less than right otherwise 0
				if (!(err = i<2)) {
					i--;
					values[i-1] = values[i-1] < values[i];
				}
				break;
			case EO_GT:				// 1 if left greater than right otherwise 0
				if (!(err = i<2)) {
					i--;
					values[i-1] = values[i-1] > values[i];
				}
				break;
			case EO_LTE:				// 1 if left less than or equal to right otherwise 0
				if (!(err = i<2)) {
					i--;
					values[i-1] = values[i-1] <= values[i];
				}
				break;
			case EO_GTE:				// 1 if left greater than or equal to right otherwise 0
				if (!(err = i<2)) {
					i--;
					values[i-1] = values[i-1] >= values[i];
				}
				break;
			case EO_CND:				// &&
				if (!(err = i<2)) {
					i--;
					values[i-1] = values[i-1] && values[i];
				}
				break;
			case EO_COR:				// ||
				if (!(err = i<2)) {
					i--;
					values[i-1] = values[i-1] || values[i];
				}
				break;
			case EO_ADD:				// +
				if (!(err = i<2)) {
					i--;
					values[i-1] = values[i-1] + values[i];
				}
				break;
			case EO_SUB:				// -
				if (!(err = i<2)) {
					i--;
					values[i-1] = values[i-1] - values[i];
				}
				break;
			case EO_MUL:				// *
				if (!(err = i<2)) {
					i--;
					values[i-1] = values[i-1] * values[i];
				}
				break;
			case EO_DIV:				// /
				if (!(err = (i<2 || values[i-1]==0))) {
					i--;
					values[i-1] = values[i-1] / values[i];
				}
				break;
			case EO_AND:				// &
				if (!(err = i<2)) {
					i--;
					values[i-1] = values[i-1] & values[i];
				}
				break;
			case EO_OR:				// |
				if (!(err = i<2)) {
					i--;
					values[i-1] = values[i-1] | values[i];
				}
				break;
			case EO_EOR:				// ^
				if (!(err = i<2)) {
					i--;
					values[i-1] = values[i-1] ^ values[i];
				}
				break;
			case EO_SHL:				// <<
				if (!(err = i<2)) {
					i--;
					values[i-1] = values[i-1] << values[i];
				}
				break;
			case EO_SHR:				// >>
				if (!(err = i<2)) {
					i--;
					values[i-1] = values[i-1] >> values[i];
				}
				break;
			case EO_NEG:				// negate value
				if (!(err = i<1))
					values[i-1] = -values[i-1];
				break;
			case EO_NOT:
				if (!(err = i<1))
					values[i-1] = !values[i-1];
			default:
				err = true;
				break;
		}
	}
	return (err || i!=1) ? 0x80000000 : values[0];
}


#ifdef _DEBUG

//bool TestExpressions()
//{
//	uint8_t ops[64];
//	uint32_t numOps;
//	bool success = true;
//	numOps = BuildExpression(L"2*3+4/2", ops, sizeof(ops));
//	if (EvalExpression(ops) != 8)
//		success = false;
//
//	numOps = BuildExpression(L"2*(-1) - -3/-2", ops, sizeof(ops));
//	if ( EvalExpression(ops) != -3)
//		success = false;
//
//
//	numOps = BuildExpression(L"(13 & 7) | 2", ops, sizeof(ops));
//	if (EvalExpression(ops) != 7)
//		success = false;
//
//	numOps = BuildExpression(L"15 && 12", ops, sizeof(ops));
//	if (EvalExpression(ops) != 1)
//		success = false;
//
//	numOps = BuildExpression(L"15 & 12", ops, sizeof(ops));
//	if (EvalExpression(ops) != 12)
//		success = false;
//
//
//	return true;
//}

#endif
