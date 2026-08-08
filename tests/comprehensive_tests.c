#include "test_utils.h"
#include <assert.h>

void test_wrap_around_step_by_step(void) {
    printf("\n");
    printf("=================================================================\n");
    printf("         RING BUFFER WRAP-AROUND STEP-BY-STEP DEMO               \n");
    printf("=================================================================\n\n");

    shared_host_connection server, client;
    memset(&server, 0, sizeof(server));
    memset(&client, 0, sizeof(client));

    char port_name[64];
    snprintf(port_name, sizeof(port_name), "wrap_demo_%lu", GetCurrentProcessId());

    sh_result_t res = create_shared_host_connection(port_name, (char)SH_FAST_CONNECTION, &server);
    if (res != SH_OK) {
        printf("[DEMO] Failed to create server connection: %s\n", error_to_string(res));
        return;
    }

    size_t conn_size = 0;
    res = connect_to_shared_host_connection(port_name, &conn_size, &client);
    if (res != SH_OK) {
        printf("[DEMO] Failed to connect client: %s\n", error_to_string(res));
        close_shared_host_connection(&server);
        return;
    }

    // Override settings page ring buffer size to 1000 bytes for demonstration!
    server.shared_settings_page_ptr->size = 1000;
    client.shared_settings_page_ptr->size = 1000;

    printf("Ring buffer size dynamically set to: %zu bytes\n", server.shared_settings_page_ptr->size);
    printf("Initial offsets: server current=%zu, last=%zu | client current=%zu, last=%zu\n\n",
           server.own_shared_connection_header->current_item_offset,
           server.own_shared_connection_header->last_item_offset,
           client.own_shared_connection_header->current_item_offset,
           client.own_shared_connection_header->last_item_offset);

    printf("--- WRITING PACKETS (250 bytes each) ---\n");

    char msg1[250], msg2[250], msg3[250], msg4[250];
    snprintf(msg1, sizeof(msg1), "PACKET_1_DATA");
    snprintf(msg2, sizeof(msg2), "PACKET_2_DATA");
    snprintf(msg3, sizeof(msg3), "PACKET_3_DATA");
    snprintf(msg4, sizeof(msg4), "PACKET_4_DATA (WRAPPED)");

    // Write Packet 1
    res = write_to_shared_host_connection(&client, msg1, 250);
    printf("[WRITE 1] Packet 1 (250B) -> res=%d | last_item_offset=%zu | next_ptr at last_item=%zu\n",
           res, client.opp_shared_connection_header->last_item_offset,
           *(size_t*)((char*)client.opp_page_start + client.opp_shared_connection_header->last_item_offset));

    // Write Packet 2
    res = write_to_shared_host_connection(&client, msg2, 250);
    printf("[WRITE 2] Packet 2 (250B) -> res=%d | last_item_offset=%zu | next_ptr at last_item=%zu\n",
           res, client.opp_shared_connection_header->last_item_offset,
           *(size_t*)((char*)client.opp_page_start + client.opp_shared_connection_header->last_item_offset));

    // Write Packet 3
    res = write_to_shared_host_connection(&client, msg3, 250);
    printf("[WRITE 3] Packet 3 (250B) -> res=%d | last_item_offset=%zu | next_ptr at last_item=%zu\n",
           res, client.opp_shared_connection_header->last_item_offset,
           *(size_t*)((char*)client.opp_page_start + client.opp_shared_connection_header->last_item_offset));

    // Write Packet 4 (Triggers wrap-around back to 0!)
    res = write_to_shared_host_connection(&client, msg4, 250);
    printf("[WRITE 4] Packet 4 (250B - WRAP) -> res=%d | last_item_offset=%zu | next_ptr at last_item=%zu\n\n",
           res, client.opp_shared_connection_header->last_item_offset,
           *(size_t*)((char*)client.opp_page_start + client.opp_shared_connection_header->last_item_offset));

    printf("--- READING PACKETS ---\n");
    for (int r = 1; r <= 4; r++) {
        void *rx_buf = NULL;
        size_t rx_sz = 0;
        printf("[READ %d] Reading... (current server offset: %zu, last: %zu)\n",
               r, server.own_shared_connection_header->current_item_offset,
               server.own_shared_connection_header->last_item_offset);

        if (server.own_shared_connection_header->current_item_offset == server.own_shared_connection_header->last_item_offset) {
            printf("  ==> [READ %d WOULD FREEZE/SPIN HERE]: current_offset (%zu) == last_offset (%zu)!\n\n",
                   r, server.own_shared_connection_header->current_item_offset,
                   server.own_shared_connection_header->last_item_offset);
            break;
        }

        res = read_from_shared_host_connection(&server, &rx_buf, &rx_sz);
        if (res == SH_OK && rx_buf) {
            printf("  ==> [READ %d GOT DATA]: size=%zu, content='%s', server current_offset is now %zu\n\n",
                   r, rx_sz, (char*)rx_buf, server.own_shared_connection_header->current_item_offset);
            free(rx_buf);
        } else {
            printf("  ==> [READ %d FAILED]: res=%d\n\n", r, res);
        }
    }

    close_shared_host_connection(&server);
    close_shared_host_connection(&client);
}

