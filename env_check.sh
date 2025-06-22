#!/usr/bin/env bash
# Simple environment bootstrapper for Android builds
set -e

REQ_NDK="r26.1.10909125"
REQ_GRADLE="8.6"

msg() { echo "[env-check] ${1}"; }

find_prop(){
    if [ -f local.properties ]; then
        grep -s "^${1}=" local.properties | cut -d'=' -f2 || true
    fi
}

sdk_dir="${ANDROID_HOME:-${ANDROID_SDK_ROOT:-$(find_prop sdk.dir)}}"
ndk_dir="${ANDROID_NDK_HOME:-$(find_prop ndk.dir)}"

if [ -z "$sdk_dir" ]; then
    msg "Android SDK not found. Set ANDROID_HOME or create local.properties."
else
    msg "Using SDK at $sdk_dir"
fi

if [ -z "$ndk_dir" ] && [ -n "$sdk_dir" ]; then
    ndk_dir="$sdk_dir/ndk/$REQ_NDK"
fi

if [ -d "$ndk_dir" ]; then
    ndk_version="$(basename "$ndk_dir")"
    if [ "$ndk_version" != "$REQ_NDK" ]; then
        msg "Warning: expected NDK $REQ_NDK but found $ndk_version"
    else
        msg "NDK version $ndk_version detected"
    fi
else
    msg "NDK not found. Expected at $ndk_dir"
fi

cmake_bin="$(command -v cmake || true)"
if [ -z "$cmake_bin" ]; then
    msg "cmake not found in PATH"
else
    msg "Using cmake $(cmake --version | head -n1)"
fi

if [ -x ./gradlew ]; then
    gradle_cmd=./gradlew
else
    gradle_cmd=$(command -v gradle || true)
fi

if [ -z "$gradle_cmd" ]; then
    msg "Gradle not found"
else
    version=$($gradle_cmd --version | awk '/Gradle /{print $2; exit}')
    if [ "$version" != "$REQ_GRADLE" ]; then
        msg "Warning: Gradle $REQ_GRADLE required, found $version"
    else
        msg "Gradle version $version"
    fi
fi

if [ ! -f local.properties ]; then
    msg "Creating default local.properties"
    echo "sdk.dir=${sdk_dir:-/opt/android-sdk}" > local.properties
    echo "ndk.dir=${ndk_dir:-/opt/android-ndk}" >> local.properties
fi

msg "Environment check complete"
