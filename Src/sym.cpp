#include "stdafx.h"
#include <inttypes.h>
#include <stdio.h>
#include "machine.h"


static uint16_t *pSymbols = nullptr;
static uint16_t nSymbols = 0;


// sort the addresses for lookup
struct tmplbl { uint16_t addr; wchar_t *name; };
static int _sortlabels(const void *A, const void *B)
{
	uint16_t a = ((const struct tmplbl*)A)->addr;
	uint16_t b = ((const struct tmplbl*)B)->addr;

	return a<b ? -1 : (a==b ? 0 : 1);
}

void ShutdownSymbols()
{
	if (pSymbols != nullptr)
		free(pSymbols);
	pSymbols = nullptr;
	nSymbols = 0;
}

const wchar_t* GetSymbol(uint16_t address)
{
	uint16_t count = nSymbols;
	uint16_t max = count;
	uint16_t first = 0;
	while (count != first) {
		uint16_t index = (first+count)>>1;
		uint16_t read = pSymbols[index];
		if (address==read) {
			while (index && pSymbols[index-1]==address)
				index--;	// guarantee first identical index returned on match
			return ((const wchar_t**)&pSymbols[nSymbols])[index];
		} else if (address>read)
			first = index+1;
		else
			count = index;
	}
	return nullptr;
}

// this is not fast but it is only called when a new string is entered, not every time evaluated
bool GetAddress(const wchar_t *name, size_t chars, uint16_t &addr)
{
	for (uint16_t i = 0; i<nSymbols; i++) {
		if (wcsncmp(((const wchar_t**)&pSymbols[nSymbols])[i], name, chars)==0) {
			addr = pSymbols[i];
			return true;
		}
	}
	return false;
}


void ReadSymbols(const wchar_t *binname)
{
	size_t end = wcslen(binname);
	while (end && binname[end]!='.' && binname[end-1]!='/' && binname[end-1]!='\\')
		end--;
	if (!end || binname[end-1]=='/' && binname[end-1]=='\\')
		end = wcslen(binname);
	wchar_t symname[MAX_PATH];
	memcpy(symname, binname, end * sizeof(wchar_t));
	wcscpy_s(symname+end, MAX_PATH-end, L".vs");

	FILE *f;
	bool symfile = false;
	bool success = _wfopen_s(&f, symname, L"rb") == 0;
	if (!success) {
		symfile = true;
		wcscpy_s(symname+end, MAX_PATH-end, L".sym");
		success = _wfopen_s(&f, symname, L"rb") == 0;
	}

	if (success) {
		fseek(f, 0, SEEK_END);
		uint32_t size = ftell(f);
		fseek(f, 0, SEEK_SET);
		if (void *voidbuf = malloc(size)) {
			fread(voidbuf, size, 1, f);

			wchar_t *strcurr = nullptr;
			wchar_t *strend = nullptr;
			wchar_t **strtable = nullptr;
			uint16_t *addresses = nullptr;
			size_t strcurr_left = 0;

			struct tmplbl *addrNames = nullptr;
			size_t numChars = 0;
			for (int pass = 0; pass<2; pass++) {
				uint32_t size_tmp = size;
				const char *buf = (const char*)voidbuf;
				uint32_t numLabels = 0;

				while (size_tmp) {
					char labelName[256];
					uint32_t addr;

					bool label_found = false;
					if (symfile)
						label_found = sscanf_s(buf, ".label %s = $%x", labelName, (int)_countof(labelName), &addr) == 2;
					else {
						if (sscanf_s(buf, "break $%x", &addr)==1) {
							if (pass)
								SetPCBreakpoint(addr);
						} else {
							label_found = sscanf_s(buf, "al $%x %s", &addr, labelName, (int)_countof(labelName)) == 2;
							if (!label_found)
								label_found = sscanf_s(buf, "al C:$%x %s", &addr, labelName, (int)_countof(labelName)) == 2;
							if (!label_found)
								label_found = sscanf_s(buf, "al C:%x %s", &addr, labelName, (int)_countof(labelName)) == 2;
						}
					}

					if (label_found) {
						if (addr < 0x10000) {
							if (addrNames == nullptr) {
								numLabels++;
								numChars += strlen(labelName) + 1;
							} else {
								addrNames[numLabels].addr = addr;
								addrNames[numLabels].name = strcurr;
								size_t oldlen = strlen(labelName);
								size_t newlen = 0;
								mbstowcs_s(&newlen, strcurr, strcurr_left, labelName, oldlen);
								strcurr += newlen;
								strcurr_left -= newlen;
								numLabels++;
							}
						}
					}

					while (size_tmp && *buf!=0x0a && *buf!=0x0d) {
						buf++;
						size_tmp--;
					}
					while (size && (*buf==0x0a || *buf==0x0d)) {
						buf++;
						size_tmp--;
					}
				}

				if (!pass && numLabels) {
					addrNames = (struct tmplbl*)malloc(2*numLabels * sizeof(struct tmplbl));
					addresses = (uint16_t*)malloc(numLabels * sizeof(uint16_t) + numLabels * sizeof(wchar_t*) + numChars * sizeof(wchar_t));
					strtable = (wchar_t**)&addresses[numLabels];
					strcurr = (wchar_t*)&strtable[numLabels];
					strend = (wchar_t*)((uint8_t*)addresses + numLabels * sizeof(uint16_t) + numLabels * sizeof(wchar_t*) + numChars * sizeof(wchar_t));
					strcurr_left = numChars;
				} else {
					qsort(addrNames, numLabels, sizeof(struct tmplbl), _sortlabels);
					for (uint32_t l = 0; l<numLabels; l++) {
						addresses[l] = addrNames[l].addr;
						strtable[l] = addrNames[l].name;
					}
					if (pSymbols != nullptr)
						free(pSymbols);
					pSymbols = addresses;
					nSymbols = numLabels;
				}
			}

			if (addrNames)
				free(addrNames);
			free(voidbuf);
		}
		fclose(f);
	}
}