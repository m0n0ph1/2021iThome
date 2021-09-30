#include <Windows.h>

int main(int argc, char* argv[]) {
    CONTEXT ctx = {0};
    ctx.ContextFlags = CONTEXT_DEBUG_REGISTERS;

    // ���o Context ���c
    if (!GetThreadContext(GetCurrentThread(), &ctx))
        return false;

    // �T�{ DR0~DR3 ���S���Q�]�w�A���D 0 �ȥN�� Hardware Breakpoint
    if (ctx.Dr0 || ctx.Dr1 || ctx.Dr2 || ctx.Dr3) {
        MessageBoxW(0, L"Detect", L"Hardware Breakpoint", 0);
    }
    else {
        MessageBoxW(0, L"Not Detect", L"Hardware Breakpoint", 0);
    }
}
