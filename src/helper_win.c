#include <windows.h>
#include <shellapi.h>

int is_elevated() {
    HANDLE token;
    DWORD size;
    DWORD elevated = 0;

    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &token))
        return 0;

    GetTokenInformation(token, TokenElevation, &elevated, sizeof(elevated), &size);
    CloseHandle(token);
    return elevated;
}

void create_dirs(const char *path) {
    char tmp[MAX_PATH];
    lstrcpyA(tmp, path);

    for (int i = 0; tmp[i]; i++) {
        if (tmp[i] == '\\' || tmp[i] == '/') {
            char saved = tmp[i];
            tmp[i] = 0;
            CreateDirectoryA(tmp, NULL);
            tmp[i] = saved;
        }
    }

    CreateDirectoryA(tmp, NULL);
}

int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrev, LPSTR lpCmd, int nShow) {
    int argc;
    LPWSTR *wargv = CommandLineToArgvW(GetCommandLineW(), &argc);

    // exe src1 dst1 src2 dst2 ...
    if (argc < 3 || (argc % 2) == 0)
        return 1;

    LPWSTR exeW = wargv[0];

    // Elevate if needed
    if (!is_elevated()) {

        // Build wide parameter string
        WCHAR paramsW[4096];
        paramsW[0] = 0;

        for (int i = 1; i < argc; i++) {
            lstrcatW(paramsW, L"\"");
            lstrcatW(paramsW, wargv[i]);
            lstrcatW(paramsW, L"\" ");
        }

        SHELLEXECUTEINFOW sei = { sizeof(sei) };
        sei.fMask = SEE_MASK_NOCLOSEPROCESS;
        sei.lpVerb = L"runas";
        sei.lpFile = exeW;
        sei.lpParameters = paramsW;
        sei.nShow = SW_SHOWNORMAL;

        if (!ShellExecuteExW(&sei))
            return 1;

        WaitForSingleObject(sei.hProcess, INFINITE);
        CloseHandle(sei.hProcess);
        return 0;
    }

    // Elevated: perform all copies
    for (int i = 1; i < argc; i += 2) {
        char src[MAX_PATH], dst[MAX_PATH];

        WideCharToMultiByte(CP_UTF8, 0, wargv[i],   -1, src, sizeof(src), NULL, NULL);
        WideCharToMultiByte(CP_UTF8, 0, wargv[i+1], -1, dst, sizeof(dst), NULL, NULL);

        // Extract directory from dst
        char dstDir[MAX_PATH];
        lstrcpyA(dstDir, dst);

        for (int j = lstrlenA(dstDir) - 1; j >= 0; j--) {
            if (dstDir[j] == '\\' || dstDir[j] == '/') {
                dstDir[j] = 0;
                break;
            }
        }

        create_dirs(dstDir);

        if (!CopyFileA(src, dst, FALSE))
            return 1;
    }

    return 0;
}
