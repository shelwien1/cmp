// Thread wrapper for Windows
#ifndef THREAD_H
#define THREAD_H

#include "common.h"
#include <windows.h>

// Template wrapper for Win32 thread creation using CRTP (Curiously Recurring Template Pattern)
// Child class must implement: void thread() - the thread's main function
template <class child>
struct thread {

  HANDLE th;  // Windows thread handle

  // Start the thread (calls child's thread() method in new thread)
  uint start( void ) {
    // Create thread with default settings, passing 'this' as parameter
    th = CreateThread( 0, 0, &thread_w, this, 0, 0 );
    return th!=0;  // Return non-zero if successful
  }

  // Wait for thread to finish and cleanup resources
  void quit( void ) {
    WaitForSingleObject( th, INFINITE );  // Block until thread terminates
    CloseHandle( th );  // Release thread handle
  }

  // Static wrapper to redirect to member function (Win32 threads can't call member functions directly)
  static DWORD WINAPI thread_w( LPVOID lpParameter ) {
    // Cast parameter back to child class and call its thread() method
    ((child*)lpParameter)->thread();
    return 0;  // Thread exit code
  }
};

// Sleep for a short duration in thread (10ms - used for polling loops)
inline void thread_wait( void ) {
  Sleep(10);
}

#endif // THREAD_H
