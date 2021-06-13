(Get-Command clang++) > $null

echo "$([char]0x2666) Checking for clang."

if ($? -ne 1) {
    echo "x Clang needs to be installed. Downloading it..."
    # if clang++ is not installed, it has a simple installer, download it!
    Invoke-WebRequest https://github.com/llvm/llvm-project/releases/download/llvmorg-12.0.0/LLVM-12.0.0-win64.exe -O llvm.exe
    llvm.exe
}

$version = cmd /c 'git rev-parse --short HEAD' 2>&1 | % ToString

echo "$([char]0x2666) Compiling the buld tool"
clang++ src/cli.cc -o bin/cli.exe -std=c++20 -DVERSION="$($version)"

if ($? -ne 1) {
    echo "$([char]0x2666) The build tool failed to compile. Here's what you can do..."
} else {
    echo "$([char]0x2665) Success!"
}