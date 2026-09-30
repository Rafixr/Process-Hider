# Process-Hider

A lightweight user-mode Windows process hiding tool written in C++ (x64) that hides target processes from Windows Task Manager (`Taskmgr.exe`) and other process monitoring utilities by hooking `NtQuerySystemInformation` (`SystemProcessInformation`).

## 👤 Author

* **Nur Mohammad Rafi** ([@Rafixr](https://github.com/Rafixr))

---

## ⚡ Overview & Features

* **User-Mode API Hooking**: Hooks the native NT API `NtQuerySystemInformation` inside `Taskmgr.exe` via an inline trampoline hook.
* **Process Unlinking**: Intercepts `SystemProcessInformation` queries and unlinks configured process nodes (`NextEntryOffset`) from the returned doubly/singly linked process list so they never appear in the Task Manager UI.
* **Manual Map Injection**: Injects itself into `Taskmgr.exe` using custom reflective section mapping (`nt_allocate_virtual_memory`, `nt_write_virtual_memory`, and `create_remote_thread`).
* **Standalone SDK**: Zero external runtime dependencies; custom lightweight string, heap memory allocator (`RtlCreateHeap` / `RtlAllocateHeap`), string encryption (`xorstr`), and NT API wrappers.
* **Hot-unhook / Unload**: Listens for the `VK_END` key to cleanly restore original bytes, release allocated memory, and terminate safely.

---

## 🚀 How to Use

### 1. Prerequisites & Requirements
* **Operating System**: Windows 10 / Windows 11 (64-bit)
* **Build Tools**: Visual Studio 2022 (with "Desktop development with C++" workload installed)
* **Architecture**: **x64** (Release or Debug)
* **Privileges**: Administrator privileges (required to open and inject into Task Manager)

---

### 2. Configure Processes to Hide (Optional)

By default, the target processes configured to be hidden are defined in [`Process-Hider/process_hider/external/external.cpp`](Process-Hider/process_hider/external/external.cpp#L76-L100):

```cpp
param->process_count = 3;

local_process_list[0] = sdk::wstring( xorstr( L"Process-Hider.exe" ) ).get_data( );
local_process_list[1] = sdk::wstring( xorstr( L"Spotify.exe" ) ).get_data( );
local_process_list[2] = sdk::wstring( xorstr( L"Discord.exe" ) ).get_data( );
```

To hide any other executable (e.g., `notepad.exe`, `cheatengine-x86_64.exe`, your custom apps):
1. Open [`Process-Hider/process_hider/external/external.cpp`](Process-Hider/process_hider/external/external.cpp).
2. Adjust `param->process_count` and modify the process names in `local_process_list`.

---

### 3. Building the Solution

1. Open `Process-Hider.sln` in **Visual Studio 2022**.
2. Set the build configuration to:
   * **Configuration**: `Release` (or `Debug`)
   * **Platform**: `x64`
3. Press `Ctrl + Shift + B` or click **Build > Build Solution**.
4. The output binary will be generated in `x64/Release/Process-Hider.exe` (or `x64/Debug/Process-Hider.exe`).

---

### 4. Running the Tool

1. **Launch Task Manager**:
   * Open Windows Task Manager (`Ctrl + Shift + Esc` or right-click Taskbar > **Task Manager**).
2. **Run as Administrator**:
   * Right-click `Process-Hider.exe` and select **Run as Administrator** (or run from an elevated command prompt).
3. **Execution**:
   * The loader will automatically detect `Taskmgr.exe`, map itself into the Task Manager process space, and install the `NtQuerySystemInformation` inline hook.
   * The specified processes will immediately vanish from Task Manager's process list.
4. **Unhook & Exit**:
   * Press the **`END`** key (`VK_END`) on your keyboard to cleanly remove the hook, restore original function bytes, unload memory, and exit.

---

## 🛠️ Architecture & Workflow

```text
+-----------------------+              +-------------------------------------+
|   Process-Hider.exe   |              |            Taskmgr.exe              |
|  (Elevated Loader)    |              |                                     |
+-----------+-----------+              +------------------+------------------+
            |                                             |
            | 1. OpenProcess(PROCESS_ALL_ACCESS)          |
            | 2. Map Image & Loader Parameters            |
            | 3. CreateRemoteThread(...)                  |
            +-------------------------------------------->|
                                                          | 4. Hook NtQuerySystemInformation
                                                          |    via trampoline hook
                                                          |
                                                          | 5. On SystemProcessInformation query:
                                                          |    Unlink matching process nodes
                                                          |    (Process-Hider, Spotify, Discord)
                                                          |
                                                          | 6. Wait for VK_END key -> Restore & Exit
```

---

## ⚠️ Disclaimer

This project is created strictly for educational, security research, and anti-debugging analysis purposes. It demonstrates internal Windows operating system mechanisms, user-mode API hooking, and PE manipulation.
