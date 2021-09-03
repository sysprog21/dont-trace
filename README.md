# dont\_trace

`dont_trace` is a simple Linux kernel module that kills ptrace tracer and its tracees.

This kernel module relies upon the Linux kernel `task_struct`'s `ptrace`
[member](https://elixir.bootlin.com/linux/latest/source/include/linux/sched.h#L661) to
detect whether a debugger is present or not.

Once any process starts "ptracing" another, the [tracee](https://man7.org/linux/man-pages/man2/ptrace.2.html)
gets added into [ptraced](https://elixir.bootlin.com/linux/latest/source/include/linux/sched.h#L867)
that is located in `task_struct`, which is simply a linked list that contains
all the tracees that the process is "ptracing".

Once a tracer is found, the module lists all its tracees and sends a `SIGKILL`
signal to each of them including the tracer. This results in killing both the
tracer and its tracees.
Once the module is attached to the kernel, the module's "core" function will
run periodically through the advantage of workqueues. Specifically, the module
runs every `JIFFIES_DELAY`, which is set to 1. That is, the module will run
every one jiffy.

## License

`dont_trace` is released under the MIT license. Use of this source code is governed
by a MIT-style license that can be found in the LICENSE file.