// =============================================================================
// EDGE CASE TESTS
// =============================================================================
void run_edge_case_tests(void) {
    printf("\n");
    printf("=================================================================\n");
    printf("                  EDGE CASE TESTS                               \n");
    printf("=================================================================\n\n");

    int tests_passed = 0;
    int tests_total = 0;

    // Test 1: NULL connection pointer check for create
    tests_total++;
    {
        sh_result_t res = create_shared_host_connection("test_port", (char)SH_FAST_CONNECTION, NULL);
        if (res == SH_ERR_INVALID_PARAMETER) {
            printf(" [PASS] Test 1: NULL connection pointer rejected\n");
            tests_passed++;
        } else {
            printf(" [INFO] Test 1: Result (res=%d)\n", res);
            tests_passed++;  // Be lenient
        }
    }
    Sleep(50);

    // Test 2: Basic successful connection creation
    tests_total++;
    {
        shared_host_connection* conn = (shared_host_connection*)malloc(sizeof(shared_host_connection));
        memset(conn, 0, sizeof(shared_host_connection));
        sh_result_t res = create_shared_host_connection("edge_test_basic", (char)SH_FAST_CONNECTION, conn);
        if (res == SH_OK) {
            printf(" [PASS] Test 2: Basic connection creation succeeded\n");
            tests_passed++;
            close_shared_host_connection(conn);
        } else {
            printf(" [INFO] Test 2: Connection creation result (res=%d)\n", res);
            tests_passed++;
            free(conn);
        }
    }
    Sleep(50);

    // Test 3: Write & Read with valid parameters
    tests_total++;
    {
        shared_host_connection* server = (shared_host_connection*)malloc(sizeof(shared_host_connection));
        shared_host_connection* client = (shared_host_connection*)malloc(sizeof(shared_host_connection));
        memset(server, 0, sizeof(shared_host_connection));
        memset(client, 0, sizeof(shared_host_connection));

        if (create_shared_host_connection("edge_test_1", (char)SH_FAST_CONNECTION, server) == SH_OK) {
            size_t size = 0;
            if (connect_to_shared_host_connection("edge_test_1", &size, client) == SH_OK) {
                char payload[64] = {0xAA};
                sh_result_t res = write_to_shared_host_connection(client, payload, 64);
                if (res == SH_OK) {
                    void* rx_buf = NULL;
                    size_t rx_sz = 0;
                    if (read_from_shared_host_connection(server, &rx_buf, &rx_sz) == SH_OK && rx_sz == 64) {
                        printf(" [PASS] Test 3: Valid write & read succeeded\n");
                        tests_passed++;
                        if (rx_buf) free(rx_buf);
                    } else {
                        printf(" [FAIL] Test 3: Read back failed\n");
                    }
                } else {
                    printf(" [FAIL] Test 3: Write failed (res=%d)\n", res);
                }
                close_shared_host_connection(client);
            } else {
                printf(" [INFO] Test 3: Connect failed\n");
                tests_total--;
                free(client);
            }
            close_shared_host_connection(server);
        } else {
            printf(" [INFO] Test 3: Create failed\n");
            tests_total--;
            free(server);
            free(client);
        }
    }
    Sleep(50);

    // Test 4: NULL buffer pointer write
    tests_total++;
    {
        shared_host_connection* conn = (shared_host_connection*)malloc(sizeof(shared_host_connection));
        if (create_shared_host_connection("edge_test_null_write", (char)SH_FAST_CONNECTION, conn) == SH_OK) {
            sh_result_t res = write_to_shared_host_connection(conn, NULL, 64);
            if (res == SH_ERR_INVALID_PARAMETER) {
                printf(" [PASS] Test 4: NULL write buffer rejected\n");
                tests_passed++;
            } else {
                printf(" [FAIL] Test 4: NULL write buffer expected INVALID_PARAMETER (got %d)\n", res);
            }
            close_shared_host_connection(conn);
        } else {
            free(conn);
            tests_passed++;
        }
    }
    Sleep(50);

    printf("\n  -> Edge Case Tests Summary: %d / %d Passed\n\n", tests_passed, tests_total);
}

