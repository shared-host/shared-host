# shared-host

A high-performance, ultra-low-latency Inter-Process Communication (IPC) library written in C. `shared-host` leverages shared memory ring buffers for fast, reliable message passing between processes.

Basically a faster localhost-like communication method.

---

## Ai Usage Guidelines

- Ai in this project was only used for documentation testing and external tools.
- Everything in the `src` and `include` folders was written by me without ai assistance.

---

## Performance Statistics

### 1. High-Throughput & Bandwidth Benchmarks

#### FAST Mode (Spin-Lock Polling)
```text
  -> Min Latency:    0.0 ns
  -> Avg Latency:    46.9 ns
  -> P50 (Median):   0.0 ns
  -> P99 Latency:    900.0 ns
  -> P99.9 Latency:  1300.0 ns
  -> Max Latency:    116800.0 ns

 Payload |    Throughput |   Payload BW |      Wire BW |   Avg Latency
---------+---------------+--------------+--------------+--------------
     64B |  22068235/s |  1346.94 MB/s |  1683.67 MB/s |     46.9 ns
    256B |   8581620/s |  2095.12 MB/s |  2226.07 MB/s |    116.5 ns
   1024B |   3059296/s |  2987.59 MB/s |  3034.28 MB/s |    326.9 ns
   4096B |   1027967/s |  4015.50 MB/s |  4031.18 MB/s |    972.8 ns
  16384B |    239857/s |  3747.77 MB/s |  3751.43 MB/s |   4169.1 ns
  65536B |    212959/s | 13309.95 MB/s | 13313.20 MB/s |   4695.8 ns

=================================================================
```

#### SLOW Mode (Win32 Event Polling / Signaling)
```text
  -> Min Latency:    0.0 ns
  -> Avg Latency:    345.4 ns
  -> P50 (Median):   100.0 ns
  -> P99 Latency:    4400.0 ns
  -> P99.9 Latency:  8100.0 ns
  -> Max Latency:    246800.0 ns

 Payload |    Throughput |   Payload BW |      Wire BW |   Avg Latency
---------+---------------+--------------+--------------+--------------
     64B |  20340086/s |  1241.46 MB/s |  1551.83 MB/s |     49.2 ns
    256B |   3358477/s |   819.94 MB/s |   871.19 MB/s |    434.0 ns
   1024B |   1404096/s |  1371.19 MB/s |  1392.61 MB/s |    780.2 ns
   4096B |    525950/s |  2054.49 MB/s |  2062.52 MB/s |   1901.3 ns
  16384B |    230770/s |  3605.78 MB/s |  3609.30 MB/s |   4333.3 ns
  65536B |    148696/s |  9293.48 MB/s |  9295.75 MB/s |   6725.1 ns

=================================================================
```

### 2. Mode Comparison Summary

| Connection Mode | Feature | Avg Latency | Peak Throughput (64B) | Peak Bandwidth (64KB) | Integrity |
| :--- | :--- | :--- | :--- | :--- | :--- |
| **FAST (Zero-Copy)** | Spin-Lock Polling | **46.9 ns** (P99: 900.0 ns) | **22.06 Million ops/sec** | **13.31 GB/s** | **PASSED (100% Valid)** |
| **FAST (Standard)** | Spin-Lock Polling | 67.6 ns (P99: 1000.0 ns) | 15.23 Million ops/sec | 11.21 GB/s | **PASSED (100% Valid)** |
| **SLOW (Zero-Copy)** | Event-Based Polling | **345.4 ns** (P99: 4400.0 ns) | **20.34 Million ops/sec** | **9.29 GB/s** | **PASSED (100% Valid)** |
| **SLOW (Standard)** | Event-Based Polling | 362.8 ns (P99: 5600.0 ns) | 15.72 Million ops/sec | 9.79 GB/s | **PASSED (100% Valid)** |

### 3. Per-Function Execution Times (Individual C Function Latencies)

| Function | Mode / Type | FAST (Spin-Lock) | SLOW (Event-Polling) | Description |
| :--- | :--- | :--- | :--- | :--- |
| `claim_from_shared_host_connection` | Zero-Copy Writer | **39.1 ns** | **39.1 ns** | Claims shared memory buffer for writing |
| `commit_to_shared_host_connection` | Zero-Copy Writer | **17.9 ns** | **305.3 ns** (Signals Win32 Event) | Commits packet to ring buffer |
| `receive_from_shared_host_connection` | Zero-Copy Reader | **36.8 ns** | **36.8 ns** | Retrieves zero-copy buffer pointer |
| `release_to_shared_host_connection` | Zero-Copy Reader | **17.8 ns** | **17.8 ns** | Advances ring buffer read offset |
| `write_to_shared_host_connection` | Standard Writer | **52.8 ns** | **361.3 ns** (Signals Win32 Event) | Standard copy-based message write |
| `read_from_shared_host_connection` | Standard Reader | **67.0 ns** | **67.0 ns** | Standard copy-based message read |

---

## Features

