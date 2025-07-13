#include <windows.h>
#include <string>
#include <vector>
#include <shlwapi.h>
#include <fstream>

#include "KanzenShuryo.h"
#include "filter.h"

#pragma comment(lib, "shlwapi.lib")

// --- グローバル変数 ---
char g_plugin_name[] = "完全終了";
const char* g_window_class_name = "KanzenShuryoMessageWindow";
HWND g_h_message_window = NULL;
FILTER* g_fp = NULL;

// --- プロトタイプ宣言 ---
LRESULT CALLBACK MessageOnlyWindowProc(HWND, UINT, WPARAM, LPARAM);
void start_helper_process(FILTER* fp);
void write_log(const FILTER* fp, const std::string& message);

// --- 初期化・終了関数 ---
BOOL func_init(FILTER* fp) {
    g_fp = fp;
    HINSTANCE h_inst = fp->dll_hinst;
    WNDCLASSEXA wcex = {};
    wcex.cbSize = sizeof(WNDCLASSEXA);
    wcex.lpfnWndProc = MessageOnlyWindowProc;
    wcex.hInstance = h_inst;
    wcex.lpszClassName = g_window_class_name;
    if (!RegisterClassExA(&wcex)) { return FALSE; }
    g_h_message_window = CreateWindowA(g_window_class_name, "KanzenShuryo Message Handler", 0, 0, 0, 0, 0, HWND_MESSAGE, NULL, h_inst, NULL);
    if (!g_h_message_window) { return FALSE; }
    fp->exfunc->add_menu_item(fp, g_plugin_name, g_h_message_window, ID_MENU_KANZEN_SHURYO, 0, 0);
    return TRUE;
}
BOOL func_exit(FILTER* fp) {
    if (g_h_message_window) { DestroyWindow(g_h_message_window); }
    UnregisterClassA(g_window_class_name, fp->dll_hinst);
    return TRUE;
}

// --- メッセージ専用ウィンドウプロシージャ ---
LRESULT CALLBACK MessageOnlyWindowProc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam) {
    if (message == WM_FILTER_COMMAND && wparam == ID_MENU_KANZEN_SHURYO) {
        if (g_fp) { start_helper_process(g_fp); }
        return 0;
    }
    return DefWindowProc(hwnd, message, wparam, lparam);
}

// --- ヘルパー起動と終了処理 ---
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
    std::wstring command_line = L"\"" + std::wstring(helper_path) + L"\" " + std::to_wstring(pid);
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

// --- ログ書き込み関数 ---
void write_log(const FILTER* fp, const std::string& message) {
    wchar_t dll_path_w[MAX_PATH] = { 0 };
    GetModuleFileNameW(fp->dll_hinst, dll_path_w, MAX_PATH);
    PathRemoveFileSpecW(dll_path_w);
    wchar_t log_dir[MAX_PATH] = { 0 };
    wcscpy_s(log_dir, dll_path_w);
    PathAppendW(log_dir, L"log");
    CreateDirectoryW(log_dir, NULL);
    PathAppendW(log_dir, L"KanzenShuryo.log");
    std::ofstream ofs(log_dir, std::ios::app);
    if (ofs) { ofs << message << std::endl; }
}

// --- フィルタテーブル定義 ---
FILTER_DLL g_filter_dll = {
    FILTER_FLAG_ALWAYS_ACTIVE | FILTER_FLAG_NO_CONFIG,
    0, 0, g_plugin_name, 0, NULL, NULL, NULL, NULL, 0, NULL, NULL,
    NULL, func_init, func_exit, NULL, NULL,
    NULL, NULL, NULL, 0, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL, {0},
};

// --- 必須の空関数定義 ---
extern "C" __declspec(dllexport) FILTER_DLL* GetFilterTable() { return &g_filter_dll; }
BOOL func_proc(FILTER* fp, FILTER_PROC_INFO* fpip) { return FALSE; }
BOOL func_update(FILTER* fp, int status) { return FALSE; }
BOOL func_save_start(FILTER* fp, int s, int e, void* editp) { return TRUE; }
BOOL func_save_end(FILTER* fp, void* editp) { return TRUE; }
BOOL func_is_saveframe(FILTER* fp, void* editp, int saveno, int frame, int fps, int edit_flag, int inter) { return TRUE; }
BOOL func_project_load(FILTER* fp, void* editp, void* data, int size) { return TRUE; }
BOOL func_project_save(FILTER* fp, void* editp, void* data, int* size) { return FALSE; }
BOOL func_modify_title(FILTER* fp, void* editp, int frame, LPSTR title, int max_title) { return FALSE; }