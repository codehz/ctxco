#if defined(__i386)
#include "arch/x86.S"
#elif defined(__amd64)
#include "arch/x64.S"
#elif defined(__aarch64__)
#include "arch/arm64.S"
#elif defined(__arm__)
#include "arch/arm.S"
#endif