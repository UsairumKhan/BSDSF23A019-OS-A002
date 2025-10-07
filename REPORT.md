# REPORT.md – LS Command Clone (v1.0.0 → v1.6.0)

## Feature 1 – Basic Listing (v1.0.0)
Used `opendir()`, `readdir()`, and `closedir()` to display files in a directory.

## Feature 2 – Long Listing (`-l`) (v1.1.0)
Added file metadata using `stat()` for permissions, owner, group, and modification time.

## Feature 3 – Column Display (v1.2.0)
Formatted output into columns based on terminal width.

## Feature 4 – Horizontal Display (`-x`) (v1.3.0)
Added left-to-right display option.

## Feature 5 – Alphabetical Sort (v1.4.0)
Sorted files alphabetically using `qsort()` and `strcasecmp()`.

## Feature 6 – Colorized Output (v1.5.0)
Used ANSI escape codes to color file names:
| Type | Color | Code |
|------|--------|------|
| Directory | Blue | `\033[1;34m` |
| Executable | Green | `\033[1;32m` |
| Symbolic Link | Cyan | `\033[1;36m` |

**Q1:** How ANSI codes work?  
They start with `\033[` and end with `m`, e.g. `\033[1;32m` for bright green.

**Q2:** How to detect executable files?  
By checking `S_IXUSR`, `S_IXGRP`, or `S_IXOTH` bits in `st_mode`.

## Feature 7 – Recursive Listing (`-R`) (v1.6.0)
Implemented recursion for subdirectories using `S_ISDIR()` and full path construction.

**Q1:** What is the base case?  
When no subdirectories remain or name is `.` or `..`.

**Q2:** Why construct full paths?  
Because calling `do_ls("subdir")` alone would look in the wrong directory.
