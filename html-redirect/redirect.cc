#include <iostream>
#include <string>
#include <filesystem>

std::string redirect(const std::string& inputPath, const std::string& basePath) {
    namespace fs = std::filesystem;

    fs::path fullPath = fs::path(basePath) / inputPath;

    if (fs::is_regular_file(fullPath)) {
        return stripBasePath(fullPath.string(), basePath);
    }

    fs::path indexPath = fullPath / "index.html";
    if (fs::is_regular_file(indexPath)) {
        return stripBasePath(indexPath.string(), basePath);
    }

    fs::path htmlPath = fullPath;
    htmlPath.replace_extension(".html");
    if (fs::is_regular_file(htmlPath)) {
        return stripBasePath(htmlPath.string(), basePath);
    }

    if (inputPath == "/") {
        fs::path rootIndexPath = fs::path(basePath) / "index.html";
        return fs::is_regular_file(rootIndexPath) ? "/index.html" : "";
    }

    return "";
}

// Helper function to strip the basePath from the fullPath
std::string stripBasePath(const std::string& fullPath, const std::string& basePath) {
    size_t pos = fullPath.find(basePath);
    if (pos != std::string::npos) {
        return fullPath.substr(pos + basePath.length());
    }
    return fullPath;
}

// int main() {
//     std::string basePath = "/path/to/base"; // Replace this with your actual base path
//     std::string testPath = "/an-index-file/a-html-file"; // Example test path
//     std::string resolvedPath = redirect(testPath, basePath);
//     std::cout << resolvedPath << std::endl;
// }
