# TROUBLE SHOOTING

## Windows

### Setting up a Windows development environment

[`clang++`][0] [version 12][1] required for building.

You will need [build tools][3]

The `WebView2LoaderStatic.lib` file was sourced from [this][2] package.

[0]:https://github.com/llvm/llvm-project/releases/tag/llvmorg-12.0.0
[1]:https://github.com/llvm/llvm-project/releases/download/llvmorg-12.0.0/LLVM-12.0.0-win64.exe
[2]:https://www.nuget.org/api/v2/package/Microsoft.Web.WebView2/1.0.864.35
[3]: https://visualstudio.microsoft.com/downloads/#build-tools-for-visual-studio-2019

### cannot be loaded because running scripts is disabled on this system.

If you get the following error message

```
./bin/bootstrap.ps1 : File C:\Users\rayno\Github\opkit\bin\bootstrap.ps1 cannot be loaded because running scripts is
disabled on this system. For more information, see about_Execution_Policies at
https:/go.microsoft.com/fwlink/?LinkID=135170.
```

Then you can follow https://superuser.com/a/106363

1. Start Windows PowerShell with the "Run as Administrator" option.
2. `set-executionpolicy remotesigned`

### MSVC build environment from Git Bash

You can leverage the MSVC build tools (`clang++`) and environment headers directly in Git Bash by loading it into your shell environment directly.
This is possible by running the following command:

```sh
source bin/mscv-bash-env.sh
```

The `bin/bootstrap.sh` shell script should work for compiling the `op` (fka `opkit`) tool.
It is also recommneded to initialize this environment when building applications with `op` (`opkit`) from the CLI so
the correct build tools can be used which ensures header and library paths for the compiler 

## Linux
### Build failures

If you are getting a failure that the build tool cant locate your
compiler, try making sure that the `CXX` environment variable is
set to the location of your C++ compiler (`which g++`, or `which c++`).

The latest version of MacOS should have installed C++ for you. But
on Linux you may need to update some packages. To ensure you have
the latest clang compiler and libraries you can try the follwing...

For debian/ubuntu, before you install the packages, you may want
to [add][0] these software update repos [here][1] to the software
updater.

#### ubuntu

```sh
sudo apt install \
  build-essential \
  clang-12 \
  libc++1-12-dev \
  libc++abi-12-dev \
  libwebkit2gtk-4.0-dev
```

[0]:https://linuxize.com/post/how-to-add-apt-repository-in-ubuntu/
[1]:https://apt.llvm.org/

#### arch/manjaro

arch uses the latest versions, so just install `base-devel`

``` sh
sudo pacman -S base-devel
```

### Running multiple versions of g++

If you've tried running the above `apt install` and you get an error
related `Unable to locate package` then you can also install multiple
versions of G++ on your system.

```sh
sudo apt install software-properties-common
sudo add-apt-repository ppa:ubuntu-toolchain-r/test
sudo apt install gcc-7 g++-7 gcc-8 g++-8 gcc-9 g++-9 gcc-10 g++-10
```

Then you can set your C++ compiler as `g++-10`

```sh
# Add this to bashrc
export CXX=g++-10
```

### Still can't find Webkit

If you run into an error about not finding webkit & gtk like this:

```
Package webkit2gtk-4.0 was not found in the pkg-config search path.
Perhaps you should add the directory containing `webkit2gtk-4.0.pc'
to the PKG_CONFIG_PATH environment variable
No package 'webkit2gtk-4.0' found
In file included from /usr/local/lib/opkit/src/webview.h:164,
                 from /usr/local/lib/opkit/src/main.cc:1:
/usr/local/lib/opkit/src/linux.h:11:10: fatal error: JavaScriptCore/JavaScript.h: No such file or directory
   11 | #include <JavaScriptCore/JavaScript.h>
      |          ^~~~~~~~~~~~~~~~~~~~~~~~~~~~~
compilation terminated.
/usr/local/lib/opkit/src/linux.cc:2:10: fatal error: gtk/gtk.h: No such file or directory
    2 | #include <gtk/gtk.h>
      |          ^~~~~~~~~~~
compilation terminated.
```

Then you will want to install those dependencies

```sh
sudo apt-get install libwebkit2gtk-4.0-dev
```
