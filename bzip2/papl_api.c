#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <string.h> 	// memcpy

#include <sys/types.h>
#include <sys/wait.h>	// waitpid

#include <fcntl.h>		// for mmap
#include <errno.h>
#include <sys/mman.h>

#include <sys/shm.h>
#include "papl.h"

void init(void);
void configure(void);
void create_process(void *(*func)(void*), pid_t*, void*);
void join_process(pid_t*);
void recover_misspec(long, pid_t *, struct PAPL);
void *scan(void *);
void *decompress(void *);
void merge(pid_t*, struct PAPL);
void InsertNode(int, unsigned long, int, unsigned char *);
void Mergebuffer(FILE *);
MergeNode *m_Head = NULL;
MergeNode *m_Tail = NULL;

void papl_exec() {

	int i;
	/* Read Configuration file */
	configure();

	/* Create Queue */
	init();

	/* Clock_gettime */
	init_time();

	//pid_t scan_pid, *decomp_pids;
	//decomp_pids = (pid_t*)malloc(sizeof(pid_t) * conf.num_process);
	pid_t *pids;
	pids = (pid_t*)malloc(sizeof(pid_t) * (conf.num_process+1));

 	/* Create SCAN process */
 	if(conf.merge_method == CENT)
 		create_process(scan, &pids[0], NULL);
 
 	/* Create DECOMPRESS process */
 	for(i=1; i<conf.num_process+1; i++) {
 		conf.pid = i - 1;
 		create_process(decompress, &pids[i], &i);
 	}
 
 	/* MERGE */
 	if(conf.merge_method == CENT)
 		merge(pids, conf);
 	/* SCAN */
 	else
 		scan(NULL);
 
 	/* Wait to join processes */ 
 	join_process(pids);

	free(pids);
	/* Destroy Queue */
	papl_destroyQueue(conf.num_process, conf.merge_method);


	if(conf.merge_method == DIST)
		munmap(token, conf.num_process * sizeof(int));

}

void init(void) {
	int i;
	FILE *fp;
	fp = fopen(conf.input_name, "rb");
	fseek(fp, 0L, SEEK_END);
	conf.file_size = ftell(fp);

	if(conf.merge_method == DIST) {
		//temp_buffer = malloc(conf.file_size * sizeof(char));
		//fread(temp_buffer, sizeof(char), conf.file_size, fp);	// whole file read

// 		gz_papl = (struct gz_papl_t *) malloc( sizeof(struct gz_papl_t) * TOTAL_BUFFER / conf.chunk_size);
// 		for(i=0; i<TOTAL_BUFFER / conf.chunk_size; i++) {
// 			memset(&gz_papl[i], 0, sizeof(*gz_papl));
// 			gz_papl[i].pos = 0;
// 			//gz_papl[i].buf = (unsigned char*)malloc(conf.file_size * BUFFER_SIZE);
// 			gz_papl[i].buf = (unsigned char*)malloc(BUFFER_SIZE * conf.chunk_size);
// 		}

		token = (int*) papl_mmap(conf.num_process * sizeof(int));
		memset(token, 0, conf.num_process * sizeof(int));
		token[0] = 1;

	}
	papl_createQueue(conf.num_process, conf.merge_method);
	fclose(fp);
}

void create_process(void *(*func)(void*), pid_t *pid, void *arg) {
	*pid = fork();
	if(*pid == -1) {
		fprintf(stderr, "  ERROR @ fork\n");
		exit(-1);
	} else if (*pid == 0) {
		func(arg);
	}
}

void join_process(pid_t *pids) {
	int i;

	if(conf.merge_method == CENT) {
		waitpid(pids[0], NULL, 0);
		pt0("SCAN process joined\n");
	}

	for(i=1; i<conf.num_process+1; i++) {
		waitpid(pids[i], NULL, 0);
		pt1("DECOMP[%2d] process joined\n", i-1);
	}
}

void *recover_func(void* arg) {
	long pos;
	struct PAPL *tmp = (struct PAPL*)arg;
	conf.pid = tmp->pid;
	conf.num_process = tmp->num_process;

	conf.input_stream = fopen(conf.input_name, "rb");
	consume_scan(conf.pid, &pos, sizeof(long));
	conf.decompress(conf.pid, pos);
	fclose(conf.input_stream);
	exit(0);
}

