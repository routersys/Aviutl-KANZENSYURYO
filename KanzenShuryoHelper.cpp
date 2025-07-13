#include <windows.h>
#include <string>
#include <fstream>
#include <vector>
#include <shlwapi.h>

#pragma comment(lib, "shlwapi.lib")

// ���O�t�@�C���Ƀ��b�Z�[�W���������ފ֐�
void write_log(const std::string& message) {
    wchar_t exe_path_w[MAX_PATH] = { 0 };
    GetModuleFileNameW(NULL, exe_path_w, MAX_PATH);
    PathRemoveFileSpecW(exe_path_w);

    wchar_t log_dir[MAX_PATH] = { 0 };
    wcscpy_s(log_dir, exe_path_w);
    PathAppendW(log_dir, L"log");
    CreateDirectoryW(log_dir, NULL);

    PathAppendW(log_dir, L"KanzenShuryo.log");

    std::ofstream ofs(log_dir, std::ios::app);
    if (ofs) {
        ofs << message << std::endl;
    }
}

int main(int argc, char* argv[])
{
    write_log("--------------------");
    write_log("[���S�I��] �v���Z�X�J�n");

    if (argc < 2) {
        write_log("[���S�I��] �G���[: ����(PID)���s�����Ă��܂��B");
        return 1;
    }

    DWORD aviutl_pid = 0;
    int timeout_ms = 5000;

    try {
        aviutl_pid = std::stoul(argv[1]);
        write_log("[���S�I��] �Ώ�PID: " + std::to_string(aviutl_pid));
        write_log("[���S�I��] �ҋ@����: " + std::to_string(timeout_ms) + "ms");
    }
    catch (...) {
        write_log("[���S�I��] �G���[: �����̌`�����s���ł��B");
        return 1;
    }

    HANDLE h_aviutl = OpenProcess(SYNCHRONIZE | PROCESS_QUERY_LIMITED_INFORMATION | PROCESS_TERMINATE, FALSE, aviutl_pid);
    if (h_aviutl == NULL) {
        write_log("[���S�I��] �G���[: �v���Z�X�n���h���̃I�[�v���Ɏ��s�B�R�[�h: " + std::to_string(GetLastError()));
        return 1;
    }

    write_log("[���S�I��] �v���Z�X�n���h���擾�����B�I����ҋ@���܂�...");
    DWORD wait_result = WaitForSingleObject(h_aviutl, timeout_ms);

    if (wait_result == WAIT_OBJECT_0) {
        write_log("[���S�I��] �v���Z�X�͐���ɏI�����܂����B");
    }
    else if (wait_result == WAIT_TIMEOUT) {
        write_log("[���S�I��] �v���Z�X���I�����܂���ł��� (�^�C���A�E�g)�B");
        DWORD exit_code;
        if (GetExitCodeProcess(h_aviutl, &exit_code) && exit_code == STILL_ACTIVE) {
            write_log("[���S�I��] �v���Z�X�͂܂��A�N�e�B�u�ł��B�����I�����܂�...");
            if (TerminateProcess(h_aviutl, 0)) {
                write_log("[���S�I��] �����I���ɐ������܂����B");
            }
            else {
                write_log("[���S�I��] �����I���Ɏ��s�B�R�[�h: " + std::to_string(GetLastError()));
            }
        }
    }

    CloseHandle(h_aviutl);
    write_log("[���S�I��] �v���Z�X�I��");

    return 0;
}