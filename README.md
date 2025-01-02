# README

## Overview

This repository is designed for fuzzing syscall handling in Tcons.

## Directory Structure

### `syscall`

- Contains modified system call files originally derived from the LTP (Linux Test Project) repository.
- **Files with the** `**iago**` **prefix**: These files triger mutating system call return values to `4099` during execution, and check if the return value has been changed successfully. This behavior is implemented to simulate Iago attacks for fuzzing purposes.
- **Other files**: These files do not triger mutation but noly triger logging system call chains using eBPF for analysis and monitoring.

### `iago`

- Includes PoC demos for five types of Iago attacks as described in the Emilia paper.
- Demonstrates how system call manipulation can be exploited to undermine application security.

### `mutation`

- Contains the logic responsible for modifying system call return values.

### `syscall_monitor`

- Implements system call capture and monitoring using eBPF.

## 