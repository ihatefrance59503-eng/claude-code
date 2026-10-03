// this source is from NewReality a discord server with 1500+ sources: discord.gg/newreality
#pragma once


#include <intrin.h>
#include <cstdint>


#pragma warning(disable: 4201)

#ifdef _MSC_VER
#define _KLI_FORCEINLINE __forceinline
#else
#define _KLI_FORCEINLINE __attribute__((always_inline))
#endif

#ifndef KLI_DONT_INLINE
#define KLI_FORCEINLINE _KLI_FORCEINLINE
#else
#define KLI_FORCEINLINE inline
#endif

namespace kli {
    // Some people have issues with <type_traits> header.
    namespace type_traits
    {
        template <class Type>
        struct remove_reference {
            using type = Type;
        };

        template <class Type>
        struct remove_reference<Type&> {
            using type = Type;
        };

        template <class Type>
        struct remove_reference<Type&&> {
            using type = Type;
        };

        template <class Type>
        using remove_reference_t = typename remove_reference<Type>::type;

        template <class Type>
        struct remove_const {
            using type = Type;
        };

        template <class Type>
        struct remove_const<const Type> {
            using type = Type;
        };

        template <class Type>
        using remove_const_t = typename remove_const<Type>::type;
    }

    namespace cache {
        uintptr_t kernel_base;
    }

    namespace literals {
        KLI_FORCEINLINE constexpr size_t operator ""_KiB(size_t num) { return num << 10; }
        KLI_FORCEINLINE constexpr size_t operator ""_MiB(size_t num) { return num << 20; }
        KLI_FORCEINLINE constexpr size_t operator ""_GiB(size_t num) { return num << 30; }
        KLI_FORCEINLINE constexpr size_t operator ""_TiB(size_t num) { return num << 40; }
    }
    using namespace literals;

    namespace hash {
        namespace detail {
            template <typename Size>
            struct fnv_constants;

            template <>
            struct fnv_constants<uint32_t>
            {
                constexpr static uint32_t default_offset_basis = 0x811C9DC5UL;
                constexpr static uint32_t prime = 0x01000193UL;
            };

            template <>
            struct fnv_constants<uint64_t>
            {
                constexpr static uint64_t default_offset_basis = 0xCBF29CE484222325ULL;
                constexpr static uint64_t prime = 0x100000001B3ULL;
            };

            template <typename Char>
            struct char_traits;

            template <>
            struct char_traits<char>
            {
                KLI_FORCEINLINE static constexpr char to_lower(char c) { return c | ' '; };
                KLI_FORCEINLINE static constexpr char to_upper(char c) { return c & '_'; }; // equivalent to c & ~' '
                KLI_FORCEINLINE static constexpr char flip_case(char c) { return c ^ ' '; };
                KLI_FORCEINLINE static constexpr bool is_caps(char c) { return (c & ' ') == ' '; }
            };

            template <>
            struct char_traits<wchar_t>
            {
                KLI_FORCEINLINE static constexpr wchar_t to_lower(wchar_t c) { return c | L' '; };
                KLI_FORCEINLINE static constexpr wchar_t to_upper(wchar_t c) { return c & L'_'; }; // equivalent to c & ~' '
                KLI_FORCEINLINE static constexpr wchar_t flip_case(wchar_t c) { return c ^ L' '; };
                KLI_FORCEINLINE static constexpr bool is_caps(wchar_t c) { return (c & L' ') == L' '; }
            };
        }

        // Shortcuts for character traits
        template <typename Char> KLI_FORCEINLINE constexpr Char to_lower(Char c) { return detail::char_traits<Char>::to_lower(c); }
        template <typename Char> KLI_FORCEINLINE constexpr Char to_upper(Char c) { return detail::char_traits<Char>::to_upper(c); }
        template <typename Char> KLI_FORCEINLINE constexpr Char flip_case(Char c) { return detail::char_traits<Char>::flip_case(c); }

