git submodule update --init --recursive

pushd "$(dirname "$0")"

mkdir -p android-dependencies

pushd android-dependencies

# Download Android SDK and NDK
#curl -L https://dl.google.com/android/repository/android-ndk-r14b-linux-x86_64.zip -O
curl -L https://dl.google.com/android/repository/sdk-tools-linux-3859397.zip -O


# Install NDK
#mkdir -p android-ndk
#unzip -q android-ndk-r14b-linux-x86_64.zip -d android-ndk
#rm android-ndk-r14b-linux-x86_64.zip

# Install SDK
mkdir -p android-sdk
unzip -q sdk-tools-linux-3859397.zip -d android-sdk
rm sdk-tools-linux-3859397.zip

# Setup environment
export ANDROID_HOME=`pwd`/android-sdk
#export ANDROID_NDK_HOME=`pwd`/android-ndk
#export LOCAL_ANDROID_NDK_HOME="$ANDROID_NDK_HOME"
#export LOCAL_ANDROID_NDK_HOST_PLATFORM="linux-x86_64"
export PATH=$PATH:${ANDROID_HOME}
export PATH=$PATH:${ANDROID_HOME}/tools
#export PATH=$PATH:${ANDROID_NDK_HOME}

# Setup Android SDK
echo "Updating SDK"
yes | ${ANDROID_HOME}/tools/bin/sdkmanager --update  > /dev/null # /dev/null because travis complains about the build log getting too long

echo "Downloading Android Build Tools"
yes | ${ANDROID_HOME}/tools/bin/sdkmanager "platforms;android-21" "build-tools;25.0.2" "extras;google;m2repository" "extras;android;m2repository" "ndk-bundle" > /dev/null

echo "Andorid SDK Licenses..."
yes | ${ANDROID_HOME}/tools/bin/sdkmanager --licenses

popd # Leaving dependencies

export TERM=dumb # Fixes gradle output looking bad
./gradlew build

popd # Leaving script-folder
