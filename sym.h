#pragma once

void ReadSymbols(const wchar_t *binname);
void ShutdownSymbols();
const wchar_t* GetSymbol(uint16_t address);
