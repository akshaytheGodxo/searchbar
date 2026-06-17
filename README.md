# Better Search

A lightweight Windows file launcher and search utility built with C++, ImGui, DirectX 11, and SQLite.

Better Search aims to provide a fast and minimal search experience for opening files and folders directly from the keyboard. The project focuses on responsive UI, efficient indexing, and scalable search performance.

---

## Features

* Fast file and folder search
* Prefix-based searching
* SQLite-backed persistent index
* Lightweight ImGui interface
* Acrylic / glass-style UI effects
* Direct file launching from search results
* Asynchronous index loading
* Modern C++ implementation
* DirectX 11 rendering backend

---

## Screenshots

### Loading State

<img width="500" alt="Loading State" src="https://github.com/user-attachments/assets/f4368bf7-6cda-4ebd-bf2e-6ee1518e243f" />

### Search State

<img width="500" height="250" alt="Screenshot 2026-06-12 144050" src="https://github.com/user-attachments/assets/445c88be-e3ae-44ef-81f2-2fb5d3ccefbb" />


---

## Technology Stack

| Component          | Usage                                  |
| ------------------ | -------------------------------------- |
| C++17/20           | Core application                       |
| ImGui              | User Interface                         |
| DirectX 11         | Rendering                              |
| SQLite             | File indexing database                 |
| nlohmann/json      | Legacy indexing and data serialization |
| Win32 API          | Windowing and system integration       |
| Visual Studio 2022 | Development environment                |

---

## Current Architecture

1. Files are scanned from selected directories.
2. Metadata is stored inside a SQLite database.
3. User input is matched against indexed filenames.
4. Results are displayed instantly in the ImGui interface.
5. Selected files are opened using the Windows Shell API.

---

## Project Goals

The long-term goal is to evolve Better Search into a high-performance Windows search utility capable of indexing and searching millions of files while maintaining a lightweight user experience.

Planned improvements include:

* Full system-wide indexing across all local drives
* Real-time filesystem monitoring
* Automatic index updates for moved, renamed, and deleted files
* Reduced memory consumption
* Improved search ranking
* Background indexing service
* Trie-based in-memory search acceleration
* NTFS USN Journal integration for Everything-style indexing
* Keyboard shortcuts and launcher workflow improvements

---

## Current Progress

### Completed

* [x] SQLite-based indexing
* [x] Acrylic glass UI
* [x] Asynchronous loading
* [x] Direct file launching
* [x] Prefix search implementation
* [x] Persistent file database

### In Progress

* [ ] Full-drive indexing
* [ ] Automatic filesystem synchronization
* [ ] Search result ranking
* [ ] Memory usage optimization
* [ ] Background indexing workers

### Future Work

* [ ] NTFS USN Journal support
* [ ] Real-time indexing
* [ ] Fuzzy search
* [ ] File type filters
* [ ] Recent files support
* [ ] Plugin system

---

## Building

### Requirements

* Windows 10 / Windows 11
* Visual Studio 2022
* DirectX 11 SDK
* SQLite3

### Clone

```bash
git clone https://github.com/akshaytheGodxo/searchbar.git
cd searchbar
```

### Build

Open the solution in Visual Studio 2022 and build using the `x64 Debug` or `x64 Release` configuration.

---

## License

This project is currently under active development.