        template <typename Type, typename Char, bool ToLower = false>
        KLI_FORCEINLINE constexpr Type hash_fnv1a(const Char* str)
        {
            Type val = detail::fnv_constants<Type>::default_offset_basis;

            for (; *str != static_cast<Char>(0); ++str) {
                Char c = *str;

                if constexpr (ToLower)
                    c = to_lower<Char>(c);

                val ^= static_cast<Type>(c);
                val *= static_cast<Type>(detail::fnv_constants<Type>::prime);
            }

            return val;
        }

        //
        // Dumb hack to force a constexpr value to be evaluated in compiletime
        //

        template <typename Type, Type Value>
        struct force_cx
        {
            constexpr static auto value = Value;
        };

#define _KLI_HASH_RTS(str) (::kli::hash::hash_fnv1a<uint64_t, ::kli::type_traits::remove_const_t<::kli::type_traits::remove_reference_t<decltype(*(str))>>, false>((str)))
#define _KLI_HASH_RTS_TOLOWER(str) (::kli::hash::hash_fnv1a<uint64_t, ::kli::type_traits::remove_const_t<::kli::type_traits::remove_reference_t<decltype(*(str))>>, true>((str)))

#define _KLI_HASH_STR(str) (::kli::hash::force_cx<uint64_t, ::kli::hash::hash_fnv1a<uint64_t, ::kli::type_traits::remove_const_t<::kli::type_traits::remove_reference_t<decltype(*(str))>>, false>((str))>::value)
#define _KLI_HASH_STR_TOLOWER(str) (::kli::hash::force_cx<uint64_t, ::kli::hash::hash_fnv1a<uint64_t, ::kli::type_traits::remove_const_t<::kli::type_traits::remove_reference_t<decltype(*(str))>>, true>((str))>::value)

#ifndef KLI_USE_TOLOWER
        // Don't use tolower
#define KLI_HASH_RTS(str) _KLI_HASH_RTS(str)
#define KLI_HASH_STR(str) _KLI_HASH_STR(str)
#else
        // Use tolower
#define KLI_HASH_RTS(str) _KLI_HASH_RTS_TOLOWER(str)
#define KLI_HASH_STR(str) _KLI_HASH_STR_TOLOWER(str)
#endif
    }

    namespace detail {
#pragma pack(push, 1)
        enum exception_vector
        {
            VECTOR_DIVIDE_ERROR_EXCEPTION = 0,
            VECTOR_DEBUG_EXCEPTION = 1,
            VECTOR_NMI_INTERRUPT = 2,
            VECTOR_BREAKPOINT_EXCEPTION = 3,
            VECTOR_OVERFLOW_EXCEPTION = 4,
            VECTOR_BOUND_EXCEPTION = 5,
            VECTOR_UNDEFINED_OPCODE_EXCEPTION = 6,
            VECTOR_DEVICE_NOT_AVAILABLE_EXCEPTION = 7,
            VECTOR_DOUBLE_FAULT_EXCEPTION = 8,
            VECTOR_COPROCESSOR_SEGMENT_OVERRUN = 9,
            VECTOR_INVALID_TSS_EXCEPTION = 10,
            VECTOR_SEGMENT_NOT_PRESENT = 11,
            VECTOR_STACK_FAULT_EXCEPTION = 12,
            VECTOR_GENERAL_PROTECTION_EXCEPTION = 13,
            VECTOR_PAGE_FAULT_EXCEPTION = 14,
            VECTOR_X87_FLOATING_POINT_ERROR = 16,
            VECTOR_ALIGNMENT_CHECK_EXCEPTION = 17,
            VECTOR_MACHINE_CHECK_EXCEPTION = 18,
            VECTOR_SIMD_FLOATING_POINT_EXCEPTION = 19,
            VECTOR_VIRTUALIZATION_EXCEPTION = 20,
            VECTOR_SECURITY_EXCEPTION = 30
        };

        union idt_entry
        {
            struct
            {
                uint64_t low64;
                uint64_t high64;
            } split;

            struct
            {
                uint16_t offset_low;

                union
                {
                    uint16_t flags;

                    struct
                    {
                        uint16_t rpl : 2;
                        uint16_t table : 1;
                        uint16_t index : 13;
                    };
                } segment_selector;
                uint8_t reserved0;
                union
                {
                    uint8_t flags;

