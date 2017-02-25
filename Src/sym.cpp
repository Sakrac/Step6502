#include "stdafx.h"
#include <inttypes.h>
#include <stdio.h>
#include "machine.h"
#include <map>
#include <string>

// 2017-02-21: Replaced memory friendly label system with wasteful easier to manage symbol lookup.

static std::map<uint16_t, CString>* sSymbols = nullptr;
static std::map<uint64_t, uint16_t>* sReverse = nullptr;


void ShutdownSymbols()
{
	if (sSymbols) {
		delete sSymbols;
		sSymbols = nullptr;
	}
	if (sReverse) {
		delete sReverse;
		sReverse = nullptr;
	}
}

const wchar_t* GetSymbol(uint16_t address)
{
	if (sSymbols) {
		std::map<uint16_t, CString>::const_iterator f = sSymbols->find(address);
		if (f!=sSymbols->end()) { return f->second; }
	}
	return nullptr;
}

static uint64_t fnv1a_64(const wchar_t *string, size_t length, uint64_t seed = 14695981039346656037ULL)
{
	uint64_t hash = seed;
	if (string) {
		size_t left = length;
		while (left--)
			hash = (*string++ ^ hash) * 1099511628211;
	}
	return hash;
}


// this is not fast but it is only called when a new string is entered, not every time evaluated
bool GetAddress(const wchar_t *name, size_t chars, uint16_t &addr)
{
	if( sReverse ) {
		uint64_t key = fnv1a_64(name, chars);

		std::map<uint64_t, uint16_t>::const_iterator i = sReverse->find(key);
		if( i!=sReverse->end()) {
			addr = i->second;
			return true;
		}
	}
	return false;
}

void AddSymbol(uint16_t address, const wchar_t *name, size_t chars)
{
	if (sSymbols == nullptr) { sSymbols = new std::map<uint16_t, CString>(); }
	if (sSymbols->find(address) == sSymbols->end() ) {
		sSymbols->insert(std::pair<uint16_t, CString>(address, CString(name, (int)chars)));
	}
	if (sReverse == nullptr) { sReverse = new std::map<uint64_t, uint16_t>(); }
	uint64_t key = fnv1a_64(name, chars);
	if (sReverse->find(key) == sReverse->end() ) {
		sReverse->insert(std::pair<uint64_t, uint16_t>(key, address));
	}
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
							wchar_t name[512];
							size_t oldlen = strlen(labelName);
							size_t newlen = 0;
							mbstowcs_s(&newlen, name, 512, labelName, oldlen);
							AddSymbol(addr, name, newlen-1);
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
			}

			if (addrNames)
				free(addrNames);
			free(voidbuf);
		}
		fclose(f);
	}
}