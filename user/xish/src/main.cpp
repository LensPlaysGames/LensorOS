// clang-format off

#include <filesystem>
#include <format>
#include <print>
#include <vector>

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>

// clang-format on

#ifdef __lensor__
#include <bits/io_defs.h>
#endif

constexpr const char prompt[] = "  $:";

// DON'T redirect stdout of the program being run to a pipe.
// DON'T wait for the program being run to exit before returning to the
// main shell loop.
// Effect: the output from the program being run is completely invisible,
// and the user is free to run other programs.
// Most often used to start up a server of some sort.
// TODO: It would be nice to keep track of programs run in this way, so
// that we may check up on them later to see if they are still running,
// what their stdout/stderr look like, etc.
void run_program_quiet_nowait(const char* const filepath, char* const* args) {
    if (fork() == 0) execv(filepath, args);
}

// FIXME: May want to do ErrorOr or some type of variant so that we can
// tell when run_program_waitpid itself failed vs the program that was
// run failing.
/// @param filepath Passed to `exec` syscall
/// @param args
///   NULL-terminated array of pointers to NULL-terminated strings.
///   Passed to `exec` syscall
int run_program_waitpid(const char* const filepath, const char** args) {
    if (not filepath or not args) {
        std::print("[XiSh]: internal error: null arguments\n");
        exit(1);
    }

    std::print("[XiSh]: running command: {}", filepath);
    for (auto arg = args; *arg; ++arg)
        std::print(" {}", *arg);
    std::print("\n");

    errno = 0;

    int fds[2] = {-1, -1};
    auto pipe_rc = pipe(fds);
    if (pipe_rc != 0) {
        std::print("Failed to create pipe: rc={} errno={}\n", pipe_rc, errno);
        return -1;
    }
    // std::print("[XiSh]: Created pipe: ({}, {})\n", fds[0], fds[1]);

    pid_t cpid = fork();
    if (cpid == -1) {
        std::print("Failed to fork process: rc={} errno={}\n", cpid, errno);
        return -1;
    }
    // printf("pid: %d\n", cpid);
    if (cpid) {
        // puts("Parent");
        {
            auto close_rc = close(fds[1]);
            if (close_rc != 0) {
                std::print("Failed to close write end of pipe in parent: rc={} errno={}\n", close_rc, errno);
                return -1;
            }
        }

        char c{0};
        int bytes_read{0};
        while ((bytes_read = read(fds[0], &c, 1)) > 0)
            std::print("{}", c);

        if (bytes_read < 0)
            std::print("[XiSh]: Failed to read output from command (errno={})\n", errno);

        {
            auto close_rc = close(fds[0]);
            if (close_rc != 0) {
                std::print("Failed to close read end of pipe in parent: rc={} errno={}\n", close_rc, errno);
                return -1;
            }
        }

        // TODO: waitpid needs to reserve some uncommon error code for
        // itself so that it is clear what is a failure from waitpid or just a
        // failing status. Maybe have some other way to check? Or wrap this in
        // libc that sets errno (that always goes well).
        fflush(NULL);
        int command_status{};
        auto wait_rc = waitpid(cpid, &command_status, 0);
        if (wait_rc == -1) {
            std::print("`waitpid` failure! on pid {}\n", cpid);
            return -1;
        }

        // puts("Parent waited");
        // fflush(NULL);

        return WEXITSTATUS(command_status);
    } else {
        // puts("Child");;
        close(fds[0]);

        // Redirect stdout to write end of pipe.
        dup2(fds[1], STDOUT_FILENO);
        close(fds[1]);

        fflush(NULL);
        execv(filepath, (char**)args);
        exit(1);
    }

    // FIXME: Unreachable

    return -1;
}

