# TROUBLE SHOOTING

## Build failures

If you are getting a failure that the build tool cant locate your
compiler, try making sure that the `CXX` environment variable is
set to the location of your C++ compiler (`which g++`, or `which c++`).

The latest version of MacOS should have installed C++ for you. But
on Linux you may need to update some packages. To ensure you have
the latest clang compiler and libraries you can try the follwing...

For debian/ubuntu, before you install the packages, you may want
to [add][0] these software update repos [here][1] to the software
updater.

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
