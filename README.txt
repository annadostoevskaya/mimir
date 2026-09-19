Mimir
=====

Lightweight, modular backup engine and orchestrator written in C for Linux.

Overview
--------
Mimir is designed as a fast, low-overhead backup tool and orchestrator. It prioritizes zero external runtime dependencies, explicit memory management using an arena allocator, high-performance zero-copy string slicing, and declarative hierarchical configuration policies.

For the overarching long-term vision and cloud-native architecture concept, see CONCEPT.txt.

Architecture & Current Design
-----------------------------

1. Core Engine & Memory Architecture
   - Low-Level POSIX Foundation: Built using direct Linux system calls (mmap, munmap, open, read, write, fstat) without bulky dependencies.
   - Arena Memory Allocator (mimir_arena.c): Linear memory allocator powered by mmap. Guarantees predictable memory consumption, zero heap fragmentation, fast O(1) allocations, and safe bulk deallocation.
   - Zero-Copy Slicing (struct mimir_slice): String parsing and inspection operate directly on pointer ranges [start, end) without early allocations or in-place buffer mutations.

2. Declarative Policy Configuration
   - Hierarchical INI Policies (policy.ini): Backups are expressed through declarative manifests with hierarchical scoped sections:
     * Root manifests: [policy_name] specifying source, capture, repository, and driver types.
     * Scoped component configurations: [policy_name.component.implementation] providing target-specific options (e.g. [attachments.source.fs], [attachments.driver.rsync]).
   - Fast Indexing & Parsing (mimir_policy_parser.c):
     * Lexer/parser handles section headers, key-value extraction, inline comments (;), and character validation.
     * Builds a flattened sorted index table (mimir_policy_index) for fast lookups and hierarchical policy reconstruction.

3. Modular Backup Pipeline (In Progress)
   - Decoupled Pipeline Stages:
       source -> capture (artifact) -> driver -> repository
     * source: Data origins (local filesystem, databases, snapshots).
     * capture: Preparation and snapshotting (direct mounts, LVM/CSI snapshots).
     * driver: Data movement engine (rsync, with planned support for restic, kopia).
     * repository: Destination storage (local/network filesystem, object storage).

Current Implementation Status
-----------------------------
- [x] POSIX syscall abstractions and basic error logging
- [x] Dynamic arena allocator with mmap/munmap (mimir_arena.c)
- [x] Policy file loader reading into memory buffers (mimir_policy_loader.c)
- [x] Tokenizer, parser, and validator for hierarchical INI policy files (mimir_policy_parser.c)
- [x] Policy index table generation with qsort-based sorting (mimir_policy_parser.c)
- [x] Extraction and parsing of root policy manifests (linux_mimir.c)
- [ ] Complete policy tree object construction from index
- [ ] Driver vtable and execution engine (rsync driver implementation)
- [ ] Capture mechanics and multi-source abstractions

Project Structure
-----------------
- linux_mimir.c          : Main program entry point, slices, memory glue, policy builder
- mimir_arena.c          : Custom mmap-based arena memory allocator
- mimir_memory.c         : Memory abstraction stubs
- mimir_policy_loader.c  : File loader reading policy files into the memory arena
- mimir_policy_parser.c  : Lexer, parser, syntax validator, and index builder for INI policies
- policy.ini             : Sample declarative backup policy definition
- Makefile               : Build recipes
- CONCEPT.txt            : High-level vision and cloud-native orchestrator architecture concept
- TODO                   : Design documentation, architectural notes, and task tracker
- HELP                   : Development environment and editor reference cheat sheet

Building and Running
--------------------

Prerequisites:
- Linux environment
- GCC or Clang
- Make

Build:
  make

Run:
  ./a.out policy.ini

Clean:
  make clean

Author & Contact
----------------
Author: Anna Dostoevskaya (Temirbek Rakhimgalyiev)
Email: iwantknow.aboutjt68h43@gmail.com
