// AUTO-DRAFT from redis/redis PR #15804
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
  // <<< BUG ANCHOR
#define PAGE_SIZE 4096*4
#define MAX_JSON_SIZE (PAGE_SIZE - 128)  /* Keep some margin */
#define MAX_FIELD_SIZE 64
#define NUM_TEST_ITERATIONS 100000
#define NUM_CORRUPTION_TESTS 10000
#define NUM_BOUNDARY_TESTS 10000

/* Test state tracking */
static char *safe_page = NULL;       /* Start of readable/writable page */
/* …（同文件无关代码省略）… */
static int tests_failed = 0;
static int corruptions_passed = 0;
static int boundary_tests_passed = 0;

/* Test metadata for tracking */
typedef struct {
/* …（同文件无关代码省略）… */
void run_normal_tests(void);
void run_corruption_tests(void);
void run_boundary_tests(void);
void print_test_summary(void);

/* Signal handler for segmentation violations */
/* …（同文件无关代码省略）… */
exprtoken *safe_extract_field(const char *json, size_t json_len,
                             const char *field, size_t field_len) {
    boundary_violation = 0;

    if (setjmp(jmpbuf) == 0) {
        return jsonExtractField(json, json_len, field, field_len);
    } else {
        return NULL; /* Return NULL if boundary violation occurred */
    }
}
/* …（同文件无关代码省略）… */
void cleanup_test_memory(void) {
    if (safe_page != NULL) {
        munmap(safe_page, PAGE_SIZE);
        safe_page = NULL;
        unsafe_page = NULL;
    }
}
/* …（同文件无关代码省略）… */
void generate_random_string(char *buffer, size_t max_len) {
    static const char charset[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    size_t len = 1 + rand() % (max_len - 2); /* Ensure at least 1 char */

    for (size_t i = 0; i < len; i++) {
        buffer[i] = charset[rand() % (sizeof(charset) - 1)];
    }
    buffer[len] = '\0';
}
/* …（同文件无关代码省略）… */
void generate_random_number(char *buffer, size_t max_len) {
    double num = (double)rand() / RAND_MAX * 1000.0;

    /* Occasionally make it negative or add decimal places */
    if (rand() % 5 == 0) num = -num;
    if (rand() % 3 != 0) num += (double)(rand() % 100) / 100.0;

    snprintf(buffer, max_len, "%.6g", num);
}
/* …（同文件无关代码省略）… */
void generate_random_field(char *field, size_t *field_len) {
    generate_random_string(field, MAX_FIELD_SIZE / 2);
    *field_len = strlen(field);
}
/* …（同文件无关代码省略）… */
char *generate_random_json(size_t *len, char *field, size_t *field_len, int *has_field) {
    char *json = malloc(MAX_JSON_SIZE);
    if (json == NULL) {
        perror("malloc");
        exit(EXIT_FAILURE);
    }

    char buffer[MAX_JSON_SIZE / 4]; /* Buffer for generating values */
    int pos = 0;
    int num_fields = 1 + rand() % 10; /* Random number of fields */
    int target_field_index = rand() % num_fields; /* Which field to return */

    /* Start the JSON object */
    pos += snprintf(json + pos, MAX_JSON_SIZE - pos, "{");

    /* Generate random field/value pairs */
    for (int i = 0; i < num_fields; i++) {
        /* Add a comma if not the first field */
        if (i > 0) {
            pos += snprintf(json + pos, MAX_JSON_SIZE - pos, ", ");
        }

        /* Generate a field name */
        if (i == target_field_index) {
            /* This is our target field - save it for the caller */
            generate_random_field(field, field_len);
            pos += snprintf(json + pos, MAX_JSON_SIZE - pos, "\"%s\": ", field);
            *has_field = 1;
            /* Sometimes change the last char so that it will not match. */
            if (rand() % 2) {
                *has_field = 0;
                field[*field_len-1] = '!';
            }
        } else {
            generate_random_string(buffer, MAX_FIELD_SIZE / 4);
            pos += snprintf(json + pos, MAX_JSON_SIZE - pos, "\"%s\": ", buffer);
        }

        /* Generate a random value type */
        int value_type = rand() % 5;
        switch (value_type) {
            case 0: /* String */
                generate_random_string(buffer, MAX_JSON_SIZE / 8);
                pos += snprintf(json + pos, MAX_JSON_SIZE - pos, "\"%s\"", buffer);
                break;

            case 1: /* Number */
                generate_random_number(buffer, MAX_JSON_SIZE / 8);
                pos += snprintf(json + pos, MAX_JSON_SIZE - pos, "%s", buffer);
                break;

            case 2: /* Boolean: true */
                pos += snprintf(json + pos, MAX_JSON_SIZE - pos, "true");
                break;

            case 3: /* Boolean: false */
                pos += snprintf(json + pos, MAX_JSON_SIZE - pos, "false");
                break;

            case 4: /* Null */
                pos += snprintf(json + pos, MAX_JSON_SIZE - pos, "null");
                break;

            case 5: /* Array (simple) */
                pos += snprintf(json + pos, MAX_JSON_SIZE - pos, "[");
                int array_items = 1 + rand() % 5;
                for (int j = 0; j < array_items; j++) {
                    if (j > 0) pos += snprintf(json + pos, MAX_JSON_SIZE - pos, ", ");

                    /* Array items - either number or string */
                    if (rand() % 2) {
                        generate_random_number(buffer, MAX_JSON_SIZE / 16);
                        pos += snprintf(json + pos, MAX_JSON_SIZE - pos, "%s", buffer);
                    } else {
                        generate_random_string(buffer, MAX_JSON_SIZE / 16);
                        pos += snprintf(json + pos, MAX_JSON_SIZE - pos, "\"%s\"", buffer);
                    }
                }
                pos += snprintf(json + pos, MAX_JSON_SIZE - pos, "]");
                break;
        }
    }

    /* Close the JSON object */
    pos += snprintf(json + pos, MAX_JSON_SIZE - pos, "}");
    *len = pos;

    return json;
}
/* …（同文件无关代码省略）… */
void corrupt_json(char *json, size_t len) {
    if (len < 2) return;  /* Too short to corrupt safely */

    /* Corrupt 1-3 characters */
    int num_corruptions = 1 + rand() % 3;
    for (int i = 0; i < num_corruptions; i++) {
        size_t pos = rand() % len;
        char corruption = " \t\n{}[]\":,0123456789abcdefXYZ"[rand() % 30];
        json[pos] = corruption;
    }
}
/* …（同文件无关代码省略）… */
void run_normal_tests(void) {
    printf("Running normal JSON extraction tests...\n");

    for (int i = 0; i < NUM_TEST_ITERATIONS; i++) {
        char field[MAX_FIELD_SIZE] = {0};
        size_t field_len = 0;
        size_t json_len = 0;
        int has_field = 0;

        /* Generate random JSON */
        char *json = generate_random_json(&json_len, field, &field_len, &has_field);

        /* Use valid field to test parser */
        exprtoken *token = safe_extract_field(json, json_len, field, field_len);

        /* Check if we got a token as expected */
        if (has_field && token != NULL) {
            exprTokenRelease(token);
            tests_passed++;
        } else if (!has_field && token == NULL) {
            tests_passed++;
        } else {
            tests_failed++;
        }

        /* Test with a non-existent field */
        char nonexistent_field[MAX_FIELD_SIZE] = "nonexistent_field";
        token = safe_extract_field(json, json_len, nonexistent_field, strlen(nonexistent_field));

        if (token == NULL) {
            tests_passed++;
        } else {
            exprTokenRelease(token);
            tests_failed++;
        }

        free(json);
    }
}
/* …（同文件无关代码省略）… */
void run_corruption_tests(void) {
    printf("Running JSON corruption tests...\n");

    for (int i = 0; i < NUM_CORRUPTION_TESTS; i++) {
        char field[MAX_FIELD_SIZE] = {0};
        size_t field_len = 0;
        size_t json_len = 0;
        int has_field = 0;

        /* Generate random JSON */
        char *json = generate_random_json(&json_len, field, &field_len, &has_field);

        /* Make a copy and corrupt it */
        char *corrupted = malloc(json_len + 1);
        if (!corrupted) {
            perror("malloc");
            free(json);
            exit(EXIT_FAILURE);
        }

        memcpy(corrupted, json, json_len + 1);
        corrupt_json(corrupted, json_len);

        /* Test with corrupted JSON */
        exprtoken *token = safe_extract_field(corrupted, json_len, field, field_len);

        /* We're just testing that it doesn't crash or access invalid memory */
        if (boundary_violation) {
            printf("Boundary violation with corrupted JSON!\n");
            tests_failed++;
        } else {
            if (token != NULL) {
                exprTokenRelease(token);
            }
            corruptions_passed++;
        }

        free(corrupted);
        free(json);
    }
}
/* …（同文件无关代码省略）… */
void run_boundary_tests(void) {
    printf("Running memory boundary tests...\n");

    for (int i = 0; i < NUM_BOUNDARY_TESTS; i++) {
        char field[MAX_FIELD_SIZE] = {0};
        size_t field_len = 0;
        size_t json_len = 0;
        int has_field = 0;

        /* Generate random JSON */
        char *temp_json = generate_random_json(&json_len, field, &field_len, &has_field);

        /* Truncate the JSON to a random length */
        size_t truncated_len = 1 + rand() % json_len;

        /* Place at the edge of the safe page */
        size_t offset = PAGE_SIZE - truncated_len;
        memcpy(safe_page + offset, temp_json, truncated_len);

        /* Test parsing with non-existent field (forcing it to scan to end) */
        char nonexistent_field[MAX_FIELD_SIZE] = "nonexistent_field";
        exprtoken *token = safe_extract_field(safe_page + offset, truncated_len,
                                             nonexistent_field, strlen(nonexistent_field));

        /* We're just testing that it doesn't access memory beyond the boundary */
        if (boundary_violation) {
            printf("Boundary violation at edge of memory page!\n");
            tests_failed++;
        } else {
            if (token != NULL) {
                exprTokenRelease(token);
            }
            boundary_tests_passed++;
        }

        free(temp_json);
    }
}

/* Print summary of test results */
void print_test_summary(void) {
    printf("\n===== FASTJSON PARSER TEST SUMMARY =====\n");
    printf("Normal tests passed: %d/%d\n", tests_passed, NUM_TEST_ITERATIONS * 2);
    printf("Corruption tests passed: %d/%d\n", corruptions_passed, NUM_CORRUPTION_TESTS);
    printf("Boundary tests passed: %d/%d\n", boundary_tests_passed, NUM_BOUNDARY_TESTS);
    printf("Failed tests: %d\n", tests_failed);

    if (tests_failed == 0) {
/* …（同文件无关代码省略）… */
    }
}

/* Entry point for fastjson parser test */
void run_fastjson_test(void) {
    printf("Starting fastjson parser stress test...\n");

    /* Seed the random number generator */
/* …（同文件无关代码省略）… */
    run_normal_tests();
    run_corruption_tests();
    run_boundary_tests();

    /* Print summary */
    print_test_summary();

    /* Cleanup */
    cleanup_test_memory();
}