void recover_misspec(long pos, pid_t *pids, struct PAPL conf) {
	int i, pid = 0;
	pid_t rec;
	unsigned int len;
	struct decomp_chunk_t decomp_chunk;

	printf("\n  ERROR misspeculation detected.\n\n");
	kill(pids[0], SIGKILL);		// kill scan
	for(i=1;i<conf.num_process+1;i++) {
		kill(pids[i], SIGKILL);		// kill decomps
	}

	reset_queue(conf.num_process);

	int prev_np = conf.num_process;
	conf.pid = pid;
	conf.num_process = 1;
	fflush(conf.output_stream);
	create_process(recover_func, &rec, &(conf));
	//create_process(decompress, &rec, &(conf));
	produce_scan(pid, &pos, sizeof(long));

	while(true) {
		chk_queue_Empty(pid + conf.num_process + 1);
		consume_chunk(pid, &len, sizeof(len));
		if( len == EOC ) break;
		else {
			consume_chunk(pid, &decomp_chunk, sizeof(decomp_chunk) - sizeof(int));
			fwrite(decomp_chunk.buf, sizeof(char), len, conf.output_stream);
			//pt2("    @@@@ fwrite %d %u\n", pid, len);
		}
	}

	waitpid(rec, NULL, 0);
	papl_destroyQueue(prev_np, conf.merge_method);
	exit(0);
}
void *scan(void *arg) {
	pt2("@@@@ SCAN(%d) created: %f sec\n", getpid(), ttime);
	int decomp_pid, eof;
	unsigned long next_pos;
	int idx = 0;
	eof = decomp_pid = 0;

	while(true) {
		next_pos = conf.scan(conf.input_stream, &eof);
		if(eof) break;
		if (!(idx%conf.chunk_size)) {
			produce_scan(decomp_pid, &next_pos, sizeof(long));
			produce_scan(MERGE, &next_pos, sizeof(long));

			//pt3("  SCAN send %ld to pid %d: %f sec\n", next_pos, decomp_pid, ttime);
			decomp_pid++;
			if(decomp_pid == conf.num_process) decomp_pid = 0;
		}
		idx++;
	}

	long flag = EOS;
	for(decomp_pid = 0; decomp_pid < conf.num_process; decomp_pid++) {
		produce_scan(decomp_pid, &flag, sizeof(long));
		//pt2("  SCAN to pid %d EOS: %f\n", decomp_pid, ttime);
	}

	flag = 0;
	produce_scan(MERGE, &flag, sizeof(long));

	pt2("@@@@ SCAN(%d) exit: %f sec\n", getpid(), ttime);
	if(conf.merge_method == DIST) return 0;
	//sleep(10);
	exit(0);
}

void *decompress(void* arg) {
	pt2("@@@@ DECOMP(%d) created: %f sec\n", getpid(), ttime);

	unsigned long pos;

	conf.pid = *(int*) arg - 1;
	conf.input_stream = fopen(conf.input_name, "rb");

	/* Centalized */
	if(conf.merge_method == CENT) {
		int flag = EOD;
		while(true) {
			consume_scan(conf.pid, &pos, sizeof(long));
			if(pos == EOS) break;
			//pt3("    DECOMP[%d] start %ld: %f sec\n", conf.pid, pos, ttime);
			conf.decompress(conf.pid, pos);
			//pt3("    DECOMP[%d] finish %ld: %f sec\n", conf.pid, pos, ttime);

		}
		produce_chunk(conf.pid, &flag, sizeof(int));
	}

	/* Distributed */
	else if(conf.merge_method == DIST) {

		/* INIT */

		int i;
		gz_papl = (struct gz_papl_t *) malloc( sizeof(struct gz_papl_t) * TOTAL_BUFFER / conf.chunk_size);

		for(i=0; i<TOTAL_BUFFER / conf.chunk_size; i++) {
			memset(&gz_papl[i], 0, sizeof(*gz_papl));
			gz_papl[i].pos = 0;
			//gz_papl[i].buf = (unsigned char*)malloc(conf.file_size * BUFFER_SIZE);
			gz_papl[i].buf = (unsigned char*)malloc(BUFFER_SIZE * conf.chunk_size);
		}
		buffer_index = 0;
		//token = (int*) mmap(0, conf.num_process * sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1 ,0);

		buff = malloc(4*MB);
		/* INIT end */

		int merge_index = 0;
		while(true) {
			if(pos != EOS) {
				consume_scan(conf.pid, &pos, sizeof(long));
				if(pos == EOS) goto merge;
				//pt3("    DECOMP[%d] start %ld: %f sec\n", conf.pid, pos, ttime);
				conf.decompress(conf.pid, pos);
				//pt3("    DECOMP[%d] finish %ld: %f sec\n", conf.pid, pos, ttime);
				InsertNode(merge_index, pos, gz_papl[merge_index].pos, gz_papl[merge_index].buf);
				merge_index++;
			}
merge:
			if(m_Head == NULL) { 
 				for(i=0; i<TOTAL_BUFFER / conf.chunk_size; i++)
 					free(gz_papl[i].buf);
 				free(gz_papl);
				break;
			}
			if(token[conf.pid] && m_Head != NULL) {
				Mergebuffer(conf.output_stream);
				token[conf.pid] = 0;
				if(conf.pid + 1 == conf.num_process)
					token[0] = 1;
				else token[conf.pid + 1] = 1;
			}
		}
	}

	//sleep(10);
	fclose(conf.input_stream);
	pt2("@@@@ DECOMP(%d) exit %f sec\n", getpid(), ttime);
	exit(0);
}

