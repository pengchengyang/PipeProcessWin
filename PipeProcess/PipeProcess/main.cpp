#include <iostream>
#include <windows.h>
#include <string>

void writeToPipe(HANDLE hPipe, const std::string& command) {
    DWORD dwWritten;
    if (!WriteFile(hPipe, command.c_str(), command.size(), &dwWritten, NULL)) {
        std::cerr << "Failed to write to pipe." << std::endl;
    }
}

std::string readFromPipe(HANDLE hPipe) {
    DWORD dwRead;
    CHAR chBuf[4096];
    std::string result;

    while (ReadFile(hPipe, chBuf, sizeof(chBuf), &dwRead, NULL) && dwRead > 0) {
        result.append(chBuf, dwRead);
    }

    return result;
}

int main() {
    HANDLE hChildStd_IN_Rd = NULL;
    HANDLE hChildStd_IN_Wr = NULL;
    HANDLE hChildStd_OUT_Rd = NULL;
    HANDLE hChildStd_OUT_Wr = NULL;

    SECURITY_ATTRIBUTES saAttr;
    saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
    saAttr.bInheritHandle = TRUE;
    saAttr.lpSecurityDescriptor = NULL;

    // Create pipes for the child process's STDOUT.
    if (!CreatePipe(&hChildStd_OUT_Rd, &hChildStd_OUT_Wr, &saAttr, 0)) {
        std::cerr << "StdoutRd CreatePipe" << std::endl;
        return 1;
    }

    // Ensure the read handle to the pipe for STDOUT is not inherited.
    if (!SetHandleInformation(hChildStd_OUT_Rd, HANDLE_FLAG_INHERIT, 0)) {
        std::cerr << "Stdout SetHandleInformation" << std::endl;
        return 1;
    }

    // Create pipes for the child process's STDIN.
    if (!CreatePipe(&hChildStd_IN_Rd, &hChildStd_IN_Wr, &saAttr, 0)) {
        std::cerr << "Stdin CreatePipe" << std::endl;
        return 1;
    }

    // Ensure the write handle to the pipe for STDIN is not inherited.
    if (!SetHandleInformation(hChildStd_IN_Wr, HANDLE_FLAG_INHERIT, 0)) {
        std::cerr << "Stdin SetHandleInformation" << std::endl;
        return 1;
    }

    PROCESS_INFORMATION piProcInfo;
    STARTUPINFO siStartInfo;
    bool bSuccess = FALSE;

    // Set up members of the PROCESS_INFORMATION structure.
    ZeroMemory(&piProcInfo, sizeof(PROCESS_INFORMATION));

    // Set up members of the STARTUPINFO structure.
    ZeroMemory(&siStartInfo, sizeof(STARTUPINFO));
    siStartInfo.cb = sizeof(STARTUPINFO);
    siStartInfo.hStdError = hChildStd_OUT_Wr;
    siStartInfo.hStdOutput = hChildStd_OUT_Wr;
    siStartInfo.hStdInput = hChildStd_IN_Rd;
    siStartInfo.dwFlags |= STARTF_USESTDHANDLES;

    // Create the child process.
    std::string cmd = "winscp.com /command \"open ftpes://rws_pcoem:k18h2c%uZ0N7H62@10.22.99.15:52106\"";
    bSuccess = CreateProcess(NULL,
        const_cast<char*>(cmd.c_str()),     // command line
        NULL,          // process security attributes
        NULL,          // primary thread security attributes
        TRUE,          // handles are inherited
        0,             // creation flags
        NULL,          // use parent's environment
        NULL,          // use parent's current directory
        &siStartInfo,  // STARTUPINFO pointer
        &piProcInfo);  // receives PROCESS_INFORMATION

    // If an error occurs, exit the application.
    if (!bSuccess) {
        std::cerr << "CreateProcess failed" << std::endl;
        return 1;
    }

    // Close handles to the child process and its primary thread.
    CloseHandle(piProcInfo.hProcess);
    CloseHandle(piProcInfo.hThread);

    // Close the write end of the pipe before reading from the read end of the pipe.
    CloseHandle(hChildStd_OUT_Wr);

    // Now you can write to the child's input.
    writeToPipe(hChildStd_IN_Wr, "get /config.ini config.ini\n");
    writeToPipe(hChildStd_IN_Wr, "get /log.txt log.txt\n");
    writeToPipe(hChildStd_IN_Wr, "close\n");
    writeToPipe(hChildStd_IN_Wr, "exit\n");

    // Close the write end of the pipe after writing.
    CloseHandle(hChildStd_IN_Wr);

    // Read from the child's output.
    std::string result = readFromPipe(hChildStd_OUT_Rd);
    std::cout << "Output: " << result << std::endl;

    // Close the read end of the pipe.
    CloseHandle(hChildStd_OUT_Rd);

    return 0;
}
