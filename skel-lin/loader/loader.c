/*
 * Loader Implementation
 *
 * 2018, Operating Systems
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>

#include "exec_parser.h"

static so_exec_t *exec;



void fault_handler() {

}


int so_init_loader(void)
{
	struct sigaction signal_action;

	memset(&signal_action, 0, sizeof(struct sigaction));

	signal_action.sa_flags = SA_SIGINFO;
	signal_action.sa_sigaction = fault_handler;

	sigemptyset(&signal_action.sa_mask);
	sigaddset(&signal_action.sa_mask, SIGSEGV);

	if (sigaction(SIGSEGV, &signal_action, NULL)) {
		fprintf(stderr, "Failed initialization\n");
		exit(-1);
	}

	/* Initialization ended successfully */
	return 0;
}

int so_execute(char *path, char *argv[])
{
	exec = so_parse_exec(path);
	if (!exec)
		return -1;

	so_start_exec(exec, argv);

	return -1;
}
