*Este proyecto ha sido creado como parte del currículo de 42 por jaialons.*

# Codexion

## Description

Codexion is a multithreaded simulation focused on resource sharing, scheduling and thread synchronization.

The goal of the project is to simulate a group of coders competing for a limited set of shared dongles. Each coder is represented by a thread and must acquire two dongles before being able to compile. After compiling, the coder releases both dongles, debugs, refactors, and then tries to compile again.

The simulation ends when either one coder burns out or all coders complete the required number of compilations.

This project explores typical concurrency problems such as mutual exclusion, deadlock prevention, starvation prevention, cooldown management, precise monitoring and thread-safe logging.

## Instructions

### Compilation

Compile the project with:

```bash
make
```

Remove object files with:

```bash
make clean
```

Remove object files and the executable with:

```bash
make fclean
```

Rebuild the project with:

```bash
make re
```

### Execution

The program is executed with eight arguments:

```bash
./codexion number_of_coders time_to_burnout time_to_compile time_to_debug time_to_refactor number_of_compiles_required dongle_cooldown scheduler
```

Example using FIFO scheduling:

```bash
./codexion 5 800 100 100 100 3 50 fifo
```

Example using EDF scheduling:

```bash
./codexion 5 800 100 100 100 3 50 edf
```

The last argument must be one of:

```txt
fifo
edf
```

### Arguments

| Argument | Description |
|---|---|
| `number_of_coders` | Number of coder threads. |
| `time_to_burnout` | Maximum time a coder can spend without starting a new compilation. |
| `time_to_compile` | Time spent compiling. |
| `time_to_debug` | Time spent debugging. |
| `time_to_refactor` | Time spent refactoring. |
| `number_of_compiles_required` | Number of compilations required for each coder. |
| `dongle_cooldown` | Cooldown time before a released dongle can be reused. |
| `scheduler` | Scheduling policy: `fifo` or `edf`. |

### Error handling

Invalid arguments print:

```txt
Error
```

Invalid cases include:

- wrong number of arguments;
- negative numbers;
- non-numeric values where numbers are expected;
- invalid scheduler name;
- zero coders;
- zero or invalid burnout time;
- zero required compilations.

## Repository structure

```txt
.
├── Makefile
├── codexion.c
├── codexion.h
└── src
    ├── parser.c
    ├── utils.c
    ├── init.c
    ├── init_objects.c
    ├── destroy.c
    ├── time.c
    ├── log.c
    ├── coder_state.c
    ├── request.c
    ├── heap_order.c
    ├── heap.c
    ├── dongle.c
    ├── dongle_release.c
    ├── cycle.c
    ├── routine.c
    ├── monitor.c
    └── threads.c
```

## Scheduler

Each dongle owns a request heap. When a coder wants to acquire a dongle, it creates a request and inserts it into the corresponding dongle heap.

A dongle can only be granted when:

- the dongle is available;
- the dongle cooldown has expired;
- the request is the highest-priority request in the heap.

Two scheduling policies are supported: FIFO and EDF.

### FIFO

FIFO means First In, First Out.

With this policy, the oldest request has the highest priority. The request with the earliest request timestamp is served first.

### EDF

EDF means Earliest Deadline First.

With this policy, the request with the earliest deadline has the highest priority. The deadline is based on the coder state and the burnout limit, so coders closer to burning out are prioritized.

## Blocking cases handled

### Deadlock prevention

The implementation reduces circular waiting by making coders acquire dongles in different orders depending on their identifier.

Odd and even coders do not all request dongles in the same order. This avoids the classic deadlock pattern where every coder holds one dongle while waiting forever for another one.

This directly addresses one of the Coffman conditions for deadlock: circular wait.

### Starvation prevention

Each dongle uses a heap of pending requests. A coder cannot take a dongle unless its request is the highest-priority request in that dongle heap.

With FIFO, older requests are prioritized. This prevents newer requests from repeatedly overtaking older ones.

With EDF, requests with the earliest deadline are prioritized. This helps coders that are closer to burnout obtain resources earlier.

### Dongle cooldown management

When a dongle is released, its release timestamp is stored.

A future request can only acquire that dongle once the cooldown time has passed. This prevents immediate reuse of a dongle when `dongle_cooldown` is greater than zero.

The cooldown check is protected by the dongle mutex, so the availability flag and release timestamp remain consistent.

### One coder case

If there is only one coder, there is only one dongle.

Since compiling requires two dongles, the coder can acquire one dongle but can never acquire a second one. The monitor eventually detects burnout and stops the simulation.

### Burnout while waiting

A coder can burn out while waiting for the first dongle, while waiting for the second dongle, or while performing actions.

The monitor thread continuously checks the elapsed time since each coder last started compiling. If this elapsed time reaches `time_to_burnout`, the monitor logs the burnout event and stops the simulation.

