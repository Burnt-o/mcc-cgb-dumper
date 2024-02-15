#pragma once
#include "pch.h"
#include "MultilevelPointer.h"

// These are the pointer offsets that need to be updated when MCC gets updated so the cgb still works

// CustomGameRefresher.h
// Pointer to OrigCallRefresh function (code). We will call this function directly.
/*
 To Find:
 Use cheat engine to scan for array-of-bytes non-writeable "80 bf e8 02 00 00 00 74 71 c6 87 e8 02 00 00 00". Disassemble. Scroll up a few instructions to the top of the function. The instruction at the beginning of the function is our base address / the function pointer.
  Confirm by setting a breakpoint. Should get hit when you click refresh or load into the CGB from the main menu.
*/
const static inline MultilevelPointer mlp_OrigCallRefreshFunction{ { 0xA768B8 } };  // old 3272 ptr: 0xA76708

// AutoDumper.cpp
// The beginning of the array of CGB objects.
/*
To Find:
Put a breakpoint on mlp_OrigUpdateCustomGameArray. Hit by refreshing CGB. RBX will contain the address of the CustomGameInfoArrayStart, so copy it to the address list. 
Right click -> "Pointer scan for this address". Untick "Don't include pointers with read-only nodes". Set Max level to 2. Click ok. Should be one result relative to exe, thats it.
*/
const static inline MultilevelPointer mlp_CustomGameInfoArrayStart{ { 0x03F629A0, 0x28, 0x0 } }; // old 3272 ptr: 0x03F639A0, 0x40, 0x0

// Autodumper.h
// Pointer to original MCC_UpdateCustomGameArray function (code). We put a hook on this to know when to dump the CGB data (ie it's just been refreshed).
/*
 To Find:
 Use cheat engine to scan for array-of-bytes non-writeable "bf f0 00 00 00 83 bd 58 02 00 00 01". Disassemble. Scroll up a few instructions to the top of the function. The instruction at the beginning of the function is our base address / the function pointer.
 Confirm by setting a breakpoint. Should get hit when you click refresh or load into the CGB from the main menu.
*/
const MultilevelPointer mlp_OrigUpdateCustomGameArray{ { 0x27A4E0 } }; // old 3272 ptr:  0x279FD0