inline void merge(pid_t *pids, struct PAPL conf) {

	pt2("@@@@ MERGE(%d) created: %f sec\n", getpid(), ttime);
	int pid = 0;
	unsigned long pred_pos;
	unsigned int len;
	//unsigned int total_len;
	struct decomp_end_t decomp_end;
	struct decomp_chunk_t decomp_chunk;

	buff = malloc(4*MB);

	// don't need this information
	consume_scan(MERGE, &pred_pos, sizeof(long));

	while(true) {
		chk_queue_Empty(pid + conf.num_process + 1);
		consume_chunk(pid, &len, sizeof(len));
		if( len == EOD ) break;
		if( len == EOC ) {

			/* check misspeculation */
			consume_chunk(pid, (void*)((long)&decomp_end + sizeof(int)), (sizeof(decomp_end) - sizeof(int)));
			consume_scan(MERGE, &pred_pos, sizeof(long));
			//pt3("    MERGE misspec: %ld\t%ld\n", decomp_end.real_pos, pred_pos);
			if(pred_pos != 0)
				if(decomp_end.real_pos != pred_pos)
					recover_misspec(decomp_end.real_pos, pids, conf);

			pid++;
			if(pid == conf.num_process) pid = 0;
		} else {
			consume_chunk(pid, (void*)((long)&decomp_chunk+sizeof(int)), (sizeof(decomp_chunk) - sizeof(int)));

					
	//		memcpy(buff, decomp_chunk.buf, len);
			fwrite(decomp_chunk.buf, sizeof(char), len, conf.output_stream);
			//fwrite(decomp_chunk.buf, sizeof(char), len, stdout);
		}


	}

	//sleep(10);
		//printf("%x\n", buff[0]);
	pt1("@@@@ MERGE exit %f sec\n", ttime);
}

void InsertNode(int merge_index, unsigned long boundary, int buffer_size, unsigned char *local_buffer){
	// int i;
	MergeNode * m_NewNode;
	m_NewNode = (MergeNode *)malloc(sizeof(MergeNode));
	m_NewNode->merge_index	= merge_index;
	m_NewNode->boundary		= boundary;
	m_NewNode->buffer_size	= buffer_size;
	m_NewNode->local_buffer	= local_buffer;

	m_NewNode->next = NULL;
	if (m_Head == NULL){
		m_Head = m_NewNode;	
		m_Tail = m_NewNode;	
	}else{
		m_Tail->next = m_NewNode;
		m_Tail = m_NewNode;
	}
	//printf("insert merge %d %ld %d\n", merge_index, boundary, buffer_size);
}

void Mergebuffer(FILE * output) {

	MergeNode * m_CurrentNode = m_Head;

	//memcpy(buff, m_CurrentNode->local_buffer, m_CurrentNode->buffer_size);
	//memcpy(buff, m_CurrentNode->local_buffer, 5000);

//  	if(fwrite(m_CurrentNode->local_buffer, sizeof(unsigned char) , m_CurrentNode->buffer_size, output) != m_CurrentNode->buffer_size ){
//  		fprintf(stderr, "failed fwrite\n");
//  	}
// 	fflush(output);

	if(m_CurrentNode != NULL ){
		m_Head = m_CurrentNode->next;
	}
	if(m_CurrentNode->next == NULL ){
		m_Head = NULL;
	}
	free(m_CurrentNode);
}

void papl_store_buf(unsigned char *buf, int len, int buffer_index) {
	//memcpy(&(gz_papl[buffer_index].buf[gz_papl[buffer_index].pos]), buf, len);
	memcpy(&(gz_papl[0].buf[gz_papl[0].pos]), buf, len);
	//gz_papl[buffer_index].pos += len;
}


/* Read papl.conf & set some parameters */
void configure() {

	FILE *fp;
	char name[20];
	int val, tmp;
	//char val2[20];
	int ret;

	int cnt = 0;
	fp = fopen("papl.conf", "rb");

	while(fscanf(fp, "%s", name) != EOF ) {
		if(!strcmp(name, "num_process")) {
			ret = fscanf(fp, "%d", &val);
			tmp = val;
			conf.num_process = tmp;
			cnt++;
		}

		else if(!strcmp(name, "chunk_size")) {
			ret = fscanf(fp, "%d", &val);
			tmp = val;
			conf.chunk_size = tmp;
			cnt++;
		}

		else if(!strcmp(name, "merge_method")) {
			ret = fscanf(fp, "%d", &val);
			tmp = val;
			conf.merge_method = tmp;
//			ret = fscanf(fp, "%s", val2);
//			if(!strcmp(val2, "DIST"))
//				conf.merge_method = 1;
//			else if(!strcmp(val2, "DISTRIBUTED"))
//				conf.chunk_size = 1;
//			else conf.merge_method = 0;
			cnt++;
		}
//
//
////		else if(!strcmp(name, "overlap")) {
////			ret = fscanf(fp, "%d", &val);
////			conf.overlap = val;
////		}
//
	}

#ifdef papl_trace
	printf("num_process is %d\n", conf.num_process);
	printf("chunk_size is %d\n", conf.chunk_size);
	if(conf.merge_method == 1) 
		printf("merge_method is DIST\n");
	else printf("merge_method is CENT\n");
#endif
	fclose(fp);
}
