#include <iostream>
#include <string>
#include <filesystem>

std::string redirect(const std::string& inputPath, const std::string& basePath) {
  namespace fs = std::filesystem;

  // Special case for root path
  if (inputPath == "/") return "/index.html";

  fs::path fullPath = fs::path(basePath) / inputPath;

  // 1. Try the given path if it's a file
  if (fs::is_regular_file(fullPath)) {
      return fullPath.string().substr(basePath.length());
  }

  // 2. Try appending a `/` to the path and checking for an index.html
  fs::path indexPath = fullPath / "index.html";
  if (fs::exists(indexPath)) {
      return indexPath.string().substr(basePath.length());
  }

  // 3. Check if the input ends with .html, strip it and check
  if (fullPath.extension() == ".html") {
      fs::path strippedPath = fullPath;
      strippedPath.replace_extension("");
      if (fs::exists(strippedPath)) {
          return strippedPath.string().substr(basePath.length());
      }
  }

  // 4. Check if a .html file exists for the given path
  fs::path htmlPath = fullPath;
  htmlPath.replace_extension(".html");
  if (fs::exists(htmlPath)) {
      return htmlPath.string().substr(basePath.length());
  }

  // If no valid path is found, return empty string
  return "";
}

int main() {
  std::string basePath = "/path/to/base"; // Replace this with your actual base path
  std::string testPath = "/a-conflict-index"; // Example test path
  std::string resolvedPath = redirect(testPath, basePath);
  std::cout << resolvedPath << std::endl;
}
