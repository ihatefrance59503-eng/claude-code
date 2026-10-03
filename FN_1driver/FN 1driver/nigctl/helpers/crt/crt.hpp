// this source is from NewReality a discord server with 1500+ sources: discord.gg/newreality
#pragma once
#include <stdlib.h>
#include <ntifs.h>
#include <ntddk.h>
#include <ntimage.h>
#include <intrin.h>
#include <cstdint>
#include <stdio.h>

namespace crt
	{
		char chrlwr(char c) {

			if (c >= 'A' && c <= 'Z') return c - 'A' + 'a';
			return c;
		}

		int stricmp(const char* cs, const char* ct) {
			if (cs && ct) {
				while (chrlwr(*cs) == chrlwr(*ct)) {
					if (*cs == 0 && *ct == 0) return 0;
					if (*cs == 0 || *ct == 0) break;
					cs++;
					ct++;
				}
				return chrlwr(*cs) - chrlwr(*ct);
			}
			return -1;
		}

		int lower(int c) {
			if (c >= 'A' && c <= 'Z')
				return c + 'a' - 'A';
			else
				return c;
		}

		int wcscmp(const wchar_t* s1, const wchar_t* s2) {
			while (*s1 == *s2++)
				if (*s1++ == '\0')
					return (0);

			return (*(const unsigned int*)s1 - *(const unsigned int*)--s2);
		}

		char* lowerstr(char* Str) {
			for (CHAR* S = Str; *S; ++S)
			{
				*S = (CHAR)lower(*S);
			}
			return Str;
		}

		size_t strlen(const char* str) {
			const char* s;
			for (s = str; *s; ++s);
			return (s - str);
		}

		int strncmp(const char* s1, const char* s2, size_t n) {
			if (n == 0)
				return (0);
			do {
				if (*s1 != *s2++)
					return (*(unsigned char*)s1 - *(unsigned char*)--s2);
				if (*s1++ == 0)
					break;
			} while (--n != 0);
			return (0);
		}

		int strcmp(const char* s1, const char* s2) {
			while (*s1 == *s2++)
				if (*s1++ == 0)
					return (0);
			return (*(unsigned char*)s1 - *(unsigned char*)--s2);
		}

		char* strstr(const char* s, const char* find) {
			char c, sc;
			size_t len;
			if ((c = *find++) != 0)
			{
				len = strlen(find);
				do
				{
					do
					{
						if ((sc = *s++) == 0)
						{
							return (NULL);
						}
					} while (sc != c);
				} while (strncmp(s, find, len) != 0);
				s--;
			}
			return ((char*)s);
		}
		static char* stristr(const char* str1, const char* str2) {
			const char* p1 = str1;
			const char* p2 = str2;
			const char* r = *p2 == 0 ? str1 : 0;
			while (*p1 != 0 && *p2 != 0)
			{
				if (lower((unsigned char)*p1) == lower((unsigned char)*p2))
				{
					if (r == 0)
					{
						r = p1;
					}
					p2++;
				}
				else
				{
					p2 = str2;
					if (r != 0)
					{
						p1 = r + 1;
					}
					if (lower((unsigned char)*p1) == lower((unsigned char)*p2))
					{
						r = p1;
						p2++;
					}
					else
					{
						r = 0;
					}
				}
				p1++;
			}
			return *p2 == 0 ? (char*)r : 0;
		}

		int memcmp(const void* s1, const void* s2, size_t n) {
			const unsigned char* p1 = (const unsigned char*)s1;
			const unsigned char* end1 = p1 + n;
			const unsigned char* p2 = (const unsigned char*)s2;
			int                   d = 0;
			for (;;) {
				if (d || p1 >= end1) break;
				d = (int)*p1++ - (int)*p2++;
				if (d || p1 >= end1) break;
				d = (int)*p1++ - (int)*p2++;
				if (d || p1 >= end1) break;
				d = (int)*p1++ - (int)*p2++;
				if (d || p1 >= end1) break;
				d = (int)*p1++ - (int)*p2++;
			}
			return d;
		}

		void* memcpy(void* dest, const void* src, size_t len) {
			char* d = (char*)dest;
			const char* s = (const char*)src;
			while (len--)
				*d++ = *s++;
			return dest;
		}

		void* memset(void* dest, unsigned char c, unsigned __int64 count) {
			unsigned __int64 blockIdx;
			unsigned __int64 blocks = count >> 3;
			unsigned __int64 bytesLeft = count - (blocks << 3);

			UINT64 cUll =
				c
				| (((UINT64)c) << 8)
				| (((UINT64)c) << 16)
				| (((UINT64)c) << 24)
				| (((UINT64)c) << 32)
				| (((UINT64)c) << 40)
				| (((UINT64)c) << 48)
				| (((UINT64)c) << 56);

			UINT64* destPtr8 = (UINT64*)dest;
			for (blockIdx = 0; blockIdx < blocks; blockIdx++) destPtr8[blockIdx] = cUll;

			if (!bytesLeft) return dest;

			blocks = bytesLeft >> 2;
			bytesLeft -= (blocks << 2);

			UINT32* destPtr4 = (UINT32*)&destPtr8[blockIdx];
			for (blockIdx = 0; blockIdx < blocks; blockIdx++) destPtr4[blockIdx] = (UINT32)cUll;

			if (!bytesLeft) return dest;

			blocks = bytesLeft >> 1;
			bytesLeft -= (blocks << 1);

			UINT16* destPtr2 = (UINT16*)&destPtr4[blockIdx];
			for (blockIdx = 0; blockIdx < blocks; blockIdx++) destPtr2[blockIdx] = (UINT16)cUll;

			if (!bytesLeft) return dest;

			UINT8* destPtr1 = (UINT8*)&destPtr2[blockIdx];
			for (blockIdx = 0; blockIdx < bytesLeft; blockIdx++) destPtr1[blockIdx] = (UINT8)cUll;

			return dest;
		}
		size_t wcslen_nt(const wchar_t* str)
		{
			size_t len = 0;
			while (*str++) len++;
			return len;
		}

		int wcsncmp_nt(const wchar_t* s1, const wchar_t* s2, size_t n)
		{
			for (size_t i = 0; i < n; i++)
			{
				if (s1[i] != s2[i]) return s1[i] - s2[i];
				if (s1[i] == L'\0') break;
			}
			return 0;
		}

		wchar_t* wcsstr(const wchar_t* str, const wchar_t* find)
		{
			wchar_t c, sc;
			size_t len;

			if ((c = *find++) == 0)
				return (wchar_t*)str;

			len = wcslen_nt(find);  // use kernel-safe version

			do
			{
				do
				{
					if ((sc = *str++) == 0)
						return nullptr;
				} while (sc != c);
			} while (wcsncmp_nt(str, find, len) != 0); // use kernel-safe version

			return (wchar_t*)(str - 1);
		}

		wchar_t* wcsstr(wchar_t* s1, const wchar_t* s2)
		{
			return (wchar_t*)wcsstr((const wchar_t*)s1, s2);
		}

}