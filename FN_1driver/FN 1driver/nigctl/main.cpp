// this source is from NewReality a discord server with 1500+ sources: discord.gg/newreality
#pragma once
#include "core/creation/creation.hpp"

extern "C" int _fltused = 0;


//\x48\x8B\x05\x00\x00\x00\x00\x48\x85\xC0\x74\x00\x48\x8D\x8C\x24\x00\x00\x00\x00\xFF\x15\x00\x00\x00\x00\x48\x85\xFF xxx????xxxx?xxxx????xx????xxx
// \x48\x8B\x05\x00\x00\x00\x00\x48\x85\xC0\x74\x00\xFF\x15\x00\x00\x00\x00\x85\xC0\x78\x00\x48\x8B\x05\x00\x00\x00\x00\x48\x85\xC0\x74\x00\xFF\x15\x00\x00\x00\x00\x8B\xD8\x4C\x39\x25 xxx????xxxx?xx????xxx?xxx????xxxx?xx????xxxxx

extern "C" NTSTATUS DriverEntry(PDRIVER_OBJECT drv_obj, PUNICODE_STRING reg_pth) {
    volatile unsigned int BIT_JUNK1 = 0xFFFFFFFF;
    volatile unsigned int BIT_JUNK2 = 0xAAAAAAAA;
    volatile unsigned int BIT_JUNK3 = 0x55555555;
    volatile unsigned int BIT_JUNK4 = 0x12345678;

    for (volatile int shift = 0; shift < 16; shift++) {
        BIT_JUNK1 = (BIT_JUNK1 >> shift) | (BIT_JUNK1 << (32 - shift));
        BIT_JUNK2 = BIT_JUNK2 ^ BIT_JUNK3;
        BIT_JUNK3 = BIT_JUNK3 & BIT_JUNK4;
        BIT_JUNK4 = BIT_JUNK4 | BIT_JUNK1;
    }

    volatile struct JunkStruct {
        int a, b, c;
        float x, y, z;
    } junk_data = { 1, 2, 3, 1.5f, 2.5f, 3.5f };

    for (volatile int i = 0; i < 6; i++) {
        junk_data.a += junk_data.b * junk_data.c;
        junk_data.x = junk_data.y * junk_data.z - junk_data.x;
        junk_data.b = (junk_data.a ^ junk_data.c) + i;
    }
    imports::resolve_imports();
    UNREFERENCED_PARAMETER(reg_pth);
    UNREFERENCED_PARAMETER(drv_obj);
    volatile int PRIME_JUNK1 = 2147483647;
    volatile int PRIME_JUNK2 = 104729;
    volatile int PRIME_JUNK3 = 1000003;

    volatile bool is_prime = true;
    for (volatile int i = 2; i * i <= PRIME_JUNK1 && is_prime; i++) {
        if (PRIME_JUNK1 % i == 0) {
            is_prime = false;
        }
    }

    volatile long double precise_junk = 0.1234567890123456789L;
    for (volatile int precision = 0; precision < 100; precision++) {
        precise_junk = precise_junk * 1.0000000001L + 0.000000000000000001L;
    }

    volatile int array_junk[20];
    for (volatile int i = 0; i < 20; i++) {
        array_junk[i] = (i * PRIME_JUNK2) % PRIME_JUNK3;
    }

    for (volatile int i = 19; i >= 0; i--) {
        array_junk[i] = array_junk[i] ^ array_junk[19 - i];
    }
    return (imports::IoCreateDriver)(0, &creation::driver_init);
}
