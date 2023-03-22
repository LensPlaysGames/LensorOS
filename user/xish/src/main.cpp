#include <filesystem>
#include <format>
#include <vector>

#include <stdio.h>
#include <sys/syscalls.h>
#include <unistd.h>

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

    // If there are pending writes, they will be executed on both the
    // parent and the child; by flushing any buffers we have, it ensures
    // the child won't write duplicate data on accident.
    fflush(NULL);
    pid_t cpid = syscall(SYS_fork);
    //printf("pid: %d\n", cpid);
    if (cpid) {
        //puts("Parent");
        close(fds[1]);

        // TODO: waitpid needs to reserve some uncommon error code for
        // itself so that it is clear what is a failure from waitpid or just a
        // failing status. Maybe have some other way to check? Or wrap this in
        // libc that sets errno (that always goes well).
        fflush(NULL);
        int command_status = (int)syscall(SYS_waitpid, cpid);
        if (command_status == -1) {
            // TODO: Technically, it's possible that the child has exited already.
            printf("`waitpid` failure!\n");
            return -1;
        }

        char c;
        while (read(fds[0], &c, 1) == 1 && c)
            std::print("{}", c);

        close(fds[0]);

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
    FILE *input = stdin;
    // FIXME: This *might* be better as a vector<char>
    std::string input_command;

    int rc = 0;

    for (int i = 1; i; --i) {
        input_command.clear();

        std::print("{}{}", rc, prompt);

        bool got_backslash = false;
        int c = 0;
        while ((c = getc(input)) != '\n') {
            // If we get end of file, spin!
            // NOTE: We should probably just quit/finish command here.
            if (c == EOF || feof(input)) continue;
            // Handle escape sequences

            // 2.2.1 Escape Character (Backslash)
            // https://pubs.opengroup.org/onlinepubs/9699919799/utilities/V3_chap02.html#tag_18_02_01
            if (got_backslash) {
                got_backslash = false;
                if (c == '\n') continue;
                input_command += c;
                // TODO: vt100/ansi escape sequence to set cursor, draw character.
                std::print("{}{}{}\n", rc, prompt, input_command);
                continue;
            }
            if (c == '\\') {
                got_backslash = true;
                continue;
            }

            // Handle control characters
            if (c == '\b') {
                if (!input_command.empty())
                    input_command.erase(input_command.size() - 1);
                continue;
            }
            if (c == '\r') continue;
            input_command += c;
            // TODO: vt100/ansi escape sequence to set cursor, draw character.
            std::print("{}{}{}\n", rc, prompt, input_command);
        }

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
            std::print("got command: \"{}\"\n", command);
            size_t index = 0;
            for (const auto& arg : arguments) {
                std::print("arg{}: \"{}\"\n", index, arg);
                ++index;
            }
        }

        std::vector<const char *> argv;
        for (const auto& arg : arguments) {
            argv.push_back(arg.data());
        }
        argv.push_back(nullptr);

        if (std::filesystem::exists(std::filesystem::path{command.data()})) {
            rc = run_program_waitpid(command.data(), argv.data());
        } else std::print("Error: \"{}\" does not exist\n", command);
    }
    return 0;
}