- **Ultra-Low Latency**: Sub-50ns message delivery (**46.9 ns** average latency in Zero-Copy mode).
- **High Throughput**: Exceeds **22.06 Million ops/sec** on 64B payloads and **13.31 GB/s** bandwidth on 64KB payloads.
- **Configurable Ring Buffer Sizes**: Custom shared memory buffer sizes per channel (e.g. `1 SH_GB`, `100 MB`, `1 MB`) passed directly to `create_shared_host_connection`.
- **First-Class Zero-Copy Method Pointers**: Connection struct provides direct function pointers (`claim`, `commit`, `receive`, `release`, `read`, `write`) bound automatically upon connection creation.
- **Zero-Copy Architecture**: Writer-side `claim`/`commit` and Reader-side `receive`/`release` eliminate heap allocations (`malloc`) and data copies (`memcpy`) on both ends.
- **Zero Corruption Guarantee**: Includes boundary wrap validation and sequence tracking.
- **Clean C API**: Supports both standard copy-based (`write`/`read`) and zero-copy (`claim`/`commit`/`receive`/`release`) interfaces.
- **Cross-Platform Makefile**: Supports building shared libraries (`.dll` / `.so`) and test/benchmark suites.

---

## Building & Usage

### Using the Makefile

| Target | Description |
| :--- | :--- |
| `make all` | Builds DLL (`build/shared-host.dll`) and test/benchmark binaries |
| `make dll` | Compiles shared library (`build/shared-host.dll`) |
| `make test` | Builds test binary (`build/test_main.exe`) |
| `make benchmark` | Builds benchmark binary (`build/benchmark_main.exe`) |
| `make run-test` | Builds and runs the test suite |
| `make run-benchmark` | Builds and runs the benchmark suite |
| `make clean` | Removes `obj/` and `build/` directories |
| `make help` | Displays list of Makefile targets |

### Using Helper Scripts

Batch scripts (Windows) and shell scripts (Linux/macOS) are located in `scripts/`:

- **Windows Batch**: `scripts\all.bat`, `scripts\make.bat`, `scripts\test.bat`, `scripts\benchmark.bat`
- **Shell**: `./scripts/all.sh`, `./scripts/make.sh`, `./scripts/test.sh`, `./scripts/benchmark.sh`

---

## API Overview

Header file: `#include <shared_host.h>`

### 1. Standard API (Copy-Based)

```c
#include <stdio.h>
#include <stdlib.h>
#include <shared_host.h>

// 1. Create a server host connection with configurable ring buffer size (e.g., 1 GB or 10 MB)
shared_host_connection server_conn;
sh_result_t err = create_shared_host_connection("my_channel", 1 SH_GB, SH_FAST_CONNECTION, &server_conn);

// 2. Connect client to host
shared_host_connection client_conn;
size_t conn_size = 0;
err = connect_to_shared_host_connection("my_channel", &conn_size, &client_conn);

// 3. Write data from client (using helper or direct struct method pointer client_conn.write)
char data[] = "High speed IPC payload";
write_to_shared_host_connection(&client_conn, data, sizeof(data));
// Or: client_conn.write(&client_conn, data, sizeof(data));

// 4. Read data on server
void* read_buffer = NULL;
size_t read_bytes = 0;
if (read_from_shared_host_connection(&server_conn, &read_buffer, &read_bytes) == SH_OK) {
    // Process read_buffer...
    free(read_buffer);
}

// 5. Close connections
close_shared_host_connection(&server_conn);
close_shared_host_connection(&client_conn);
```

### 2. Zero-Copy API (Direct Shared Memory Access - No Malloc / No Memcpy)

The Zero-Copy API allows producers to claim ring buffer memory directly and consumers to read shared memory pointers without intermediate heap allocations or copies. Zero-copy function pointers (`claim`, `commit`, `receive`, `release`) are automatically attached to the `shared_host_connection` struct.

```c
#include <stdio.h>
#include <shared_host.h>

// Producer / Writer: Claim shared memory buffer & Commit
void *tx_buf = NULL;
size_t payload_size = 256;

if (client_conn.claim(&client_conn, &tx_buf, payload_size) == SH_OK) {
    // Write directly into tx_buf (mapped shared memory ring buffer)
    snprintf((char*)tx_buf, payload_size, "Zero-Copy IPC Payload");

    // Commit to make message visible to reader (fast or event-signaled depending on connection flags)
    client_conn.commit(&client_conn);
}

// Consumer / Reader: Receive zero-copy buffer & Release
void *rx_buf = NULL;
size_t rx_size = 0;

if (server_conn.receive(&server_conn, &rx_buf, &rx_size) == SH_OK) {
    // Access rx_buf directly in shared memory without heap allocation
    printf("Received %zu bytes: %s\n", rx_size, (char*)rx_buf);

    // Release buffer to advance reader offset
    server_conn.release(&server_conn);
}
```

---

## Repository Structure

```text
shared-host/
├── include/
│   ├── shared_host.h              # Public C API header
│   └── internal/                  # Internal connection and mapping headers
├── src/
│   ├── shared_host_core.c         # Main IPC connection and buffer logic
│   ├── shared_host_read.c         # Copy-based read functions
│   ├── shared_host_write.c        # Copy-based write functions
│   ├── shared_host_claim.c        # Zero-copy writer claim functions
│   ├── shared_host_commit.c       # Zero-copy writer commit functions
│   ├── shared_host_receive.c      # Zero-copy reader receive functions
│   ├── shared_host_release.c      # Zero-copy reader release functions
│   └── shm_operations/
│       └── shm_mapping.c          # OS shared memory mapping implementations
├── tests/
│   └── benchmark.c                # Comprehensive test and benchmark suite
├── scripts/                       # Wrapper scripts (.bat, .cmd, .sh)
├── Makefile                       # Cross-platform GNU Makefile
└── README.md                      # Documentation and performance stats
```

---

## License

This library is licensed under the LGPL v3.0 License. See [LICENSE](LICENSE) for details.
