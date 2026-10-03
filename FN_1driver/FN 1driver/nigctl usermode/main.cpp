// this source is from NewReality a discord server with 1500+ sources: discord.gg/newreality

#include "communcation.hpp"
#include <iostream>
template<typename T>
bool read_safe(uintptr_t address, T& out)
{
    // Zero output
    memset(&out, 0, sizeof(T));

    // Attempt read
    bool ok = communcations::read_memory((PVOID)address, &out, sizeof(T));

    // DeviceIoControl returns TRUE on success, FALSE on failure
    if (!ok)
        return false;

    // Optional sanity check: address NULL?
    if (address < 0x1000)
        return false;

    return true;
}
void test_read()
{
    if (!communcations::find_driver()) {
        std::cout << "Driver not found\n";
        return;
    }
    
    communcations::get_process_id(L"notepad.exe");
    uintptr_t base = communcations::get_base();

    std::cout << "Base: 0x" << std::hex << base << "\n";

    uint16_t signature = 0;
    if (read_safe(base, signature))
    {
        std::cout << "Read OK! Value = 0x" << std::hex << signature << "\n";

        // PE files always begin with 'MZ' → 0x5A4D
        if (signature == 0x5A4D)
            std::cout << "Valid PE header (MZ). Memory read succeeded.\n";
        else
            std::cout << "Read succeeded but invalid signature.\n";
    }
    else
    {
        std::cout << "Read failed.\n";
    }
}
int main()
{
    std::cout << communcations::get_peb_address();
    test_read();
    Sleep(2000);
    communcations::find_driver();
    if (0) {
        std::cout << "Driver not found\n";
		Sleep(2000);
        return 0;
    }
   
   
	Sleep(2000);
    communcations::get_process_id(L"notepad.exe");
    communcations::get_cr3();

    Sleep(6000);
}
