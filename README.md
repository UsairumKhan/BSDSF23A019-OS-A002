# BSDSF23A019-OS-A002
# LS Command Clone – Programming Assignment 02

## Overview
This project implements a simplified version of the Linux `ls` command in C.  
It was developed step by step from version **v1.0.0** to **v1.6.0**,  
each version introducing a new feature.

## Versions and Features
| Version | Feature | Description |
|----------|----------|-------------|
| **v1.0.0** | Basic Listing | Display all files in the current directory. |
| **v1.1.0** | `-l` Option | Long listing format (permissions, size, owner, etc.). |
| **v1.2.0** | Column Display | Display filenames in multiple columns. |
| **v1.3.0** | Horizontal Display `(-x)` | Prints files left-to-right across the terminal. |
| **v1.4.0** | Alphabetical Sorting | Sorts filenames case-insensitively. |
| **v1.5.0** | Colorized Output | Colors directories, executables, and other types. |
| **v1.6.0** | Recursive Listing `(-R)` | Lists directories recursively. |

## How to Build
```bash
make