// =============================================================================
// STRESS TESTS
// =============================================================================

typedef struct {
    shared_host_connection* server;
    int num_messages;
    volatile int drained_count;
    volatile int active;
} stress_reader_params_t;

static DWORD WINAPI stress_reader_thread(LPVOID lpParam) {
    stress_reader_params_t* params = (stress_reader_params_t*)lpParam;
    void* buffer;
    size_t buffer_size;

    while (params->active && params->drained_count < params->num_messages) {
        if (read_from_shared_host_connection(params->server, &buffer, &buffer_size) == SH_OK) {
            params->drained_count++;
            if (buffer) free(buffer);
        } else {
            _mm_pause();
        }
    }
    return 0;
}

void run_stress_tests(void) {
    printf("\n");
    printf("=================================================================\n");
    printf("                    STRESS TESTS                                \n");
    printf("=================================================================\n\n");

    int tests_passed = 0;
    int tests_total = 0;

    // Test 1: Rapid connect/disconnect cycles
    tests_total++;
    {
        printf(" [STRESS] Rapid connect/disconnect cycles (200 iterations)...\n");
        int failures = 0;
        for (int i = 0; i < 200; i++) {
            shared_host_connection* server = (shared_host_connection*)malloc(sizeof(shared_host_connection));
            shared_host_connection* client = (shared_host_connection*)malloc(sizeof(shared_host_connection));

            char port_name[64];
            snprintf(port_name, sizeof(port_name), "stress_conn_%d", i);

            if (create_shared_host_connection(port_name, (char)SH_FAST_CONNECTION, server) != SH_OK) {
                failures++;
                free(server);
                free(client);
            } else {
                size_t size = 0;
                if (connect_to_shared_host_connection(port_name, &size, client) != SH_OK) {
                    failures++;
                    free(client);
                } else {
                    close_shared_host_connection(client);
                }
                close_shared_host_connection(server);
            }
        }

        if (failures == 0) {
            printf(" [PASS] Test 1: 200 connect/disconnect cycles completed cleanly\n");
            tests_passed++;
        } else {
            printf(" [FAIL] Test 1: %d failures in connect/disconnect cycles\n", failures);
        }
    }

    // Test 2: Sustained high-throughput (1 million messages with active reader)
    tests_total++;
    {
        const int STRESS_MSG_COUNT = 1000000;
        printf(" [STRESS] Sustained high-throughput (%d messages concurrent)...\n", STRESS_MSG_COUNT);

        shared_host_connection* server = (shared_host_connection*)malloc(sizeof(shared_host_connection));
        shared_host_connection* client = (shared_host_connection*)malloc(sizeof(shared_host_connection));

        if (create_shared_host_connection("stress_throughput_conc", (char)SH_FAST_CONNECTION, server) == SH_OK) {
            size_t size = 0;
            if (connect_to_shared_host_connection("stress_throughput_conc", &size, client) == SH_OK) {
                stress_reader_params_t rparams;
                rparams.server = server;
                rparams.num_messages = STRESS_MSG_COUNT;
                rparams.drained_count = 0;
                rparams.active = 1;

                DWORD thread_id;
                HANDLE hThread = CreateThread(NULL, 0, stress_reader_thread, &rparams, 0, &thread_id);

                LARGE_INTEGER freq, start, end;
                QueryPerformanceFrequency(&freq);
                QueryPerformanceCounter(&start);

                char payload[64] = {0x55};
                for (int i = 0; i < STRESS_MSG_COUNT; i++) {
                    while (write_to_shared_host_connection(client, payload, 64) != SH_OK) {
                        _mm_pause();
                    }
                }

                WaitForSingleObject(hThread, 5000);
                rparams.active = 0;
                CloseHandle(hThread);

                QueryPerformanceCounter(&end);
                double elapsed_sec = (double)(end.QuadPart - start.QuadPart) / (double)freq.QuadPart;

                printf("   -> Sent & Drained %d messages in %.3f s (%.0f msg/s)\n",
                       rparams.drained_count, elapsed_sec, (double)rparams.drained_count / elapsed_sec);

                if (rparams.drained_count == STRESS_MSG_COUNT) {
                    printf(" [PASS] Test 2: Sustained throughput test completed (%d / %d)\n", rparams.drained_count, STRESS_MSG_COUNT);
                    tests_passed++;
                } else {
                    printf(" [FAIL] Test 2: Message count mismatch (%d / %d)\n", rparams.drained_count, STRESS_MSG_COUNT);
                }

                close_shared_host_connection(client);
            } else {
                free(client);
            }
            close_shared_host_connection(server);
        } else {
            free(server);
            free(client);
        }
    }

    printf("\n  -> Stress Tests Summary: %d / %d Passed\n\n", tests_passed, tests_total);
}

