#!/usr/bin/env bash

# Exit immediately on critical script failures, set up output colors
set -e

GREEN="\033[32m"
RED="\033[31m"
YELLOW="\033[33m"
CYAN="\033[36m"
RESET="\033[0m"

BUILD_DIR="build"
EXEC="compiler"
TEST_DIR="scope_tests"

echo -e "\n${CYAN}==============================================${RESET}"
echo -e "${CYAN}        BUILDING COMPILER VIA MAKE           ${RESET}"
echo -e "${CYAN}==============================================${RESET}\n"

# 1. Build project using existing Makefile target
make build

COMPILER_PATH="./${BUILD_DIR}/${EXEC}"

if [ ! -f "$COMPILER_PATH" ]; then
    echo -e "${RED}Error: Executable not found at $COMPILER_PATH${RESET}"
    exit 1
fi

if [ ! -d "$TEST_DIR" ]; then
    echo -e "${RED}Error: Test directory '$TEST_DIR' does not exist.${RESET}"
    exit 1
fi

echo -e "\n${CYAN}==============================================${RESET}"
echo -e "${CYAN}        RUNNING PHASE 2a SCOPE SUITE         ${RESET}"
echo -e "${CYAN}==============================================${RESET}\n"

PASSED_COUNT=0
FAILED_COUNT=0
TOTAL_COUNT=0

# Disable exit on command failure during test execution loop
set +e

for test_file in "$TEST_DIR"/*.txt; do
    [ -e "$test_file" ] || continue

    TOTAL_COUNT=$((TOTAL_COUNT + 1))
    filename=$(basename "$test_file")

    # Determine expected outcome based on file prefix naming convention
    if [[ "$filename" == *"_valid_"* ]]; then
        EXPECTED="PASS"
    elif [[ "$filename" == *"_err_"* ]]; then
        EXPECTED="FAIL"
    else
        EXPECTED="UNKNOWN"
    fi

    echo -e "${YELLOW}--------------------------------------------------${RESET}"
    echo -e "Test #${TOTAL_COUNT}: ${CYAN}${test_file}${RESET} (Expected: ${EXPECTED})"
    echo -e "${YELLOW}--------------------------------------------------${RESET}"

    # Pass the test file name directly to the compiler binary
    OUTPUT=$("$COMPILER_PATH" "$test_file" 2>&1)
    EXIT_CODE=$?

    if [ $EXIT_CODE -eq 0 ]; then
        ACTUAL="PASS"
    else
        ACTUAL="FAIL"
    fi

    echo "$OUTPUT"

    # Evaluate outcome against expectation
    if [ "$EXPECTED" == "$ACTUAL" ]; then
        echo -e "\nResult: ${GREEN}VERDICT PASSED${RESET} (Compiler output matched expected ${EXPECTED})\n"
        PASSED_COUNT=$((PASSED_COUNT + 1))
    else
        echo -e "\nResult: ${RED}VERDICT FAILED${RESET} (Expected ${EXPECTED}, got ${ACTUAL})\n"
        FAILED_COUNT=$((FAILED_COUNT + 1))
    fi
done

# Re-enable exit on error
set -e

echo -e "${CYAN}==============================================${RESET}"
echo -e "${CYAN}              SUMMARY REPORT                  ${RESET}"
echo -e "${CYAN}==============================================${RESET}"
echo -e "Total Executed: ${TOTAL_COUNT}"
echo -e "Passed:         ${GREEN}${PASSED_COUNT}${RESET}"
echo -e "Failed:         ${RED}${FAILED_COUNT}${RESET}"

if [ $FAILED_COUNT -gt 0 ]; then
    echo -e "\n${RED}STATUS: TEST SUITE FAILED${RESET}\n"
    exit 1
else
    echo -e "\n${GREEN}STATUS: ALL TESTS PASSED SUCCESSFULLY${RESET}\n"
    exit 0
fi
