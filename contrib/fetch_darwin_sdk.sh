#!/bin/bash
# Copyright (c) 2025 The Echelon Technology Group developers

# Script to fetch the Darwin SDK if not present

# Color codes for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
ORANGE='\033[38;5;202m'
#PURPLE='\033[0;35m'
PURPLE='\033[38;5;129m'
NC='\033[0m' # No Color

# Function to print colored output
print_status() {
    echo -e "${GREEN}[+]${NC} $1"
}

print_error() {
    echo -e "${RED}[!]${NC} $1"
}

print_warning() {
    echo -e "${ORANGE}[*]${NC} $1"
}

# Check if running as root
if [ "$EUID" -eq 0 ]; then
   print_warning "Running as root. This is not recommended."
fi

SDK_VERSION="11.3.1-11C505"
SDK_FILENAME="Xcode-${SDK_VERSION}-extracted-SDK-with-libcxx-headers.tar.gz"
SDK_URL="https://bitcoincore.org/depends-sources/sdks/${SDK_FILENAME}"
SDK_DIR="SDKs"
SDK_PATH="${SDK_DIR}/Xcode-${SDK_VERSION}-extracted-SDK-with-libcxx-headers"

# Expected SHA256 hash (update this with actual hash)
EXPECTED_SHA256="436df6dfc7073365d12f8ef6c1fdb060777c720602cc67c2dcf9a59d94290e38"

if [ -d "${SDK_PATH}" ]; then
    print_status "Darwin SDK already present at ${SDK_PATH}"
    exit 0
fi

print_status "Fetching Darwin SDK ${SDK_VERSION}..."
mkdir -p "${SDK_DIR}"

# Download if not present
if [ ! -f "${SDK_DIR}/${SDK_FILENAME}" ]; then
    print_status "Downloading ${SDK_FILENAME}..."
    wget -P "${SDK_DIR}" "${SDK_URL}" || {
        print_error "Failed to download SDK"
        exit 1
    }
fi

# Verify checksum
print_status "Verifying checksum..."
echo "${EXPECTED_SHA256}  ${SDK_DIR}/${SDK_FILENAME}" | sha256sum -c - || {
    print_error "Checksum verification failed!"
    rm -f "${SDK_DIR}/${SDK_FILENAME}"
    exit 1
}

# Extract
print_status "Extracting SDK..."
cd "${SDK_DIR}" && tar -xzf "${SDK_FILENAME}" || {
    print_error "Failed to extract SDK"
    exit 1
}

print_status "Darwin SDK ready at ${SDK_PATH}"
echo
echo "You can now run for a x86_64 (64-bit Intel) build:"
echo "  make HOST=x86_64-apple-darwin -j$(nproc)"
echo
print_warning "  Users can install Rosetta 2 upon initial launch if not already installed on their system, on ARM Apple Silicon architecture (M1/M2/M3)"
echo
print_warning "  Note: -j$(nproc) uses $(nproc) parallel jobs. Adjust based on your system core/thread count."
echo
echo -e "${RED}(㇏(•̀ᵥᵥ•́)ノ)${PURPLE} This shit ain't nothin' to me man!${NC}"
echo
