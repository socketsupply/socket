# Notes about Microsoft's `WebView2`

For some terrible reason WebView2 component is a semi-userland installable/uninstallable component. This
conversation gets really stupid, go ahead, google for it.

The headers needed to compile using this are here https://www.nuget.org/packages/Microsoft.Web.WebView2

Running `.\bin\bootstrap.ps1 update` will get the latest copy of the `WebView2.h` file from the package
and extract it into this repo.
