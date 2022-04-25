/*
 * Loader Implementation
 *
 * 2018, Operating Systems
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "exec_parser.h"
#include "utils.h"

static so_exec_t *exec;
static struct sigaction old_signal;
static int fd;
static int page_size;

/* Returns a pointer to the segment where the address of the page fault resides */
static so_seg_t *get_segment(uintptr_t fault_address)
{
	so_seg_t *fault_segment = NULL;

	for (int i = 0; i != exec->segments_no; ++i) {
		if (fault_address >= (&exec->segments[i])->vaddr &&
		fault_address < (&exec->segments[i])->vaddr + (&exec->segments[i])->mem_size) {
			fault_segment = &exec->segments[i];
			break;
		}
	}

	return fault_segment;
}

static void fault_handler(int signum, siginfo_t *info, void *context) {
	uintptr_t fault_address;
	so_seg_t *segment = NULL;
	int page_index = 0, dif = 0, flags = MAP_PRIVATE | MAP_FIXED, offset = 0;
	int page_chunk = 0;
	char *aux = NULL;
	void *addr = NULL;

	/* Find the segment which caused the error. */
	fault_address = (uintptr_t)info->si_addr;
	segment = get_segment(fault_address);
	
	/* 1. Page not found in any of program's segments, so we have an illegal
     * memory access.
	 * OR
	 * 2. The program tried to perform an illegal operation on a page which
	 * was already mapped.
	 */
	if (!segment || info->si_code == SEGV_ACCERR) {
		old_signal.sa_sigaction(signum, info, context);
		return;
	}

	/* If we reach this point it means that the program tried to access a page
	 * which has not yet been mapped.
	 */

	/* Retrieve the page index inside the segment where the fault occured */
	page_index = ((uintptr_t) info->si_addr - segment->vaddr) / page_size;
	page_chunk = page_index * page_size; /* Where the page starts*/
	addr = (void *) segment->vaddr + page_chunk; /* Effective address */
	offset = segment->offset + page_chunk; /* File offset used for mapping */

	/* Corner case */
	if (segment->file_size < segment->mem_size) {
		/* Case 1: The page is completely outside the file range.
		 * This means the entire memory between segment->file_size and 
		 * segment->mem_size needs to be zeroed. In order to do this more
		 * efficiently we use the MAP_ANON flag.
		 */
		if (segment->file_size < page_chunk) {
			flags |= MAP_ANON;
		}
		/* Case 2: Only a part of the page sits outside the file range.
		 * We need to zero the rest of the page, so we calculate the remainder.
		 *
		 */
		if (segment->file_size < (page_index + 1) * page_size) {
			dif = (page_index + 1) * page_size - segment->file_size;	
		}
	}
	
	/* Map the requiered page alongside file data in the address space */
	aux = mmap(addr, page_size, segment->perm, flags, fd, offset);
	DIE(aux == MAP_FAILED, "Error mmap.");

	/* Zero the rest of the page which is not backed-up by a file mapping */
	if (dif)
		memset((char *)(segment->vaddr + page_index * page_size + (page_size - dif)),0, dif);
}

int so_init_loader(void)
{
	struct sigaction signal_action; /* New signal handler */
	int ret = 0;

	page_size = getpagesize();
	memset(&signal_action, 0, sizeof(struct sigaction));

	/* Set the new SIGSEGV handler */
	signal_action.sa_flags = SA_SIGINFO;
	signal_action.sa_sigaction = fault_handler;

	ret = sigemptyset(&signal_action.sa_mask);
	if (ret) exit(-1);

	/* Mask any other SIGSEGV during handler operation */
	ret = sigaddset(&signal_action.sa_mask, SIGSEGV);
	DIE(ret < 0, "Error sigaddset.");

	/* Associate the new handler for SIGSEGV */
	ret = sigaction(SIGSEGV, &signal_action, &old_signal);
	DIE(ret < 0, "Error sigaction.");	

	/* Initialization ended successfully */
	return 0;
}

int so_execute(char *path, char *argv[])
{	
	/* Open the file for mapping */
	fd = open(path, O_RDONLY, 0644);
	if (fd < 0) {
		fprintf(stderr, "Fd error.\n");
		exit(-1);
	}

	exec = so_parse_exec(path);
	if (!exec)
		return -1;

	so_start_exec(exec, argv);

	return -1;
}