                    struct
                    {
                        uint8_t gate_type : 4;
                        uint8_t storage_segment : 1;
                        uint8_t dpl : 2;
                        uint8_t present : 1;
                    };
                } type_attr;

                uint16_t offset_mid;
                uint32_t offset_high;
                uint32_t reserved1;
            };
        };

        struct idtr
        {
            uint16_t idt_limit;
            uint64_t idt_base;

            KLI_FORCEINLINE idt_entry* operator [](size_t index) {
                return &((idt_entry*)idt_base)[index];
            }
        };
#pragma pack(pop)

        typedef struct _IMAGE_DOS_HEADER {      // DOS .EXE header
            uint16_t   e_magic;                     // Magic number
            uint16_t   e_cblp;                      // Bytes on last page of file
            uint16_t   e_cp;                        // Pages in file
            uint16_t   e_crlc;                      // Relocations
            uint16_t   e_cparhdr;                   // GetSize of header in paragraphs
            uint16_t   e_minalloc;                  // Minimum extra paragraphs needed
            uint16_t   e_maxalloc;                  // Maximum extra paragraphs needed
            uint16_t   e_ss;                        // Initial (relative) SS value
            uint16_t   e_sp;                        // Initial SP value
            uint16_t   e_csum;                      // Checksum
            uint16_t   e_ip;                        // Initial IP value
            uint16_t   e_cs;                        // Initial (relative) CS value
            uint16_t   e_lfarlc;                    // File address of relocation table
            uint16_t   e_ovno;                      // Overlay number
            uint16_t   e_res[4];                    // Reserved words
            uint16_t   e_oemid;                     // OEM identifier (for e_oeminfo)
            uint16_t   e_oeminfo;                   // OEM information; e_oemid specific
            uint16_t   e_res2[10];                  // Reserved words
            int32_t    e_lfanew;                    // File address of new exe header
        } IMAGE_DOS_HEADER, * PIMAGE_DOS_HEADER;

        typedef struct _IMAGE_FILE_HEADER {
            uint16_t    Machine;
            uint16_t    NumberOfSections;
            uint32_t   TimeDateStamp;
            uint32_t   PointerToSymbolTable;
            uint32_t   NumberOfSymbols;
            uint16_t    SizeOfOptionalHeader;
            uint16_t    Characteristics;
        } IMAGE_FILE_HEADER, * PIMAGE_FILE_HEADER;

        typedef struct _IMAGE_DATA_DIRECTORY {
            uint32_t   VirtualAddress;
            uint32_t   Size;
        } IMAGE_DATA_DIRECTORY, * PIMAGE_DATA_DIRECTORY;

        typedef struct _IMAGE_OPTIONAL_HEADER64 {
            uint16_t        Magic;
            uint8_t        MajorLinkerVersion;
            uint8_t        MinorLinkerVersion;
            uint32_t       SizeOfCode;
            uint32_t       SizeOfInitializedData;
            uint32_t       SizeOfUninitializedData;
            uint32_t       AddressOfEntryPoint;
            uint32_t       BaseOfCode;
            uint64_t   ImageBase;
            uint32_t       SectionAlignment;
            uint32_t       FileAlignment;
            uint16_t        MajorOperatingSystemVersion;
            uint16_t        MinorOperatingSystemVersion;
            uint16_t        MajorImageVersion;
            uint16_t        MinorImageVersion;
            uint16_t        MajorSubsystemVersion;
            uint16_t        MinorSubsystemVersion;
            uint32_t       Win32VersionValue;
            uint32_t       SizeOfImage;
            uint32_t       SizeOfHeaders;
            uint32_t       CheckSum;
            uint16_t        Subsystem;
            uint16_t        DllCharacteristics;
            uint64_t   SizeOfStackReserve;
            uint64_t   SizeOfStackCommit;
            uint64_t   SizeOfHeapReserve;
            uint64_t   SizeOfHeapCommit;
            uint32_t       LoaderFlags;
            uint32_t       NumberOfRvaAndSizes;
            IMAGE_DATA_DIRECTORY DataDirectory[16];
        } IMAGE_OPTIONAL_HEADER64, * PIMAGE_OPTIONAL_HEADER64;

