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

void fault_handler(int signum, siginfo_t *info, void *context) {
	
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
