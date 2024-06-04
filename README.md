# BF-compiler
BF-compiler is a [Brainf**k](https://en.wikipedia.org/wiki/Brainfuck) compiler made in C++. It follows the initial specification and supports the standard instructions `<>.,-+[]`. The data tape size is 30000 bytes but that's easily adjustable.

# Prerequisites
For this program to run, a few tools must be installed on the user's computer. These applications are listed below.

 1. NASM (Netwide Assembler) - Assembler
 2. ld (GNU binutils) - Linker
 3. An operating system that supports the Linux syscalls.
# Compilation & Execution
To compile the **compiler**, run the following command: `g++ -Wall -Wextra compiler.cpp -o compiler`
Now, to compile a **Brainf\*\*k program** run the following command **in the same directory as the compiler binary**: `./compiler <myprogram.bf> <myprogrambinary>`

# How does this work?
The compiler works by "translating" a Brainf**k program into a temporary assembly file, which it then assembles and links, generating a new binary file. A small optimization occurs before a file is converted into assembly. Identical instructions are compiled into one, which decreases file size, but the performance gain is probably very little.
