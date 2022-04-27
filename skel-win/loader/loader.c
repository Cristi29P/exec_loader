/*
 * Loader Implementation
 *
 * 2018, Operating Systems
 */

#include <stdio.h>
#include <Windows.h>
#include <signal.h>

#define DLL_EXPORTS
#include "loader.h"
#include "exec_parser.h"
#include "utils.h"

#define PAGE_SIZE 0x10000

static so_exec_t *exec; /* Exec pointer */
static HANDLE handle; /* File handle */

/* Returns a pointer to the segment where the address of the page fault
 * resides.
 */
static so_seg_t *get_segment(uintptr_t fault_address)
{
	so_seg_t *fault_segment = NULL;
	int i;

	for (i = 0; i != exec->segments_no; ++i) {
		if (fault_address >= (&exec->segments[i])->vaddr &&
			fault_address < (&exec->segments[i])->vaddr +
			(&exec->segments[i])->mem_size) {
			fault_segment = &exec->segments[i];
			break;
		}
	}

	return fault_segment;
}

/*
 * Read file data from segment and copy it in the newly mapped page.
 * Implementation is similar to xread/xwrite from lab2 solutions.
 */
void my_read(uintptr_t buffer, int size, int offset)
{
	int ret = 0, bytes_written = 0, bytes_actually = 0;

	ret = SetFilePointer(handle, offset, NULL, SEEK_SET);
	DIE(ret == INVALID_SET_FILE_POINTER, "SetFilePointer failed");

	while (bytes_written < size) {
		ret = ReadFile(handle, (char *)buffer + bytes_written,
		      size - bytes_written, &bytes_actually, NULL);
		DIE(ret == 0, "Error ReadFile.");

		bytes_written += bytes_actually;
	}
}

/* Returns the Windows equivalent of Linux style page permissions. */
unsigned long convert_permissions(unsigned int seg_perm)
{
	switch (seg_perm) {
	case PERM_R:
		return PAGE_READONLY;

	case PERM_W:
		return PAGE_READONLY;

	case PERM_X:
		return PAGE_READWRITE;

	case PERM_R | PERM_W:
		return PAGE_READWRITE;

	case PERM_R | PERM_X:
		return PAGE_EXECUTE_READ;

	default:
		return PAGE_NOACCESS;
	}
}

static LONG CALLBACK fault_handler(PEXCEPTION_POINTERS except_info)
{
	uintptr_t fault_address;
	so_seg_t *segment = NULL;
	EXCEPTION_RECORD *exec_rec; /* Struct containing fault info. */
	int page_index = 0, offset = 0, page_chunk = 0;
	int size_to_copy = PAGE_SIZE, ret = 0;
	void *aux = NULL, *addr = NULL;
	unsigned long old_prot_attr = 0; /* Old protection attributes */

	/* Find the segment which caused the error. */
	exec_rec = except_info->ExceptionRecord;
	fault_address = (uintptr_t)exec_rec->ExceptionInformation[1];
	segment = get_segment(fault_address);

    /* Page not found in any of program's segments, so we have an illegal
     * memory access or the program tried to perform an illegal operation
     * on a page which was already mapped.
     */
	if (!segment || (except_info->ExceptionRecord->ExceptionCode
		!= EXCEPTION_ACCESS_VIOLATION))
		return EXCEPTION_CONTINUE_SEARCH;

	/*
	 * If we reach this point it means that the program tried to
	 * access a page which has not yet been mapped.
	 */

	/* Retrieve the page index inside the segment where
	 * the fault occurred.
	 */
	page_index = ((uintptr_t) fault_address - segment->vaddr) / PAGE_SIZE;

	/* The page is already mapped, so we have an invalid
	 * permissions access.
	 */
	if (((char *)segment->data)[page_index])
		return EXCEPTION_CONTINUE_SEARCH;
	((char *)segment->data)[page_index] = 1; /* Mark the page as mapped. */

	/* Where the page starts */
	page_chunk = page_index * PAGE_SIZE;
	/* Effective address */
	addr = (void *)(segment->vaddr + page_chunk);
	/* File offset used for mapping */
	offset = segment->offset + page_chunk;

	/*
	 * Map the required page alongside file data in the address space.
	 * We initially allocate the page as readwrite in case we have a
	 * page we subsequently need to write to. We restore the segment
	 * permissions later on.
	 */
	aux = VirtualAlloc((LPVOID)addr, PAGE_SIZE,
		MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
	DIE(aux == NULL, "Error VirtualAlloc.");

	/* First half of the page lies inside the file_size.
	 * So we copy that one.
	 */
	if ((int)segment->file_size < (page_index + 1) * PAGE_SIZE)
		size_to_copy = segment->file_size - page_chunk;

	/* Read file data in page memory. */
	my_read((uintptr_t)addr, size_to_copy, offset);

	/* Restore the segment permissions for the page. */
	ret = VirtualProtect((LPVOID)addr, PAGE_SIZE,
			convert_permissions(segment->perm),
			&old_prot_attr);
	DIE(ret == 0, "Error VirtualProtect.");

	/*
	 * We took care of the fault, so we return control to the point at which
	 * the exception occurred.
	 */
	return EXCEPTION_CONTINUE_EXECUTION;
}

int so_init_loader(void)
{
	PVOID segv_handle;

	/* Set up the SIGSEGV handler via an exception vector */
	segv_handle = AddVectoredExceptionHandler(1, fault_handler);
	DIE(segv_handle == NULL, "Error AddVectoredException.");

	/* Initialization ended successfully */
	return 0;
}

void init_aux_data(void)
{
	int i, number_of_pages = 0;

	/*
	 * Go through each segment, get the number of pages and init an array
	 * that keeps track of mapped pages. Initially, the array is set to 0.
	 * No page is mapped.
	 */
	for (i = 0; i != exec->segments_no; ++i) {
		/*
		 * A segment is not paged aligned. So we need to add 1 if
		 * segment.mem_size is not a multiple of PAGE_SIZE.
		 */
		number_of_pages = (exec->segments[i].mem_size / PAGE_SIZE) +
			(exec->segments[i].mem_size % PAGE_SIZE ? 1 : 0);

		exec->segments[i].data = calloc(number_of_pages, sizeof(char));
		DIE(!(exec->segments[i].data), "Error calloc.");
	}
}

int so_execute(char *path, char *argv[])
{
	/* Open the file for mapping. */
	handle = CreateFile((LPCSTR)path, GENERIC_READ,
			FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
			OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	DIE(handle == INVALID_HANDLE_VALUE, "Error CreateFile.");

	exec = so_parse_exec(path);
	if (!exec)
		return -1;

	/* Init the auxiliary array for keeping track of mapped pages. */
	init_aux_data();

	so_start_exec(exec, argv);

	return -1;
}
