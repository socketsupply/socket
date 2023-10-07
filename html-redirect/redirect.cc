#include <iostream>
#include <string>
#include <filesystem>

std::string redirect(const std::string& inputPath, const std::string& basePath) {
    namespace fs = std::filesystem;

    // Resolve the full path
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

    // 3. Check if appending a .html file extension gives a valid file
    fs::path htmlPath = fullPath;
    htmlPath.replace_extension(".html");
    if (fs::is_regular_file(htmlPath)) {
        return htmlPath.string().substr(basePath.length());
    }

    // Special case for root path
    if (inputPath == "/") {
        fs::path rootIndexPath = fs::path(basePath) / "index.html";
        return fs::exists(rootIndexPath) ? "/index.html" : "";
    }

    // If no valid path is found, return empty string
    return "";
}

int main() {
    std::string basePath = "/path/to/base"; // Replace this with your actual base path
    std::string testPath = "/an-index-file/a-html-file"; // Example test path
    std::string resolvedPath = redirect(testPath, basePath);
    std::cout << resolvedPath << std::endl;
}