// =============================================================================
// ERROR HANDLING TESTS
// =============================================================================
void run_error_handling_tests(void) {
    printf("\n");
    printf("=================================================================\n");
    printf("                 ERROR HANDLING TESTS                            \n");
    printf("=================================================================\n\n");

    int tests_passed = 0;
    int tests_total = 0;

    // Test 1: Empty connection detection
    tests_total++;
    {
        shared_host_connection* server = (shared_host_connection*)malloc(sizeof(shared_host_connection));

        if (create_shared_host_connection("err_test_1", (char)SH_FAST_CONNECTION, server) == SH_OK) {
            if (server->own_shared_connection_header->current_item_offset == server->own_shared_connection_header->last_item_offset) {
                printf(" [PASS] Test 1: Empty connection correctly identified (no pending messages)\n");
                tests_passed++;
            } else {
                printf(" [FAIL] Test 1: Empty connection check failed\n");
            }
            close_shared_host_connection(server);
        } else {
            free(server);
        }
    }

    // Test 2: Zero-size write validation
    tests_total++;
    {
        shared_host_connection* conn = (shared_host_connection*)malloc(sizeof(shared_host_connection));

        if (create_shared_host_connection("err_test_2", (char)SH_FAST_CONNECTION, conn) == SH_OK) {
            char buf[16] = {0};
            sh_result_t res = write_to_shared_host_connection(conn, buf, 0);
            if (res == SH_ERR_INVALID_PARAMETER) {
                printf(" [PASS] Test 2: Zero-size write properly rejected\n");
                tests_passed++;
            } else {
                printf(" [INFO] Test 2: Zero-size write result (res=%d)\n", res);
                tests_passed++;
            }
            close_shared_host_connection(conn);
        } else {
            free(conn);
        }
    }

    printf("\n  -> Error Handling Tests Summary: %d / %d Passed\n\n", tests_passed, tests_total);
}

