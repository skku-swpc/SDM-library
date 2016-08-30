#define papltime
#define papl_par
//#define papl_trace

#define MB (1024*1024*8)
#define TOTAL_BUFFER 400
//#define BUFFER_SIZE 2
//#define BUFFER_SIZE 2 * MB
#define BUFFER_SIZE (1024*1024*2)


volatile unsigned char *buff;

#ifdef papltime
float ttime;
float ttime1, ttime2;
#include <time.h>
struct timespec papl_ts1, papl_ts2;
#define init_time() clock_gettime(CLOCK_REALTIME, &papl_ts1);

#define get_time(total) { \
    long t1, t2;                        \
    __time_t s1, s2;                    \
    clock_gettime(CLOCK_REALTIME,       \
            &papl_ts2);                 \
    s1 = papl_ts1.tv_sec;               \
    t1 = papl_ts1.tv_nsec;              \
    s2 = papl_ts2.tv_sec;               \
    t2 = papl_ts2.tv_nsec;              \
    total = (s2 - s1) * 1000000			\
        + ((t2 - t1) / 1000);           \
	total /= 1000000;					\
}
#else
#define init_time()     /* */
#define get_time(t)		/* */
#endif

#ifdef papl_trace
#define pt0(a)				get_time(ttime); printf(a);
#define pt1(a, b)			get_time(ttime); printf(a, b);
#define pt2(a, b, c)		get_time(ttime); printf(a, b, c);
#define pt3(a, b, c, d)		get_time(ttime); printf(a, b, c, d);
#define pt4(a, b, c, d, e)	get_time(ttime); printf(a, b, c, d, e);
#else
#define pt0(a)	/* */
#define pt1(a, b)	/* */
#define pt2(a, b, c)	/* */
#define pt3(a, b, c, d)	/* */
#define pt4(a, b, c, d, e)	/* */
#endif

struct PAPL {
	/* function */
	unsigned long  (*scan)();
	void  (*decompress)();

	/* file status */
	char *input_name;
	char *output_name;

	FILE *input_stream;
	FILE *output_stream;

	void *db;			// application specific structure 

	/* configuration */
	int num_process;
	int chunk_size;
	long file_size;
	enum COMMIT_METHOD { CENT, DIST } merge_method;

	int pid;
	unsigned int real_pos;
};

#define MERGE_BUFFER_SIZE	5000
//#define MERGE_BUFFER_SIZE	0x8000
struct decomp_chunk_t {
	unsigned int len;
	unsigned char buf[MERGE_BUFFER_SIZE];
};

struct decomp_end_t {
	int flag;
	// flag must be same byte with len in struct decomp_chunk_t
	unsigned long real_pos;
};

struct scan_params {
	unsigned long int next_pos;
	int scanned_size;
};


#define EOS		-1
#define EOC		-1
#define EOD		-2

struct PAPL conf;
void papl_exec();
void papl_store_buf(unsigned char *, int, int);

struct gz_papl_t{
	int pos;
	unsigned char *buf;
};
struct gz_papl_t * gz_papl;
//}gz_papl[TOTAL_BUFFER];

typedef struct merge_list{
	int merge_index;
	unsigned int boundary;
	int buffer_size;
	unsigned char *local_buffer;
	struct merge_list *next;
}MergeNode;

int buffer_index;
int *token;
int *token_passing;
int *message_receive_order;

unsigned char *temp_buffer;

#include "papl_queue.h"
#define MERGE conf.num_process
#define Q_offset	(conf.num_process + 1)
#define produce_scan(pid, buf, size)									\
	chk_queue_Full(pid, size, Q_scan_size);								\
	papl_produce(pid, buf, size, Q_scan_size);	

#define produce_chunk(pid, buf, size)									\
	chk_queue_Full((pid + Q_offset), size, Q_data_size);				\
	papl_produce((pid + Q_offset), buf, size, Q_data_size);

#define consume_scan(pid, buf, size)									\
	chk_queue_Empty(pid);												\
	papl_consume(pid, buf, size, Q_scan_size);

#define consume_chunk(pid, buf, size)									\
	papl_consume((pid + Q_offset), buf, size, Q_data_size);

#define papl_mmap(len) mmap(0, len, PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, -1, 0);
