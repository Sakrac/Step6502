#pragma once

uint32_t BuildExpression(const wchar_t *Expr, uint8_t *ops, uint32_t max_ops);
int EvalExpression(const uint8_t *RPN);
#ifdef _DEBUG
bool TestExpressions();
#endif
