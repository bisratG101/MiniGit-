# MiniGit-

**MiniGit** is a lightweight, command-line version control system inspired by Git.  
It allows users to track changes, commit files, create branches, merge, and view commit history — all implemented in C++ from scratch without external libraries.

## Features
- Initialize a repository (`init`)
- Stage files (`add <filename>`)
- Commit changes (`commit <message>`)
- View commit history (`log`)
- Create and switch branches (`branch` / `checkout`)
- Merge branches with conflict detection (`merge`)
- Compare changes between commits (`diff`)

## Purpose
The project was designed to help understand **how Git works internally**, including file tracking, hashing, commit history, and branching, using only standard C++ and file-based storage.
