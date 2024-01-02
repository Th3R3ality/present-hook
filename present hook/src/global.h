#pragma once
extern bool ejecting;
extern bool presentReset;

#define safe_release(p) if (p) { p->Release(); p = nullptr; }

#include <iostream>
extern bool doDebugPrint;
#define debugPrint(x) if (doDebugPrint == true) std::cout << #x << " : " << x << "\n"
#define debugPrintCast(x) debugPrint((uintptr_t)x)