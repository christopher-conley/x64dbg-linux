#include <stdio.h>
#include <Windows.h>

static void printArgs(const wchar_t* prefix, int argc, wchar_t** argv)
{
    wprintf(L"[%s] argc: %d\n", prefix, argc);
    for(int i = 0; i < argc; i++)
        wprintf(L"[%s] argv[%d]: '%s'\n", prefix, i, argv[i]);
    puts("");
}

int wmain(int argc, wchar_t** argv, wchar_t** envp)
{
    printArgs(L"CRT", argc, argv);

    auto winArgc = 0;
    auto winArgv = CommandLineToArgvW(GetCommandLineW(), &winArgc);
    printArgs(L"WIN", winArgc, winArgv);
    LocalFree(winArgv);

    puts("Press enter to exit...");
    getchar();

    return EXIT_SUCCESS;
}
