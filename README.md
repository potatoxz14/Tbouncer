# README

## Overview

This repository is designed for experimenting with system call manipulation and fuzzing techniques, particularly those related to Iago attacks. It includes modified system call files and proofs-of-concept (PoC) for various Iago attack scenarios, as described in the Emilia paper. The project enables testing system security by simulating malicious behaviors and capturing system call chains using eBPF.

## Directory Structure

### `syscall`

- Contains modified system call files originally derived from the LTP (Linux Test Project) repository.
- **Files with the** `**iago**` **prefix**: These files alter system call return values to `4099` during execution. This behavior is implemented to simulate Iago attacks for fuzzing purposes.
- **Other files**: These files do not alter system call behaviors but instead log system call chains using eBPF for analysis and monitoring.

### `iago`

- Includes PoC demos for five types of Iago attacks as described in the Emilia paper.
- Demonstrates how system call manipulation can be exploited to undermine application security.

### `mutation`

- Contains the logic responsible for modifying system call return values.
- This directory enables the systematic testing of applications by introducing controlled anomalies in system call responses.

### `syscall_monitor`

- Implements system call capture and monitoring using eBPF.
- This functionality is critical for observing and analyzing system call chains during the experiments.

## 