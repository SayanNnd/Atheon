# Atheon
A zero-dependency version control system built from scratch in C++17.

Atheon re-implements the core ideas behind Git: content-addressed storage, a staging index, tree and commit objects, and symbolic refs. It uses only the C++ standard library with a SHA-256 generator written from scratch. I built it to understand how a VCS works under the hood.

---

## Features

- **Content-addressed object store**:- blobs, trees, and commits, each named by its SHA-256 hash
- **Staging area**:- with an index file that tracks hash, size, and modification time
- **Multi-threaded file hashing**:- uses multi-threading and 128 KB read buffers, so large files never sit fully in memory
- **Commit history**:- with parent links, forming a DAG of commits
- **Status reporting**:- that compares the working directory, the index, and the last commit
- **Checkout**:- of any commit hash or branch name
- **Ignore rules**:- through a `.atheonignore` file

---

## Build

Requirements: CMake and a C++17 compiler (GCC, Clang, or MSVC).

```bash
git clone https://github.com/SayanNnd/Atheon-Mini-VCS.git
cd Atheon-Mini-VCS
```
 
**Windows (MinGW-w64):**
 
```powershell
cmake -S . -B build -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_COMPILER=g++
cmake --build build
.\build\atheon.exe help
```

**Linux / macOS (Makefiles or Ninja):**
 
```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
./build/atheon help
```
 
> Developed and tested on Windows. Linux and macOS builds have not been verified yet.

---

## Usage

Run `atheon` with no arguments to open the interactive menu, or pass a command directly:

| Command | Description |
|---|---|
| `atheon vault` | Initialize a repository in the current directory |
| `atheon conflux <file \| .>` | Stage one file, or everything with `.` |
| `atheon anchor -m "<message>"` | Commit the staged files |
| `atheon timestream` | Show commit history |
| `atheon oracle` | Show staged and unstaged changes |
| `atheon timegate <commit \| branch>` | Restore the working tree to a commit or branch |
| `atheon help` | Show usage |

### Example session

```bash
atheon vault                        # creates .atheon/
atheon conflux .                    # stage everything not ignored
atheon oracle                       # see what is staged
atheon anchor -m "first commit"     # commit
atheon timestream                   # view history
atheon timegate <commit-hash>       # go back in time
atheon timegate alpha               # return to the tip of the branch
```

Checking out a commit hash puts the repository in a detached state, and staging and committing are blocked until you return to a branch. If the working tree has uncommitted changes, `timegate` asks before overwriting them.

### Ignoring files

Create a `.atheonignore` file in the project root. Lines starting with `#` or `//` are comments.

```
build/          # ignore a directory
*.exe           # ignore by extension
secrets.txt     # ignore a specific file
```

---

## How it works

Everything lives in a `.atheon/` directory:

```
.atheon/
├── HEAD              # "ref: refs/heads/alpha", or a raw commit hash when detached
├── index             # staging area, one line per file: <hash> <size> <mtime> <path>
├── refs/heads/       # one file per branch, containing a commit hash
└── objects/          # content-addressed store, fanned out as objects/ab/cdef...
```

**Objects** are stored under the first two hex characters of their hash, then the remaining 62:

| Object | Contents |
|---|---|
| Blob | `blob <size>\0` followed by the raw file bytes. Its name is the SHA-256 of the file contents. |
| Tree | One line per entry: `<mode> <name>\0<hash>\n`, sorted by name. Subdirectories are nested trees. |
| Commit | `tree <hash>`, an optional `parent <hash>`, `timestamp <unix>`, and `message <text>` |

**Staging** hashes files in parallel. Worker threads claim file indices from an atomic counter, then blobs are written for any file whose content is new or changed. The index is written to a temporary file and renamed into place, so an interrupted run cannot leave a half-written index.

**Status** avoids needless hashing. A file is only re-hashed when its size or modification time differs from the index entry; if the hash still matches, it is considered unchanged.

**Commit** builds a tree hierarchy from the flat index, writes tree objects bottom-up (a child's hash must be known before its parent's), then writes the commit object and advances the branch ref. If the new root tree matches the previous commit's, nothing is committed.

---

## Project layout

```
include/Atheon/   public headers
src/
├── main.cpp        CLI parsing and interactive menu
├── vault.cpp       repository initialization
├── conflux.cpp     staging and parallel hashing
├── anchor.cpp      tree and commit creation
├── oracle.cpp      status
├── timestream.cpp  history
├── timegate.cpp    checkout
├── ignore.cpp      .atheonignore rules
└── sha256.cpp      SHA-256 implementation
```

---

## Current limitations

- Only the default branch (`alpha`) is created automatically, and there is no branch-creation command yet
- No merge, diff, or unstaging
- Objects are stored uncompressed
- All files are recorded with a single file mode (no executable bit or symlinks)
- Commits do not record an author yet

## Roadmap (V2)

- `cleanse`: remove untracked files
- Unstage files
- Branch creation, listing, and switching
- Commit authors
- File diffing
- Compressed object storage
- Parallelism in checkout

## Related projects

- [SHA-256 Generator](https://github.com/SayanNnd/SHA-256-Generator): the standalone hashing tool whose implementation Atheon builds on.
  
---

## Author

**Sayan Nandi** · [@SayanNnd](https://github.com/SayanNnd)

## License

MIT. See `LICENSE`.
