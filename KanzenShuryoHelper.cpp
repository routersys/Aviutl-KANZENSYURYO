#include <windows.h>
#include <string>
#include <fstream>
#include <vector>
#include <shlwapi.h>

#pragma comment(lib, "shlwapi.lib")

bool g_save_log_helper = false;

void write_log(const std::string& message) {
    if (!g_save_log_helper) return;

    wchar_t exe_path_w[MAX_PATH] = { 0 };
    GetModuleFileNameW(NULL, exe_path_w, MAX_PATH);
    PathRemoveFileSpecW(exe_path_w);

    PathAppendW(exe_path_w, L"KanzenShuryo.log");

    std::ofstream ofs(exe_path_w, std::ios::app);
    if (ofs) {
        ofs << message << std::endl;
    }
}

int main(int argc, char* argv[])
{
    if (argc < 4) {
        g_save_log_helper = true;
        write_log("[完全終了] エラー: 引数が不足しています。");
        return 1;
    }

    DWORD aviutl_pid = 0;
    int timeout_ms = 5000;

    try {
        aviutl_pid = std::stoul(argv[1]);
        timeout_ms = std::stoi(argv[2]);
        g_save_log_helper = (std::stoi(argv[3]) == 1);
    }
    catch (...) {
        g_save_log_helper = true;
        write_log("[完全終了] エラー: 引数の形式が不正です。");
        return 1;
    }

    write_log("--------------------");
    write_log("[完全終了] プロセス開始");
    write_log("[完全終了] 対象PID: " + std::to_string(aviutl_pid));
    write_log("[完全終了] 待機時間: " + std::to_string(timeout_ms) + "ms");
    write_log("[完全終了] ログ保存: " + std::string(g_save_log_helper ? "有効" : "無効"));

    HANDLE h_aviutl = OpenProcess(SYNCHRONIZE | PROCESS_QUERY_LIMITED_INFORMATION | PROCESS_TERMINATE, FALSE, aviutl_pid);
    if (h_aviutl == NULL) {
        write_log("[完全終了] エラー: プロセスハンドルのオープンに失敗。コード: " + std::to_string(GetLastError()));
        return 1;
    }

    write_log("[完全終了] プロセスハンドル取得成功。終了を待機します...");
    DWORD wait_result = WaitForSingleObject(h_aviutl, timeout_ms);

    if (wait_result == WAIT_OBJECT_0) {
        write_log("[完全終了] プロセスは正常に終了しました。");
    }
    else if (wait_result == WAIT_TIMEOUT) {
        write_log("[完全終了] プロセスが終了しませんでした (タイムアウト)。");
        DWORD exit_code;
        if (GetExitCodeProcess(h_aviutl, &exit_code) && exit_code == STILL_ACTIVE) {
            write_log("[完全終了] プロセスはまだアクティブです。強制終了します...");
            if (TerminateProcess(h_aviutl, 0)) {
                write_log("[完全終了] 強制終了に成功しました。");
            }
            else {
                write_log("[完全終了] 強制終了に失敗。コード: " + std::to_string(GetLastError()));
            }
        }
    }

    CloseHandle(h_aviutl);
    write_log("[完全終了] プロセス終了");

    return 0;
}
