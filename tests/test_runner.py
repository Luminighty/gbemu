#!/bin/python3

import os
import sys
import subprocess

SRC_OBJS_FOLDER = "./build"
TESTS_FOLDER = "./tests"
INCLUDES = "./include"


def build_project():
    result = subprocess.run(['make'], capture_output=True, text=True)
    print(result.stdout)
    if result.returncode != 0:
        print(result.stderr)
        sys.exit(1)


if not os.path.exists(SRC_OBJS_FOLDER):
    os.makedirs(SRC_OBJS_FOLDER)

build_project()
SRC_OBJS = [os.path.join(SRC_OBJS_FOLDER, f) for f in os.listdir(SRC_OBJS_FOLDER) if f != "main.o" ]

formatted = "--no-format" not in sys.argv


RED = "\033[1m\033[91m" if formatted else ""
GREEN = "\033[1m\033[92m" if formatted else ""
RESET = "\033[0m" if formatted else ""

def execute_test(directory, c_file):
    c_file_path = os.path.join(directory, c_file)

    o_file = c_file.replace('.c', '.o')
    o_file = os.path.join(directory, "build", o_file)
    subprocess.run(['gcc', '-c', c_file_path, '-I', INCLUDES, '-o', o_file], check=True)

    executable = c_file.replace('.c', '.out')
    executable = os.path.join(directory, "bin", executable)
    subprocess.run(['gcc', o_file, *SRC_OBJS, '-o', executable], check=True)

    result = subprocess.run([executable], capture_output=True, text=True)
    if result.returncode == 0:
        print(f"{GREEN}[PASS]{RESET} {c_file} passed...")
    else:
        print(f"{RED}[FAIL]{RESET} {c_file} failed...")
        if len(result.stdout):
            print(result.stdout)
        if len(result.stderr):
            print(result.stderr)

    return result.returncode == 0


def run_all_tests(directory):
    os.makedirs(os.path.join(directory, "build"), exist_ok=True)
    os.makedirs(os.path.join(directory, "bin"), exist_ok=True)
    c_files = [f for f in os.listdir(directory) if f.endswith('.c')]
    if not c_files:
        print(f"No tests found in {directory}.")
        return
    all_passed = True
    for c_file in c_files:
        try:
            result = execute_test(directory, c_file)
            all_passed = all_passed and result
        except subprocess.CalledProcessError as e:
            print(f"An error occurred during compilation/linking: {e}")
    if not all_passed:
        sys.exit(1)

if __name__ == "__main__":
    run_all_tests(TESTS_FOLDER)
