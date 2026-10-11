## Homework: isolate the command parser

### What the exercise emulates

A connected device receives commands from the network and parses them. The same firmware
stores the device's private key, which the connection code uses to authenticate the device.

The command `get=<n>` returns setting number `n`. The parser does not check `n` against the
size of the settings table. An attacker who sends a large `n` makes the parser read memory
past the table, and with the right values, the device key.

### What is realistic and what is not

Realistic:
- **An out-of-bounds read with a value taken from a packet.** This is a common bug class.
  Example: Heartbleed (2014) was one, and it leaked TLS private keys from servers.
- **The attacker knowing where the key is.** Every device runs the same firmware, so the key
  is at the same address on every unit. An attacker finds that address by analysing a
  firmware image.
- **Isolating code that handles untrusted input,** as a second line of defence against bugs
  that were missed.

Simplifications:
- The packets are hardcoded in `main.c` instead of coming from the network.
- `main.c` computes the attacker's index from the real addresses. A real attacker would get
  it from the firmware's ELF file or in a different way.
- The key is labelled `EC-PRIVATE-KEY:`, which is pretty obvious to make the exercise
  understandable. A real attacker recognises a key by its format, or by checking it
  against the device's public certificate.

### Starter project

| File | Contents |
|---|---|
| `src/credentials.c` | Stores the device key. Do not modify this file. |
| `src/parser.c` | Parses commands, including the bug. Do not modify this file. |
| `src/main.c` | Starts the parser thread and feeds it one normal command and two attack commands |

Run it with `./run.sh` (see [SETUP.md](SETUP.md)). The output shows the normal setting,
then the full key in two parts.

### Task

Change the project so the parser can no longer read the key. The normal command must still
work.

Do not modify `parser.c` or `credentials.c`. Finding and fixing bugs like this one is the
developer's job. But some bugs get missed and reach production, and the exercise assumes
this one did. Memory isolation limits what a bug or vulnerability can reach, so the
protection must come from how the code runs, not from fixing this particular bug.

### Expected result

- `get=0` still prints `led=off`.
- The first attack command causes an MPU fault. The fault address is the key's address.
- The key is never printed.

### Hints

1. Which parts of this firmware handle untrusted data?
2. Should the parser thread be a kernel or a user-space thread?
3. What can provide memory isolation in Zephyr? See the "Isolation Features in the Kernel"
   slide.
