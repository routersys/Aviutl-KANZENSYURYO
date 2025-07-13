#include <windows.h>
#include <string>
#include <vector>
#include <shlwapi.h>
#include <fstream>

#include "KanzenShuryo.h"
#include "filter.h"

#pragma comment(lib, "shlwapi.lib")

#define ID_MENU_EXECUTE_EXIT 1

char g_plugin_name[] = "完全終了";
char g_menu_name_execute[] = "今すぐ実行";
const char* g_window_class_name = "KanzenShuryoFinalMessageWindow";
HWND g_h_message_window = NULL;
FILTER* g_fp = NULL;

int g_timeout_ms = 5000;
int g_save_log = 1;

char* g_track_name[] = { (char*)"終了待機時間 (ms)" };
int g_track_default[] = { 5000 };
int g_track_s[] = { 1000 };
int g_track_e[] = { 30000 };

char* g_check_name[] = { (char*)"ログを保存する" };
int g_check_default[] = { 1 };

LRESULT CALLBACK MessageOnlyWindowProc(HWND, UINT, WPARAM, LPARAM);
void start_helper_process(FILTER* fp);
void write_log(const FILTER* fp, const std::string& message);

BOOL func_init(FILTER* fp) {
    g_fp = fp;

    g_timeout_ms = fp->exfunc->ini_load_int(fp, "timeout", g_track_default[0]);
    g_save_log = fp->exfunc->ini_load_int(fp, "savelog", g_check_default[0]);
    if (fp->track) fp->track[0] = g_timeout_ms;
    if (fp->check) fp->check[0] = g_save_log;

    HINSTANCE h_inst = fp->dll_hinst;
    WNDCLASSEXA wcex = {};
    wcex.cbSize = sizeof(WNDCLASSEXA);
    wcex.lpfnWndProc = MessageOnlyWindowProc;
    wcex.hInstance = h_inst;
    wcex.lpszClassName = g_window_class_name;
    if (!RegisterClassExA(&wcex)) return FALSE;
    g_h_message_window = CreateWindowA(g_window_class_name, "KanzenShuryo Final Message Handler", 0, 0, 0, 0, 0, HWND_MESSAGE, NULL, h_inst, NULL);
    if (!g_h_message_window) return FALSE;

    fp->exfunc->add_menu_item(fp, g_menu_name_execute, g_h_message_window, ID_MENU_EXECUTE_EXIT, 'Q', ADD_MENU_ITEM_FLAG_KEY_CTRL | ADD_MENU_ITEM_FLAG_KEY_ALT);

    return TRUE;
}

BOOL func_exit(FILTER* fp) {
    if (g_h_message_window) {
        DestroyWindow(g_h_message_window);
        g_h_message_window = NULL;
    }
    UnregisterClassA(g_window_class_name, fp->dll_hinst);
    return TRUE;
}

LRESULT CALLBACK MessageOnlyWindowProc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam) {
    if (message == WM_FILTER_COMMAND && wparam == ID_MENU_EXECUTE_EXIT) {
        if (g_fp) {
            start_helper_process(g_fp);
        }
        return 0;
    }
    return DefWindowProc(hwnd, message, wparam, lparam);
}

BOOL func_WndProc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam, void* editp, FILTER* fp) {
    return FALSE;
}

BOOL func_update(FILTER* fp, int status) {
    if (fp->track) g_timeout_ms = fp->track[0];
    if (fp->check) g_save_log = fp->check[0];

    fp->exfunc->ini_save_int(fp, "timeout", g_timeout_ms);
    fp->exfunc->ini_save_int(fp, "savelog", g_save_log);
    return TRUE;
}

void start_helper_process(FILTER* fp) {
    write_log(fp, "[完全終了プラグイン] 実行開始");
    wchar_t helper_path[MAX_PATH] = { 0 };
    GetModuleFileNameW(fp->dll_hinst, helper_path, MAX_PATH);
    PathRemoveFileSpecW(helper_path);
    PathAppendW(helper_path, L"KanzenShuryoHelper.exe");

    if (!PathFileExistsW(helper_path)) {
        write_log(fp, "[完全終了プラグイン] エラー: 実行ファイルが見つかりません。");
        MessageBoxA(GetAncestor(fp->hwnd, GA_ROOT), "KanzenShuryoHelper.exe が見つかりません。", g_plugin_name, MB_OK | MB_ICONERROR);
        return;
    }

    DWORD pid = GetCurrentProcessId();
    std::wstring command_line = L"\"" + std::wstring(helper_path) + L"\" "
        + std::to_wstring(pid) + L" "
        + std::to_wstring(g_timeout_ms) + L" "
        + std::to_wstring(g_save_log);

    std::vector<wchar_t> command_line_vec(command_line.begin(), command_line.end());
    command_line_vec.push_back(0);

    STARTUPINFOW si = { sizeof(STARTUPINFOW) };
    PROCESS_INFORMATION pi = {};

    if (!CreateProcessW(NULL, command_line_vec.data(), NULL, NULL, FALSE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi)) {
        write_log(fp, "[完全終了プラグイン] エラー: プロセスの起動に失敗。コード: " + std::to_string(GetLastError()));
    }
    else {
        write_log(fp, "[完全終了プラグイン] 起動成功。終了リクエストを送信します。");
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
        HWND hAviutl = GetAncestor(fp->hwnd, GA_ROOT);
        PostMessage(hAviutl, WM_CLOSE, 0, 0);
    }
}

void write_log(const FILTER* fp, const std::string& message) {
    if (!g_save_log) return;

    wchar_t dll_path_w[MAX_PATH] = { 0 };
    GetModuleFileNameW(fp->dll_hinst, dll_path_w, MAX_PATH);
    PathRemoveFileSpecW(dll_path_w);
    PathAppendW(dll_path_w, L"KanzenShuryo.log");

    std::ofstream ofs(dll_path_w, std::ios::app);
    if (ofs) {
        ofs << message << std::endl;
    }
}

FILTER_DLL g_filter_dll = {
    FILTER_FLAG_ALWAYS_ACTIVE,
    0, 0,
    g_plugin_name,
    1, g_track_name, g_track_default, g_track_s, g_track_e,
    1, g_check_name, g_check_default,
    NULL,
    func_init,
    func_exit,
    func_update,
    func_WndProc,
    NULL, NULL, NULL, 0, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, {0, 0},
};

extern "C" __declspec(dllexport) FILTER_DLL* GetFilterTable() { return &g_filter_dll; }
BOOL func_proc(FILTER* fp, FILTER_PROC_INFO* fpip) { return FALSE; }
BOOL func_save_start(FILTER* fp, int s, int e, void* editp) { return TRUE; }
BOOL func_save_end(FILTER* fp, void* editp) { return TRUE; }
BOOL func_is_saveframe(FILTER* fp, void* editp, int saveno, int frame, int fps, int edit_flag, int inter) { return TRUE; }
BOOL func_project_load(FILTER* fp, void* editp, void* data, int size) { return TRUE; }
BOOL func_project_save(FILTER* fp, void* editp, void* data, int* size) { return FALSE; }
BOOL func_modify_title(FILTER* fp, void* editp, int frame, LPSTR title, int max_title) { return FALSE; }
