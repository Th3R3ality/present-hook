#pragma once
#include <iostream>

uintptr_t VMTEntryHook(uintptr_t VMT, size_t Index, uintptr_t Destination)
{
	uintptr_t* Address = (uintptr_t*)(VMT + Index * sizeof(uintptr_t));

	DWORD OldProtection{ 0 };
	VirtualProtect(Address, sizeof(uintptr_t), PAGE_READWRITE, &OldProtection);
	uintptr_t result = *Address;
	*Address = Destination;
	VirtualProtect(Address, sizeof(uintptr_t), OldProtection, &OldProtection);
	return result;
}