### Precise burnout detection

The monitor runs independently from coder threads. It regularly checks all coders and compares the current timestamp with each coder's last compile timestamp.

The timestamp update is protected by the state mutex. This ensures that the monitor reads a consistent value and can detect burnout without racing against coder threads.

### Log serialization

All output is protected by a print mutex.

This prevents messages from different threads from being printed at the same time or being mixed together in the terminal.

## Thread synchronization mechanisms

The project uses POSIX threads and mutexes.

### `pthread_t`

Each coder is represented by one `pthread_t`.

The monitor is also represented by a separate `pthread_t`.

This allows coders and the monitor to run concurrently.

### `pthread_mutex_t` for dongles

Each dongle has its own mutex.

This mutex protects:

- the dongle availability flag;
- the dongle cooldown timestamp;
- the dongle request heap.

When a coder tries to take a dongle, it locks the dongle mutex, inserts or checks its request, verifies cooldown and availability, and only then takes the dongle if its request has priority.

This prevents race conditions where two coders could take the same dongle at the same time.

### `pthread_mutex_t` for simulation state

The simulation has a state mutex.

This mutex protects:

- the global `finished` flag;
- each coder compilation counter;
- each coder last compile timestamp.

Coder threads update their own state through this mutex. The monitor also reads coder state through this mutex.

This creates thread-safe communication between coders and the monitor.

Example: when a coder starts compiling, it updates `last_compile_time` while holding the state mutex. The monitor later reads this value while holding the same mutex, avoiding a data race.

### `pthread_mutex_t` for logging

The simulation has a print mutex.

Before printing a log message, a thread locks the print mutex. After printing, it unlocks it.

This guarantees that log lines are serialized and remain readable.

### Communication between coders and monitor

The monitor does not directly interrupt coder threads. Instead, it sets the shared `finished` flag when the simulation must stop.

Coder threads regularly check this flag through a thread-safe function protected by the state mutex.

This means coders can stop cleanly after the monitor detects burnout or after all required compilations are completed.

### Custom scheduling mechanism

The project implements a custom event-like mechanism using request heaps.

A request represents a coder waiting for a dongle. Each dongle heap orders these requests according to the selected scheduling policy.

The heap is always accessed while holding the dongle mutex, so request insertion, removal and priority checks are thread-safe.

## Resources

### Technical references

- POSIX Threads documentation.
- Linux manual pages for `pthread_create`, `pthread_join`, `pthread_mutex_init`, `pthread_mutex_lock`, `pthread_mutex_unlock` and `pthread_mutex_destroy`.
- Linux manual pages for `gettimeofday` and `usleep`.
- Valgrind Memcheck documentation.
- Valgrind Helgrind documentation.
- Operating systems material related to mutual exclusion, deadlocks, starvation and scheduling.
- Classical scheduling concepts: FIFO and Earliest Deadline First.

### Artificial intelligence usage

Artificial intelligence was used as an assistant during the development process.

It was used for:

- discussing the project architecture;
- reviewing possible synchronization strategies;
- helping split the code into smaller files;
- checking whether the implementation respected the 42 Norm style constraints;
- preparing test commands;
- drafting explanations for the README.

The final code decisions, testing, compilation, debugging and validation were performed by the project author.

AI was not used as a replacement for understanding the synchronization model. The implementation was manually reviewed and tested with `make`, Valgrind Memcheck and Valgrind Helgrind.

## Testing

The project was tested with normal execution cases, burnout cases, invalid argument cases and thread-checking tools.

### Normal execution

```bash
./codexion 5 800 100 100 100 3 50 fifo
./codexion 5 800 100 100 100 3 50 edf
```

### Burnout case

```bash
./codexion 1 100 200 20 200 3 0 fifo
```

### Invalid arguments

```bash
./codexion
./codexion 5 800 100 100 100 3 50
./codexion 5 800 100 100 100 3 50 bad
./codexion -5 800 100 100 100 3 50 fifo
./codexion 5 abc 100 100 100 3 50 fifo
./codexion 0 800 100 100 100 3 50 fifo
```

### Memory checking

The project was checked with Valgrind Memcheck.

Example command:

```bash
valgrind --leak-check=full --show-leak-kinds=all ./codexion 5 800 100 100 100 3 50 fifo
```

The tested output showed:

```txt
All heap blocks were freed -- no leaks are possible
ERROR SUMMARY: 0 errors
```

### Thread checking

The project was checked with Valgrind Helgrind using both FIFO and EDF scheduling modes.

Example commands:

```bash
valgrind --tool=helgrind ./codexion 5 800 100 100 100 3 50 fifo
valgrind --tool=helgrind ./codexion 5 800 100 100 100 3 50 edf
```

The tested output showed:

```txt
ERROR SUMMARY: 0 errors
```