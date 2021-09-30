#include <Windows.h>

extern "C" void SetTrapFlag();

int main(int argc, char* argv[]) {
    __try {
        // �p�G�O�b Debugger ������o�� Function�A
        // Ĳ�o�� Exception �o�S���Q�ۤv�w�q���N�~�B�z�覡�B�z�A
        // ��ܳo�� Exception �w�g�Q Debugger �B�z�F�A�]�N�N��ثe���b�Q Debug
        SetTrapFlag();
        MessageBoxW(0, L"Detect", L"Debugger", 0);
    }
    __except (GetExceptionCode() == EXCEPTION_SINGLE_STEP
        ? EXCEPTION_EXECUTE_HANDLER
        : EXCEPTION_CONTINUE_EXECUTION) {
        MessageBoxW(0, L"Not Detect", L"Debugger", 0);
    }
}