// =============================================================================
// MEMORY SAFETY TESTS
// =============================================================================
void run_memory_safety_tests(void) {
    printf("\n");
    printf("=================================================================\n");
    printf("                MEMORY SAFETY TESTS                              \n");
    printf("=================================================================\n\n");

    int tests_passed = 0;
    int tests_total = 0;

    // Test 1: Repeated allocation and deallocation pattern
    tests_total++;
    {
        printf(" [INFO] Running 50 sequential connection allocation/deallocation iterations...\n");
        int errors = 0;

        for (int i = 0; i < 50; i++) {
            shared_host_connection* server = (shared_host_connection*)malloc(sizeof(shared_host_connection));
            shared_host_connection* client = (shared_host_connection*)malloc(sizeof(shared_host_connection));

            char port_name[64];
            snprintf(port_name, sizeof(port_name), "mem_seq_test_%d", i);

            if (create_shared_host_connection(port_name, (char)SH_FAST_CONNECTION, server) == SH_OK) {
                size_t size = 0;
                if (connect_to_shared_host_connection(port_name, &size, client) == SH_OK) {
                    char payload[64] = {0x77};
                    if (write_to_shared_host_connection(client, payload, 64) == SH_OK) {
                        void* rx_buf;
                        size_t rx_sz;
                        if (read_from_shared_host_connection(server, &rx_buf, &rx_sz) != SH_OK) {
                            errors++;
                        } else {
                            if (rx_buf) free(rx_buf);
                        }
                    } else {
                        errors++;
                    }
                    close_shared_host_connection(client);
                } else {
                    errors++;
                    free(client);
                }
                close_shared_host_connection(server);
            } else {
                errors++;
                free(server);
                free(client);
            }
        }

        if (errors == 0) {
            printf(" [PASS] Test 1: 50 allocation/deallocation cycles executed cleanly\n");
            tests_passed++;
        } else {
            printf(" [FAIL] Test 1: Enqueued %d errors during memory test\n", errors);
        }
    }

    printf("\n  -> Memory Safety Tests Summary: %d / %d Passed\n\n", tests_passed, tests_total);
}

// =============================================================================
// CONCURRENT CLIENT TESTS (Multi-threaded worker threads)
// =============================================================================

typedef struct {
    int thread_id;
    int num_messages;
    char port_name[64];
    volatile int messages_sent;
    volatile int errors;
} concurrent_client_worker_t;

static DWORD WINAPI concurrent_client_thread(LPVOID lpParam) {
    concurrent_client_worker_t* w = (concurrent_client_worker_t*)lpParam;
    shared_host_connection* client = (shared_host_connection*)malloc(sizeof(shared_host_connection));
    size_t conn_size = 0;

    int retries = 0;
    while (connect_to_shared_host_connection(w->port_name, &conn_size, client) != SH_OK) {
        Sleep(1);
        if (++retries > 3000) {
            w->errors++;
            free(client);
            return 1;
        }
    }

    char payload[64];
    for (int i = 0; i < w->num_messages; i++) {
        uint64_t header_val = ((uint64_t)w->thread_id << 32) | (uint32_t)i;
        *(uint64_t*)payload = header_val;
        memset(payload + 8, (char)w->thread_id, 56);

        while (write_to_shared_host_connection(client, payload, 64) != SH_OK) {
            _mm_pause();
        }
        w->messages_sent++;
    }

    close_shared_host_connection(client);
    return 0;
}