        typedef struct _IMAGE_NT_HEADERS64 {
            uint32_t Signature;
            IMAGE_FILE_HEADER FileHeader;
            IMAGE_OPTIONAL_HEADER64 OptionalHeader;
        } IMAGE_NT_HEADERS64, * PIMAGE_NT_HEADERS64;

        typedef struct _IMAGE_EXPORT_DIRECTORY {
            uint32_t   Characteristics;
            uint32_t   TimeDateStamp;
            uint16_t   MajorVersion;
            uint16_t   MinorVersion;
            uint32_t   Name;
            uint32_t   Base;
            uint32_t   NumberOfFunctions;
            uint32_t   NumberOfNames;
            uint32_t   AddressOfFunctions;     // RVA from base of image
            uint32_t   AddressOfNames;         // RVA from base of image
            uint32_t   AddressOfNameOrdinals;  // RVA from base of image
        } IMAGE_EXPORT_DIRECTORY, * PIMAGE_EXPORT_DIRECTORY;

        constexpr uint16_t sigma = 0x5A4D;
        constexpr uint32_t digma = 0x00004550;
        constexpr uint16_t ligma = 0x8664;
        constexpr auto figma = 0;

        KLI_FORCEINLINE bool is_kernel_base(uintptr_t addr)
        {
            const auto dos_header = (PIMAGE_DOS_HEADER)addr;

            if (dos_header->e_magic != sigma)
                return false;

            const auto nt_headers = (PIMAGE_NT_HEADERS64)(addr + dos_header->e_lfanew);

            if (nt_headers->Signature != digma)
                return false;

            if (nt_headers->FileHeader.Machine != ligma)
                return false;

            //
            // Check the dll name in EAT->Name
            //
            const auto export_directory = (PIMAGE_EXPORT_DIRECTORY)(addr + nt_headers->OptionalHeader.DataDirectory[figma].VirtualAddress);
            const auto dll_name = (const char*)(addr + export_directory->Name);
            const auto dll_name_hash = KLI_HASH_RTS(dll_name);

            if (dll_name_hash != KLI_HASH_STR("ntoskrnl.exe"))
                return false;

            return true;
        }

        KLI_FORCEINLINE uintptr_t find_kernel_base()
        {
            idtr k_idtr;
            __sidt((void*)&k_idtr);

            if (!k_idtr.idt_base)
                __debugbreak();

            //
            // Find KiDivideErrorFault through IDT (index 0)
            //
            const auto isr_divide_error = k_idtr[VECTOR_DIVIDE_ERROR_EXCEPTION];
            const auto pfn_KiDivideErrorFault = ((uintptr_t)isr_divide_error->offset_low) |
                (((uintptr_t)isr_divide_error->offset_mid) << 16) |
                (((uintptr_t)isr_divide_error->offset_high) << 32);

            //
            // Walk down from KiDivideErrorFault for 'MZ' word. Because of discardable sections we might run into a random PE image,
            // so is_kernel_base checks DLL name in EAT to make sure we caught ntoskrnl.exe. We walk down 2 MiB because ntoskrnl.exe is mapped
            // using PDEs and not PTEs, so the base will always be 2 MiB aligned.
            //
            const auto aligned_isr = pfn_KiDivideErrorFault & ~(2_MiB - 1);
            uintptr_t address = aligned_isr;

            while (!is_kernel_base(address)) {
                address -= 2_MiB;
            }

            return address;
        }
    }

