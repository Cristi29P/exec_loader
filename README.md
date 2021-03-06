* Name: Cristian-Tanase Paris 331CAb 

# Homework <NR> 3 Executable Loader

General structure
-

* The homework consists of 3 main parts: the handler binding function, the
SIGSEGV handler and the look-up function, whose main purpose is to retrieve
a pointer to the segment where the page fault occurred.
* I found this assignment particularly useful because it cleared up some things
regarding the mapping process, executable structure and page faults that I 
didn't quite understand during the lectures.
* From my point of view, I think the assignment has been implemented as efficient
as possible, in a way that is easy to understand. 

Implementation
-

* The entire functionality has been implemented.
* For the sake of simplicity and accessibility, the file descriptor, the page
size and the old signal handler have been declared as static variables outside
the scope of any function.
```
get_segment
```
* Receives a virtual address and returns a pointer to the segment it belongs to
or NULL otherwise.

```
so_init_loader
```
* We initialize the page size and associate a new handler for SIGSEGV.

```
fault_handler
```
* We extract the virtual address that caused the fault from the info structure
and get a pointer to the segment containing the address.
* We check if the cause of the SIGSEGV is because the program tried to access
a page within an unknown segment or the segment was accessed with illegal 
permissions. In both cases, we run the old handler (the default one), which was
saved during the initialization stage.
* If the program tried to access a page which has not yet been mapped, we 
calculate the page index within the segment, the offset inside the file where 
should we start to copy data from and the effective address where we are
supposed to map the page.
* We check for corner cases:
    1.  The page is completely outside the file range.
		This means the entire memory between segment->file_size and
		segment->mem_size needs to be zeroed. In order to do this more
		efficiently, we use the MAP_ANON flag. (which zeroes the page by default).
    2. Only a part of the page sits outside the file range.
	   We need to zero the rest of the page, so we calculate the remainder.
* We map the page in memory alongside the file data (mmap can do both operations
at the same time) and memset the rest of the page which has no data.

* Difficulties:
    1. Debugging was kinda hard.
    2. Resolving the corner cases and having the right conditions in the handler
    function.

* Interesting facts:
    1. There was no need for an auxiliary data structure, because the 
    ```siginfo_t``` has the ```si_code``` field that states the cause of the 
    SIGSEGV (unmapped page or illegal access).
    2. No need to memset an entire page if it resides outside the file size
    range. The flag ```MAP_ANON``` or ```MAP_ANONYMOUS``` passed to ```mmap```
    can do that for you.

How should I compile and run this library?
-
```
make
```

This should generate the `libso_loader.so` library. Next, build the example:

```
make -f Makefile.example
```

This should generate the `so_exec` and `so_test_prog` used for the test:

```
LD_LIBRARY_PATH=. ./so_exec so_test_prog
```

Resources
-
* Lab 4: [Signal manipulation](https://ocw.cs.pub.ro/courses/so/laboratoare/laborator-04)
* Lab 5: [Memory overview](https://ocw.cs.pub.ro/courses/so/laboratoare/laborator-05)
* Lab 6: [Virtual memory](https://ocw.cs.pub.ro/courses/so/laboratoare/laborator-06)
* Operating Systems official GitHub: [Makefile and file skeleton](https://github.com/systems-cs-pub-ro/so/tree/master/assignments/3-loader)
* Linux man pages: [Link](https://linux.die.net/man/)
* Operating Systems Concepts: [Book](https://cloudflare-ipfs.com/ipfs/bafykbzaceavsju4l3yz7sbukzvmdxvaxtxvtceimf5hl2oesunfqaik3tlthq?filename=Abraham%20Silberschatz%2C%20Greg%20Gagne%2C%20Peter%20B.%20Galvin%20-%20Operating%20System%20Concepts-Wiley%20%282018%29.pdf)
* The Linux Programming Interface: [Book](https://man7.org/tlpi/)
* The ELF file format: [Wiki](https://en.wikipedia.org/wiki/Executable_and_Linkable_Format)
* Memory-mapped files: [Wiki](https://en.wikipedia.org/wiki/Memory-mapped_file)
* Demand paging: [Wiki](https://en.wikipedia.org/wiki/Demand_paging)
* Page fault: [Wiki](https://en.wikipedia.org/wiki/Page_fault)

Git
-
* Link: [exec-loader](https://github.com/Cristi29P/exec_loader.git) - repository
should be public after the 30th of April. Otherwise, please contact me on
Microsoft Teams or e-mail.