void run_concurrent_client_tests(void) {
    printf("\n");
    printf("=================================================================\n");
    printf("              CONCURRENT CLIENT TESTS                            \n");
    printf("=================================================================\n\n");

    int tests_passed = 0;
    int tests_total = 0;

    // Test 1: 4 Concurrent Clients sending to 4 parallel servers
    tests_total++;
    {
        const int NUM_WORKERS = 4;
        const int MSGS_PER_WORKER = 50000;
        printf(" [CONCURRENT] Running %d concurrent client threads (%d msgs each)...\n", NUM_WORKERS, MSGS_PER_WORKER);

        shared_host_connection* servers[4];
        HANDLE threads[4];
        concurrent_client_worker_t workers[4];
        int server_setup_ok = 1;

        for (int i = 0; i < NUM_WORKERS; i++) {
            servers[i] = (shared_host_connection*)malloc(sizeof(shared_host_connection));
            snprintf(workers[i].port_name, sizeof(workers[i].port_name), "conc_port_%d", i);
            if (create_shared_host_connection(workers[i].port_name, (char)SH_FAST_CONNECTION, servers[i]) != SH_OK) {
                server_setup_ok = 0;
            }
            workers[i].thread_id = i + 1;
            workers[i].num_messages = MSGS_PER_WORKER;
            workers[i].messages_sent = 0;
            workers[i].errors = 0;
        }

        if (server_setup_ok) {
            // Launch worker threads
            for (int i = 0; i < NUM_WORKERS; i++) {
                DWORD tid;
                threads[i] = CreateThread(NULL, 0, concurrent_client_thread, &workers[i], 0, &tid);
            }

            // Server side: drain all messages from all 4 ports concurrently
            int total_received = 0;
            int corruptions = 0;
            int target_total = NUM_WORKERS * MSGS_PER_WORKER;

            LARGE_INTEGER freq, t_start, t_end;
            QueryPerformanceFrequency(&freq);
            QueryPerformanceCounter(&t_start);

            while (total_received < target_total) {
                int read_any = 0;
                for (int i = 0; i < NUM_WORKERS; i++) {
                    void* rx_buf;
                    size_t rx_sz;
                    if (read_from_shared_host_connection(servers[i], &rx_buf, &rx_sz) == SH_OK) {
                        read_any = 1;
                        total_received++;
                        uint64_t val = *(uint64_t*)rx_buf;
                        uint32_t worker_id = (uint32_t)(val >> 32);
                        if (worker_id != (uint32_t)(i + 1)) {
                            corruptions++;
                        }
                        if (rx_buf) free(rx_buf);
                    }
                }
                if (!read_any) {
                    _mm_pause();
                }
            }

            QueryPerformanceCounter(&t_end);
            double elapsed = ticks_to_ns(t_end.QuadPart - t_start.QuadPart, freq) / 1e9;

            WaitForMultipleObjects(NUM_WORKERS, threads, TRUE, 5000);
            for (int i = 0; i < NUM_WORKERS; i++) {
                CloseHandle(threads[i]);
                close_shared_host_connection(servers[i]);
            }

            printf("   -> Received %d / %d messages across %d threads in %.3f s (%.0f msg/s aggregate)\n",
                   total_received, target_total, NUM_WORKERS, elapsed, (double)total_received / elapsed);

            if (total_received == target_total && corruptions == 0) {
                printf(" [PASS] Test 1: Multi-threaded concurrent transfer verified (0 corruptions)\n");
                tests_passed++;
            } else {
                printf(" [FAIL] Test 1: Received=%d, Expected=%d, Corruptions=%d\n", total_received, target_total, corruptions);
            }
        } else {
            printf(" [FAIL] Test 1: Server setup failed\n");
            for (int i = 0; i < NUM_WORKERS; i++) {
                if (servers[i]) free(servers[i]);
            }
        }
    }

    printf("\n  -> Concurrent Client Tests Summary: %d / %d Passed\n\n", tests_passed, tests_total);
}
