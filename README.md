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

### Current State : Loading all the correct file icons

<img width="500" height="250" alt="Screenshot 2026-06-17 102812" src="https://github.com/user-attachments/assets/25ad4fdb-9485-4422-88d9-9b986736f680" />

### Index Manager Window : Handle and choose which files to load and show
<img width="1920" height="1080" alt="image" src="https://github.com/user-attachments/assets/10917805-8deb-454f-80a7-06eaf0f47e8c" />


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
