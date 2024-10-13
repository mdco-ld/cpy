#include <cerrno>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <utility>
#include <vector>

#define PROGRAM_NAME "cpy"

std::string readAllFromFd(int fd) {
    const size_t BUFFER_SIZE = 2048;
    char buffer[BUFFER_SIZE + 1];
    size_t length = 0;
    std::string data;
    do {
        length = read(fd, buffer, BUFFER_SIZE);
        buffer[length] = 0;
        data += buffer;
    } while (length == BUFFER_SIZE);
    return data;
}

void copyToClipboard(const std::string_view content) {
    int fd[2];
    pipe(fd);
    pid_t pid = fork();
    if (pid == 0) {
        close(fd[1]);
        dup2(fd[0], fileno(stdin));
        execlp("xclip", "xclip", "-selection", "clipboard", "-i", nullptr);
        std::cerr << "Error executing xclip: " << std::strerror(errno)
                  << std::endl;
        exit(1);
    } else {
        close(fd[0]);
        write(fd[1], content.data(), content.length());
        close(fd[1]);
        int status;
        waitpid(pid, &status, 0);
        if (status != 0) {
            std::cerr << "Failed to copy to clipboard" << std::endl;
            exit(1);
        }
    }
}

void copyFileToClipboard(const std::string_view path,
                         const std::string_view fileType) {
    pid_t pid = fork();
    if (pid == 0) {
        execlp("xclip", "xclip", "-selection", "clipboard", "-t",
               fileType.data(), path.data(), nullptr);
        std::cerr << "Error executing xclip: " << std::strerror(errno)
                  << std::endl;
        exit(1);
    } else {
        int status;
        waitpid(pid, &status, 0);
        if (status != 0) {
            std::cerr << "Failed to copy to clipboard" << std::endl;
            exit(1);
        }
    }
}

std::string getClipboardContents() {
    int fd[2];
    pipe(fd);
    pid_t pid = fork();
    if (pid == 0) {
        close(fd[0]);
        dup2(fd[1], fileno(stdout));
        execlp("xclip", "xclip", "-selection", "clipboard", "-o", nullptr);
        std::cerr << "Error executing xclip: " << std::strerror(errno)
                  << std::endl;
        exit(1);
    } else {
        close(fd[1]);
        std::string content = readAllFromFd(fd[0]);
        close(fd[0]);
        int status;
        waitpid(pid, &status, 0);
        if (status != 0) {
            std::cerr << "Error getting clipboard data" << std::endl;
            exit(1);
        }
        return content;
    }
}

std::optional<std::string_view>
getFileTypeFromExtension(const std::string_view extension) {
    static const std::map<std::string_view, std::string_view>
        wellKnownExtensions = {
            {"json", "application/json"}, {"pdf", "application/pdf"},
            {"zip", "application/zip"},   {"png", "image/png"},
            {"jpeg", "image/jpeg"},       {"jpg", "image/jpeg"},
            {"gif", "image/gif"},         {"bmp", "image/bmp"},
            {"js", "text/javascript"},    {"html", "text/html"},
            {"css", "text/css"},
        };
    auto it = wellKnownExtensions.find(extension);
    if (it != wellKnownExtensions.end()) {
        return it->second;
    }
    return std::nullopt;
}

void copyRawFileToClipboard(std::string_view path) {
    FILE *file = fopen(path.data(), "r");
    int fd = fileno(file);
    std::string data = readAllFromFd(fd);
    copyToClipboard(data);
}

void copyFile(std::string_view path) {
    std::string::size_type pos = path.rfind(".");
    if (pos == path.npos) {
        copyRawFileToClipboard(path);
        return;
    }
    std::string_view extension = std::string_view(path).substr(pos + 1);
    std::optional<std::string_view> fileTypeOpt =
        getFileTypeFromExtension(extension);
    if (!fileTypeOpt.has_value()) {
        copyRawFileToClipboard(path);
        return;
    }
    std::string_view fileType = fileTypeOpt.value();
    copyFileToClipboard(path, fileType);
}

void printUsage() {
    std::vector<std::pair<std::string, std::string>> examples = {
        {
            PROGRAM_NAME " <filename>",
            "Copy file to clipboard",
        },
        {
            "<command> | " PROGRAM_NAME,
            "Copy the output of command to clipboard",
        },
        {
            PROGRAM_NAME,
            "Output clipboard contents to stdout",
        },
        {
            PROGRAM_NAME " > output.txt",
            "Output clipboard contents to file",
        },
    };
    size_t maxLength = 0;
    for (auto &[command, explanation] : examples) {
        maxLength = std::max(maxLength, command.length());
    }
    for (auto &[command, explanation] : examples) {
        while (command.length() < maxLength) {
            command += ' ';
        }
    }
    std::cout << "Usage:" << std::endl;
    for (auto &[command, explanation] : examples) {
        std::cout << '\t' << command << " # " << explanation << std::endl;
    }
}

int main(int argc, char *argv[]) {
    if (argc == 1) {
        if (isatty(fileno(stdin))) {
            std::cout << getClipboardContents();
        } else {
            std::string data = readAllFromFd(fileno(stdin));
            copyToClipboard(data);
        }
    } else if (argc == 2) {
        std::string arg = argv[1];
        if (arg == "--help" || arg == "-h") {
            printUsage();
            return 0;
        }
        copyFile(arg);
    } else {
        printUsage();
    }
    return 0;
}
