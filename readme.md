# Unix userland as C++ coroutines

A very basic Unix shell implementation which does not spawn any processes
but instead implements binary functionality with cooperative multitasking

## Limitations

Only a few commands are implemented. There are no background processes,
stdio redirection apart from the pipe, environment variables or the like.
Spawning external programs is also not supported.

## Running

Compile the app and run `babyshell`. Then type commands such as:

    cd subdir
    ls
    cd ..
    cat build.ninja | sort | uniq
    ls | grep build

None of the commands takes file arguments so you need to use `cat`.

To end the shell type `exit` to quit or just hit `ctrl-c`.
