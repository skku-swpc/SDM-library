/* Parallel Architecture & Programming Lab. */
/* Header file for queueing*/

#include <stdint.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/mman.h>

#ifndef MB
#define MB (1024*1024*8)
#endif


#define Q_scan_size		0x200000			// 256KB
//#define Q_data_size		0x8000000			// 16MB
#define Q_data_size		32*MB

#define CACHE_LINE	64
#define PAD(suffix, size)	char cachepad ##suffix [CACHE_LINE - size]
#define true		1
#define false		0

typedef struct
{
	PAD(0, 0);

	uint64_t c_index; 
	uint64_t p_next;
	PAD(1, sizeof(uint64_t)*2);

	volatile uint64_t glb_p_index; 
	PAD(2, sizeof(volatile uint64_t));
	
	volatile uint64_t glb_c_index;
	PAD(3, sizeof(volatile uint64_t));

	uint64_t p_index; 
	uint64_t c_next;
	PAD(4, sizeof(uint64_t)*2);
	
} papl_queue;

//papl_queue **queue;
papl_queue *queue;
unsigned char **Qdata;

static inline void papl_createQueue(int NPROCESS, int mode)
{
	int i;

	queue = (papl_queue *) mmap(0, (2*NPROCESS+1) * sizeof(papl_queue), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1 , 0);
	Qdata = (unsigned char **) mmap(0, (2*NPROCESS+1) * sizeof(char *), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1 , 0);
	
	/* For scan */
	for (i=0; i<NPROCESS+1; i++)
	{
		Qdata[i] = (unsigned char *) mmap(0, Q_scan_size + 1024, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1 , 0);

		queue[i].p_index = 0;
		queue[i].c_index = 0;
		queue[i].p_next = 0;
		queue[i].c_next = 0;
		queue[i].glb_p_index = 0;
		queue[i].glb_c_index = 0;
	}

	/* For merge */
	if(mode == 0) {
		for (; i<2*NPROCESS+1; i++)
		{
			// CREATE Qdata with 1MB margin space
			Qdata[i] = (unsigned char *) mmap(0, Q_data_size + MB, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1 , 0);

			queue[i].p_index = 0;
			queue[i].c_index = 0;
			queue[i].p_next = 0;
			queue[i].c_next = 0;
			queue[i].glb_p_index = 0;
			queue[i].glb_c_index = 0;
		}
	}
}

static inline void papl_destroyQueue(int NPROCESS, int mode)
{
	int i;
	/* scan */
	for (i=0; i<NPROCESS+1; i++) {
		munmap(Qdata[i], sizeof(char) * Q_scan_size);
		//munmap(queue[i], sizeof(papl_queue));
	}

	/* merge */
	if(mode == 0) {
		for (; i<2*NPROCESS+1; i++) {
			munmap(Qdata[i], sizeof(char) * Q_data_size);
			//munmap(queue[i], sizeof(papl_queue));
		}
		munmap(Qdata, (2*NPROCESS+1) * sizeof(char *));
		munmap(queue, (2*NPROCESS+1) * sizeof(papl_queue));
	} else if(mode == 1) {
		munmap(Qdata, (NPROCESS+1) * sizeof(char *));
		munmap(queue, (NPROCESS+1) * sizeof(papl_queue));
	}

}

static inline void chk_queue_Full(int pid, int size, int Qsize)
{
	if (((queue[pid].p_next - queue[pid].c_index) & (Qsize-size)) == (Qsize-size))
	{
		while (((queue[pid].p_next - queue[pid].glb_c_index) & (Qsize-size)) == (Qsize-size))
			usleep(1);
		queue[pid].c_index = queue[pid].glb_c_index;
	}
}

static inline void chk_queue_Empty(int pid)
{
	if (queue[pid].glb_c_index == queue[pid].p_index)
	{
		while (queue[pid].glb_c_index == queue[pid].glb_p_index) {
			usleep(1);
		}
		queue[pid].p_index = queue[pid].glb_p_index;
	}
}

static inline void papl_produce(int pid, void *buf, int size, int Qsize)
{
	memcpy(&(Qdata[pid][queue[pid].p_next]), buf, size);
	queue[pid].p_next += size;
	queue[pid].p_next &= Qsize -1;
	queue[pid].glb_p_index = queue[pid].p_next;
}

static inline void papl_consume(int pid, void* buf, int size, int Qsize)
{
	memcpy(buf, &(Qdata[pid][queue[pid].c_next]), size);

	queue[pid].c_next += size;
	queue[pid].c_next &= Qsize - 1;
	queue[pid].glb_c_index = queue[pid].c_next;
}

static inline void reset_queue(int NPROCESS) {

	int i;
	for(i=0; i<2*NPROCESS+1; i++) {
		queue[i].p_index = 0;
		queue[i].p_next = 0;
		queue[i].c_index = 0;
		queue[i].c_next = 0;
		queue[i].glb_p_index = 0;
		queue[i].glb_c_index = 0;
	}
}
