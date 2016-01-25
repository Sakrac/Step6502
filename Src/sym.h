#pragma once

void ReadSymbols(const wchar_t *binname);
void ShutdownSymbols();
bool GetAddress(const wchar_t *name, size_t chars, uint16_t &addr);
const wchar_t* GetSymbol(uint16_t address);