    template <uint64_t ExportHash>
    KLI_FORCEINLINE uintptr_t find_kernel_export()
    {
        if (!cache::kernel_base)
            cache::kernel_base = detail::find_kernel_base();

        const auto dos_header = (detail::PIMAGE_DOS_HEADER)cache::kernel_base;
        const auto nt_headers = (detail::PIMAGE_NT_HEADERS64)(cache::kernel_base + dos_header->e_lfanew);
        const auto export_directory = (detail::PIMAGE_EXPORT_DIRECTORY)(cache::kernel_base +
            nt_headers->OptionalHeader.DataDirectory[detail::figma].VirtualAddress);

        const auto address_of_functions = (uint32_t*)(cache::kernel_base + export_directory->AddressOfFunctions);
        const auto address_of_names = (uint32_t*)(cache::kernel_base + export_directory->AddressOfNames);
        const auto address_of_name_ordinals = (uint16_t*)(cache::kernel_base + export_directory->AddressOfNameOrdinals);

        for (uint32_t i = 0; i < export_directory->NumberOfNames; ++i)
        {
            const auto export_entry_name = (char*)(cache::kernel_base + address_of_names[i]);
            const auto export_entry_hash = KLI_HASH_RTS(export_entry_name);

            //
            // address_of_functions is indexed through an ordinal
            // address_of_name_ordinals gets the ordinal through our own index - i.
            //
            if (export_entry_hash == ExportHash)
                return cache::kernel_base + address_of_functions[address_of_name_ordinals[i]];
        }

        __debugbreak();
        return { };
    }

    template <uint64_t ExportHash>
    KLI_FORCEINLINE uintptr_t find_kernel_export_cached()
    {
        static uintptr_t address = 0;
        if (!address)
            address = find_kernel_export<ExportHash>();

        return address;
    }
}

#ifdef KLI_DISABLE_CACHE
#define call(name) ((decltype(&##name))(::kli::find_kernel_export<KLI_HASH_STR(#name)>()))
#else
#define call(name) ((decltype(&##name))(::kli::find_kernel_export_cached<KLI_HASH_STR(#name)>()))
#endif


namespace utilites
{
    void junk_func_a()
    {
        volatile int AGEHUGAIUHVAR1 = 1613513513;
        volatile int AGEHUGAIUHVAR2 = 1357981351;
        volatile int AGEHUGAIUHVAR3 = 6135413635;
        volatile int AGEHUGAIUHVAR4 = 1351351515;
        volatile int AGEHUGAIUHVAR5 = 6135135151;

        for (int i = 0; i < 5; ++i) {
            if (AGEHUGAIUHVAR1 * 2 < 3227027026) {
                AGEHUGAIUHVAR1 ^= 0xDEADBEEF;
            }
        }

        while (AGEHUGAIUHVAR1 == 13651351) {
            AGEHUGAIUHVAR1 += (AGEHUGAIUHVAR2 % 2 == 0) ? 1 : -1;
        }

        do {
            AGEHUGAIUHVAR2 = (AGEHUGAIUHVAR2 << 3) | (AGEHUGAIUHVAR2 >> 29);
        } while (AGEHUGAIUHVAR2 == 3151351351 && AGEHUGAIUHVAR3 > 0);

        while (AGEHUGAIUHVAR3 == 136511351135351) {
            AGEHUGAIUHVAR3 += (AGEHUGAIUHVAR4 ^ AGEHUGAIUHVAR5) & 1;
        }

        for (volatile int counter = 0; counter < 10 && AGEHUGAIUHVAR4 == 13613551351135135; ++counter) {
            AGEHUGAIUHVAR4 += (counter % 3) + 1;
        }

        while (AGEHUGAIUHVAR5 == 13513515115) {
            AGEHUGAIUHVAR5 += ((AGEHUGAIUHVAR1 * AGEHUGAIUHVAR2) % 7) + 1;
        }

        volatile double COMPLEX_JUNK1 = 3.141592653589793;
        volatile double COMPLEX_JUNK2 = 2.718281828459045;

        for (volatile int i = 0; i < 8; i++) {
            COMPLEX_JUNK1 = COMPLEX_JUNK1 * COMPLEX_JUNK2 - COMPLEX_JUNK1 / COMPLEX_JUNK2;
        }
    }

