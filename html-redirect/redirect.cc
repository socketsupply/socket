#include <iostream>
#include <string>
#include <filesystem>

std::string redirect(std::string inputPath, const std::string& basePath) {
    namespace fs = std::filesystem;

    if (inputPath.starts_with("/")) {
        inputPath = inputPath.substr(1);
    }

    // Resolve the full path
    fs::path fullPath = fs::path(basePath) / fs::path(inputPath);

    // 1. Try the given path if it's a file
    if (fs::is_regular_file(fullPath)) {
        return "/" + fs::relative(fullPath, basePath).string();
    }

    // 2. Try appending a `/` to the path and checking for an index.html
    fs::path indexPath = fullPath / fs::path("index.html");
    if (fs::is_regular_file(indexPath)) {
        // Also redirect
        return "/" + fs::relative(indexPath, basePath).string();
    }

    // 3. Check if appending a .html file extension gives a valid file
    fs::path htmlPath = fullPath;
    htmlPath.replace_extension(".html");
    if (fs::is_regular_file(htmlPath)) {
        return "/" + fs::relative(htmlPath, basePath).string();
    }

    // If no valid path is found, return empty string
    return "";
}

int main() {
    std::string basePath = "/Users/bret/Developer/ssc/socket/html-redirect/test-cases/1";
    std::string testPath = "/";
    std::string resolvedPath = redirect(testPath, basePath);
    std::cout << resolvedPath << std::endl;
}
