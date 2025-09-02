"""
Example of a multi-threaded Python program demonstrating parallel call stacks.

This program creates exactly 3 threads, each with its own call stack:
1. Thread 1: zzz -> aaa -> bbb -> ccc -> fff -> ggg
2. Thread 2: zzz -> aaa -> bbb -> ccc -> ddd -> eee
3. Thread 3: zzz -> hhh -> iii -> jjj

The program runs for 60 seconds or until Ctrl+C is pressed.
"""

import signal
import threading
from threading import Thread, Event

# Global timeout in seconds
GLOBAL_TIMEOUT = 300  # 1 minute global timeout

# Global event to signal threads to stop
stop_event = Event()

def zzz():
    if hasattr(threading.current_thread(), 'path'):
        fun = None
        if threading.current_thread().path == 1:
            fun = aaa
        elif threading.current_thread().path == 2:
            fun = aaa
        elif threading.current_thread().path == 3:
            fun = hhh
        fun()

def ggg():
    stop_event.wait()

def fff():
    ggg()

def eee():
    stop_event.wait()

def ddd():
    eee()

def ccc():
    if hasattr(threading.current_thread(), 'path'):
        fun = None
        if threading.current_thread().path == 1:
            fun = fff
        elif threading.current_thread().path == 2:
            fun = ddd
        fun()

def bbb():
    ccc()

def aaa():
    bbb()

def jjj():
    stop_event.wait()

def iii():
    jjj()

def hhh():
    iii()

def signal_handler(sig, frame):
    print("\nCtrl+C detected, stopping threads...")
    stop_event.set()

def run_threads():
    """Create and start all three threads with their respective call stacks."""
    threads = []

    # Create and start all three threads
    for i in range(1, 4):
        t = Thread(target=zzz, name=f"Thread-{i}")
        t.path = i
        t.start()
        threads.append(t)

    return threads

def main():
    print("Starting threads...")

    # Set up signal handler for Ctrl+C
    signal.signal(signal.SIGINT, signal_handler)

    # Start all threads
    threads = run_threads()

    # Wait for global timeout or until Ctrl+C
    print(f"Running for {GLOBAL_TIMEOUT} seconds (press Ctrl+C to stop)...")
    if not stop_event.wait(timeout=GLOBAL_TIMEOUT):
        stop_event.set()

    # Wait for all threads to complete
    for t in threads:
        t.join()

    print("All threads finished")

if __name__ == "__main__":
    main()
