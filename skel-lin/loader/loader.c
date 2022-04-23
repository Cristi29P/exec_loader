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
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "exec_parser.h"
#include "utils.h"

static so_exec_t *exec;
static struct sigaction old_signal;
static int fd;
static int page_size;

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
	so_seg_t *segment;

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
	



}

int so_init_loader(void)
{
	struct sigaction signal_action;
	int ret;

	page_size = getpagesize();
	memset(&signal_action, 0, sizeof(struct sigaction));

	/* Set the new SIGSEGV handler */
	signal_action.sa_flags = SA_SIGINFO;
	signal_action.sa_sigaction = fault_handler;

	ret = sigemptyset(&signal_action.sa_mask);
	DIE(ret == -1, "Error sigemptyset.");

	ret = sigaddset(&signal_action.sa_mask, SIGSEGV);
	DIE(ret == -1, "Error sidaddset.");

	ret = sigaction(SIGSEGV, &signal_action, &old_signal);
	DIE(ret == -1, "Error sigaction.");	

	/* Initialization ended successfully */
	return 0;
}

int so_execute(char *path, char *argv[])
{	
	int ret;

	fd = open(path, O_RDONLY, 0644);
	DIE(fd < 0, "Error fd open.");

	exec = so_parse_exec(path);
	if (!exec)
		return -1;

	so_start_exec(exec, argv);

	/* Close the file descriptor */
	ret = close(fd);
	DIE(ret < 0, "Error close.");

	return -1;
}