int main(int argc, char** argv) {
    // Set stdout for this program to unbuffered so the user can see updates
    // as they type.
    setvbuf(stdout, nullptr, _IONBF, BUFSIZ);

    FILE* input = stdin;
    // FIXME: This *might* be better as a vector<char>, seeing as we only add/
    // remove from the end.
    std::string input_command{};

    std::print("Welcome to XiSH\n");
    std::print("  XiSH is the main userspace shell for LensorOS.\n");
    std::print("  Try \"/fs0/bin/ls /fs0/bin\"\n");

    int rc = 0;

    std::vector<std::filesystem::path> PATH{""};
#if defined(__unix__)
    PATH.emplace_back("/usr/local/bin/");
    PATH.emplace_back("/usr/bin/");
    PATH.emplace_back("/bin/");
#elif defined(__lensor__)
    PATH.emplace_back("/fs0/bin/");
#endif

    for (;;) {
        input_command.clear();

        std::print("{}{}", rc, prompt);

        bool got_backslash = false;
        int c = 0;
        while ((c = getc(input)) != '\n') {
            // If we get end of file, spin!
            // NOTE: We should probably just quit/finish command here, but LensorOS
            // kernel had a quirk where it would return EOF when no input was
            // happening...
            if (c == EOF || feof(input)) continue;
            // Handle escape sequences

            // 2.2.1 Escape Character (Backslash)
            // https://pubs.opengroup.org/onlinepubs/9699919799/utilities/V3_chap02.html#tag_18_02_01
            if (got_backslash) {
                got_backslash = false;
                if (c == '\n') continue;
                input_command += c;
                std::print("{}", (char)c);
                continue;
            }
            if (c == '\\') {
                got_backslash = true;
                continue;
            }

            // Handle control characters
            if (c == '\b') {
                if (input_command.empty()) continue;
                input_command.erase(input_command.size() - 1);
                std::print("{}", (char)c);
                continue;
            }
            if (c == '\r') continue;
            input_command += c;
            std::print("{}", (char)c);
        }
        std::print("\n");

        static constexpr char separators[] = " \r\n\t&;";
        auto command_end = input_command.find_first_of(separators);
        if (command_end == std::string::npos)
            command_end = input_command.size();
        auto command = input_command.substr(0, command_end);
        auto the_rest = std::string_view{
            input_command.data() + command_end,
            input_command.size() - command_end};

        auto collect_arg = [](std::string_view& the_rest) -> std::string_view {
            // Skip all separators at beginning
            auto arg_offset = the_rest.find_first_not_of(separators);
            // If only separators are left, we're done parsing.
            if (arg_offset == std::string::npos) return {};
            // Skip past separators to the start of the next argument.
            the_rest = the_rest.substr(arg_offset);
            if (!the_rest.size()) return {};

            std::string_view arg_start = the_rest;
            // Find next separator.
            auto arg_end = arg_start.find_first_of(separators);
            // If the argument is zero-length, we are done.
            if (!arg_end) return {};
            // If the argument has no separators after it, the rest of
            // the input *is* the argument.
            if (arg_end == std::string::npos)
                arg_end = arg_start.size();

            // Update "the rest" so that next time we collect an arg,
            // we won't get the same one.
            the_rest = the_rest.substr(arg_end);
            return arg_start.substr(0, arg_end);
        };

        std::vector<std::string> arguments;
        for (;;) {
            auto arg = collect_arg(the_rest);
            if (!arg.size()) break;
            // TODO: Process argument (glob expansions, variable replacement, etc)
            arguments.push_back({arg.data(), arg.size()});
        }

        {
            std::print("[XiSH]: got command: \"{}\"\n", command);
            size_t index = 0;
            for (const auto& arg : arguments) {
                std::print("  arg{}: \"{}\"\n", index, arg);
                ++index;
            }
        }

        // BUILTINS
        if (command == "quit")
            break;

        // TODO: "help"

        if (command == "bg") {
            if (arguments.empty()) {
                std::print("[XiSH]:Error:builtin_background: No arguments given, so no command to run in background\n");
                continue;
            }

            command = arguments.at(0);
            arguments.erase(arguments.begin());

            if (std::filesystem::exists(std::filesystem::path{command.data()})) {
                // Prepare arguments for exec syscall
                std::vector<char*> argv;
                for (const auto& arg : arguments) {
                    argv.push_back((char*)arg.data());
                }
                argv.push_back(nullptr);

                run_program_quiet_nowait(command.data(), argv.data());
                std::print("[XiSH]: Ran \"{}\" in background\n", command);
            } else
                std::print("[XiSH]:Error:builtin_background: \"{}\" does not exist\n", command);

            continue;
        }

        // NOT A BUILTIN, DELEGATE TO SYSTEM COMMAND
        std::vector<const char*> argv;
        argv.push_back(command.data());
        for (const auto& arg : arguments) {
            argv.push_back(arg.data());
        }
        argv.push_back(nullptr);

        bool found{false};
        for (auto& p : PATH) {
            auto e = p / command.data();
            if (std::filesystem::exists(e)) {
                found = true;
                std::print("[XiSh]: found command at {}\n", e.c_str());
                rc = run_program_waitpid(e.c_str(), argv.data());
                break;
            }
        }
        if (not found)
            std::print("[XiSH]:Error: \"{}\" does not exist\n", command);
    }
    return 0;
}
