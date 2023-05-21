#include <stdio.h>
#include <windows.h>

// Function to be executed in a separate thread
DWORD WINAPI threadFunction(LPVOID lpParam) {
  int n = *(int*)lpParam;
  for (int i = 0; i < n; i++) {
    printf("%i: Thread function executing\n", i);
  }
  return 0;
}

int main() {
  HANDLE hThread;
  DWORD dwThreadId;

  int n = 5;

  // Create a new thread and start executing the thread function
  hThread = CreateThread(NULL, 0, threadFunction, &n, 0, &dwThreadId);

  // Wait for the thread to finish executing
  WaitForSingleObject(hThread, INFINITE);

  // Main thread continues executing
  printf("Main thread executing\n");

  // Close thread handle
  CloseHandle(hThread);

  return 0;
}