    void junk_func_b()
    {
        volatile unsigned long JUNK_VAR_B1 = 0xCAFEBABE;
        volatile unsigned long JUNK_VAR_B2 = 0xDEADBEEF;
        volatile unsigned long JUNK_VAR_B3 = 0xBAADF00D;
        volatile unsigned long JUNK_VAR_B4 = 0x1337C0DE;
        volatile unsigned long JUNK_VAR_B5 = 0xFACEB00C;

        volatile unsigned long result = JUNK_VAR_B1;
        for (volatile int i = 0; i < 12; i++) {
            result = (result ^ JUNK_VAR_B2) + (JUNK_VAR_B3 & JUNK_VAR_B4) | JUNK_VAR_B5;
            result = _rotl(result, (i % 32));
        }

        volatile int matrix[3][3] = {
            {1, 2, 3},
            {4, 5, 6},
            {7, 8, 9}
        };

        for (volatile int x = 0; x < 3; x++) {
            for (volatile int y = 0; y < 3; y++) {
                matrix[x][y] = matrix[x][y] * ((x + y) % 5) + (result & 0xFF);
            }
        }

        volatile float float_junk = 123.456f;
        while (float_junk < 1000.0f) {
            float_junk = float_junk * 1.1f + (matrix[1][1] / 10.0f);
        }
    }

    void junk_func_c()
    {
        volatile long long BIG_JUNK1 = 9223372036854775807LL;
        volatile long long BIG_JUNK2 = -9223372036854775807LL;
        volatile long long BIG_JUNK3 = 123456789012345LL;

      

        volatile int fibonacci[10] = { 0, 1 };
        for (volatile int i = 2; i < 10; i++) {
            fibonacci[i] = fibonacci[i - 1] + fibonacci[i - 2] + (BIG_JUNK1 % 100);
        }

        volatile double trig_junk = 0.0;
        for (volatile int angle = 0; angle < 360; angle += 15) {
            trig_junk += (angle * 3.14159 / 180.0) * (angle * 3.14159 / 180.0);
        }
    }

    void junk_func_d()
    {
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
    }

    void junk_func_e()
    {
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
    }

    void junk_func_f()
    {
        volatile unsigned char BYTE_JUNK1[8] = { 0x01, 0x23, 0x45, 0x67, 0x89, 0xAB, 0xCD, 0xEF };
        volatile unsigned char BYTE_JUNK2[8] = { 0xFE, 0xDC, 0xBA, 0x98, 0x76, 0x54, 0x32, 0x10 };

        for (volatile int round = 0; round < 8; round++) {
            for (volatile int i = 0; i < 8; i++) {
                BYTE_JUNK1[i] = (BYTE_JUNK1[i] + BYTE_JUNK2[7 - i]) ^ (round * 0x11);
                BYTE_JUNK2[i] = (BYTE_JUNK2[i] - BYTE_JUNK1[i]) | (i * 0x0F);
            }
        }

        volatile int recursive_junk = 100;
        while (recursive_junk > 0) {
            recursive_junk = recursive_junk - ((recursive_junk % 7) + 1);
        }

        volatile double complex_junk_calc = 0.0;
        for (volatile int n = 1; n <= 50; n++) {
            complex_junk_calc += 1.0 / (n * n);
        }
    }

    void junk_func_g()
    {
        volatile long pointer_like_junk1 = 0x1000;
        volatile long pointer_like_junk2 = 0x2000;
        volatile long pointer_like_junk3 = 0x3000;

        for (volatile int iter = 0; iter < 10; iter++) {
            pointer_like_junk1 = (pointer_like_junk1 << 2) | (pointer_like_junk1 >> 30);
            pointer_like_junk2 = pointer_like_junk2 + (pointer_like_junk3 - pointer_like_junk1);
            pointer_like_junk3 = pointer_like_junk3 ^ pointer_like_junk2;
        }

        volatile enum JunkEnum { RED = 1, GREEN = 2, BLUE = 4, ALPHA = 8 } color_junk = RED;

        for (volatile int mix = 0; mix < 5; mix++) {
            color_junk = static_cast<JunkEnum>(color_junk | GREEN | BLUE);
            color_junk = static_cast<JunkEnum>(color_junk & ~RED);
            color_junk = static_cast<JunkEnum>(color_junk ^ ALPHA);
        }

        volatile int final_junk_result = 0;
        for (volatile int bit = 0; bit < 32; bit++) {
            if ((pointer_like_junk1 >> bit) & 1) {
                final_junk_result += (1 << ((bit + 7) % 32));
            }
        }
    }
}
