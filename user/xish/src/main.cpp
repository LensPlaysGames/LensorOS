#include <filesystem>
#include <format>
#include <vector>

#include <stdio.h>
#include <sys/syscalls.h>
#include <unistd.h>
#include <bits/io_defs.h>

constexpr const char prompt[] = "  $:";

// FIXME: May want to do ErrorOr or some type of variant so that we can
// tell when run_program_waitpid itself failed vs the program that was
// run failing.
/// @param filepath Passed to `exec` syscall
/// @param args
///   NULL-terminated array of pointers to NULL-terminated strings.
///   Passed to `exec` syscall
int run_program_waitpid(const char *const filepath, const char **args) {
    size_t fds[2] = {size_t(-1), size_t(-1)};
    syscall(SYS_pipe, fds);
    //std::print("[XiSh]: Created pipe: ({}, {})\n", fds[0], fds[1]);

    pid_t cpid = fork();
    //printf("pid: %d\n", cpid);
    if (cpid) {
        //puts("Parent");
        close(fds[1]);

        char c;
        while (read(fds[0], &c, 1) != EOF && c)
            std::print("{}", c);

        close(fds[0]);

        // TODO: waitpid needs to reserve some uncommon error code for
        // itself so that it is clear what is a failure from waitpid or just a
        // failing status. Maybe have some other way to check? Or wrap this in
        // libc that sets errno (that always goes well).
        fflush(NULL);
        int command_status = syscall<int>(SYS_waitpid, cpid);
        if (command_status == -1) {
            std::print("`waitpid` failure!\n");
            return -1;
        }

        //puts("Parent waited");
        //fflush(NULL);

        return command_status;

    } else {
        //puts("Child");;
        close(fds[0]);

        // Redirect stdout to write end of pipe.
        syscall(SYS_repfd, fds[1], STDOUT_FILENO);
        close(fds[1]);

        fflush(NULL);
        syscall(SYS_exec, filepath, args);
    }

    // FIXME: Unreachable

    return -1;
}

int main(int argc, char **argv) {
    // Set stdout unbuffered so the user can see updates as they type.
    setvbuf(stdout, nullptr, _IONBF, BUFSIZ);

    FILE *input = stdin;
    // FIXME: This *might* be better as a vector<char>
    std::string input_command{};

    std::print("Welcome to XiSH\n");

    int rc = 0;

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
            input_command.size() - command_end
        };

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

        // NOT A BUILTIN, DELEGATE TO SYSTEM COMMAND
        std::vector<const char *> argv;
        for (const auto& arg : arguments) {
            argv.push_back(arg.data());
        }
        argv.push_back(nullptr);

        if (std::filesystem::exists(std::filesystem::path{command.data()}))
            rc = run_program_waitpid(command.data(), argv.data());
        else std::print("[XiSH]:Error: \"{}\" does not exist\n", command);
    }
    return 0;
}
