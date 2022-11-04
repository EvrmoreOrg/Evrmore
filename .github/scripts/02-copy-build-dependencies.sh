#!/usr/bin/env bash

OS=${1}
GITHUB_WORKSPACE=${2}
GITHUB_REF=${3}

if [[ ! ${OS} || ! ${GITHUB_WORKSPACE} ]]; then
    echo "Error: Invalid options"
    echo "Usage: ${0} <operating system> <github workspace path>"
    exit 1
fi
echo "----------------------------------------"
echo "OS: ${OS}"
echo "----------------------------------------"

if [[ ${OS} == "arm32v7-disable-wallet" || ${OS} == "linux-disable-wallet" || ${OS} == "aarch64-disable-wallet" ]]; then
    OS=`echo ${OS} | cut -d"-" -f1`
fi

echo "----------------------------------------"
echo "Building Dependencies for ${OS}"
echo "----------------------------------------"

cd depends
if [[ ${OS} == "windows" ]]; then
    make HOST=x86_64-w64-mingw32 -j2
elif [[ ${OS} == "osx" ]]; then
    cd ${GITHUB_WORKSPACE}
    # curl -O <url>
    echo "LEGAL issues with OSX SDK, need to get it yourself and extract the valuable stuff."
    echo "See github -> contrib/macdeploy/README.md"
    echo "Countdown from 3 sec. Make sure your SDK is at /tmp/SDKs and it is the only *.tar.gz file."
    echo "3..."
    sleep 1
    echo "2..."
    sleep 1
    echo "1..."
    sleep 1
    echo "0.. continuing.."
    mkdir -p ${GITHUB_WORKSPACE}/depends/SDKs 
    cd  ${GITHUB_WORKSPACE}/depends/SDKs
    tar -zxf /tmp/SDKs/*.tar.gz
    cd ${GITHUB_WORKSPACE}/depends && make HOST=x86_64-apple-darwin14 -j2
elif [[ ${OS} == "linux" || ${OS} == "linux-disable-wallet" ]]; then
    make HOST=x86_64-linux-gnu -j2
elif [[ ${OS} == "arm32v7" || ${OS} == "arm32v7-disable-wallet" ]]; then
    make HOST=arm-linux-gnueabihf -j2
elif [[ ${OS} == "aarch64" || ${OS} == "aarch64-disable-wallet" ]]; then
    make HOST=aarch64-linux-gnu -j2
fi
