#!/usr/bin/env bash

OS=${1}
GITHUB_WORKSPACE=${2}

# "all" is too much log information. This will increase from verbosity from error"
#export BOOST_TEST_LOG_LEVEL=all

if [[ ${OS} == "windows" ]]; then
    echo "----------------------------------------"
    echo "Checking binary security for ${OS}"
    echo "----------------------------------------"
    make -C src check-security
        # this calls contrib/devtools/security-check.py on each file specified in src/Makefile.am
        # it checks the executables for things like immunity to stack overflow bugs and support for address space randomization.
elif [[ ${OS} == "osx" ]]; then
    echo "----------------------------------------"
    echo "No binary checks available for ${OS}"
    echo "----------------------------------------"
elif [[ ${OS} == "linux" || ${OS} == "linux-disable-wallet" ]]; then
    echo "----------------------------------------"
    echo "Checking binary security for ${OS}"
    echo "----------------------------------------"
    make -C src check-security
        # this calls contrib/devtools/security-check.py on each file specified in src/Makefile.am
        # it checks the executables for things like immunity to stack overflow bugs and support for address space randomization.
    make -C src check-symbols
        # this calls contrib/devtools/symbol-check.py to make sure Linux ELF files contain only gcc,glibc,libstd++ valid symbols.
    echo "----------------------------------------"
    echo "Running unit tests for ${OS}"
    echo "----------------------------------------"
    make check
        # this compiles and runs all the unit tests recursively found by 'make':
        #   * rune the evrmored unit tests in src/test/test_evrmore
        #   * runs the evrmore-qt unit tests in src/qt/test/test_main
        #   * runs "test/util/raven-util-test.py" which checks evrmore-tx
        #   * runs the wallet unit tests in src/wallet/wallet_tests, and
        #   * runs the unit tests for some dependencies (ie for libsecp256k1 and univalue)
        # For details see src/test/README.md and test/README.md
    echo "----------------------------------------"
    echo "Running functional tests for ${OS}"
    echo "----------------------------------------"
    ${GITHUB_WORKSPACE}/test/functional/test_runner.py --extended
        # see test/README.md for details (note: NOT src/test)
elif [[ ${OS} == "arm32v7" || ${OS} == "arm32v7-disable-wallet" || ${OS} == "aarch64" || ${OS} == "aarch64-disable-wallet" ]]; then
    echo "----------------------------------------"
    echo "No binary checks available for ${OS}"
    echo "----------------------------------------"
else
    echo "You must pass an OS."
    echo "Usage: ${0} <operating system> <github workspace path>"
    exit 1
fi

