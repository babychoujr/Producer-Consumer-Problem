//Eric Chou
//PROJECT 6
//95408627

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <semaphore.h>
#include <unistd.h>
#define DEBUG 0
#define BUFFER_SIZE 8

//creating the buffer using sbuf package
typedef struct{
	int * buf;
	int n; //max slots
	int front;
	int rear;
	int empty;
	int full;
	//sem_t mutex; // protects accesses to buf
	//sem_t slots; //number of empty slots
	//sem_t items; //number of items

}sbuf_t;

typedef struct{
	int i;
	int producer_number;
}p_data;

typedef struct{
	int p;
	int i;
	int c;
	int d;
	int consumer_number;
}command;
//global declaration of sbuf
sbuf_t shared_buffer;

//creating a struct to hold all the values of the command inputs
command global_args[16];

//producer_args
p_data producer_args[16];
//global count of producers
//int producer_number = 0;

//global count of consumers
//int consumer_number = 0;

//global mutex
pthread_mutex_t mutex;
pthread_cond_t notempty;
pthread_cond_t notfull;

//initialize the sbuf
void sbuf_init(sbuf_t *sp, int n){
	sp->buf = calloc(n, sizeof(int));
	sp->n = n;
	sp->front = sp->rear = 0;
	//sem_init(&sp->mutex, 0, 1);
	//sem_init(&sp->slots, 0, n);
	//sem_init(&sp->items, 0, 0);
	sp->empty = n;
	sp->full = 0;
}

//free the sbuf
void sbuf_deinit(sbuf_t * sp){
	free(sp->buf);
}

//insert the item onto the front of shared buffer sp
void sbuf_insert(sbuf_t *sp, int item){
	//P(&sp->slots);
	//P(&sp-> mutex);
	sp->buf[(sp->rear)++%(sp->n)] = item;
	
	sp->empty -= 1;
	
	sp->full += 1;
	//V(&sp->mutex);
	//V(&sp->items);
}

//remove an item from a shared buffer
int sbuf_remove(sbuf_t *sp){
	int item;
	//P(&sp->items);
	//P(&sp->mutex);
	item = sp->buf[(sp->front)++%(sp->n)];
	
	sp->empty += 1;
	sp->full -= 1;

	sp->buf[((sp->front)-1)%(sp->n)]= 0;
	//sp->buf[(sp->front)%(sp->n)] = 0;
	//V(&sp->mutex);
	//V(&sp->slots);
	return item;
}


//used to check the values of the command input
int check_values(int p, int c, int i, int d){
	int check = 0;
	if(p > 16 || p < 0){
	//	printf("HI");
		check = 1;
	}
	if(c > 16 || c < 0 || c >= (p*i)){
	//	printf("HII");
		check = 1;
	}
	if(d != 0 && d != 1){
	//	printf("HIII");
		check = 1;
	}
	
	return check;
}

//thread_produce - used to create the producers
void* thread_producer(void* arg){
	p_data* item_ptr = (p_data *)arg;
	
	//int* item_ptr = (int*)arg;
	int item = item_ptr->i;
	int producer_number = item_ptr->producer_number;
	
	int item_value = producer_number * item;
	
	int i;
	//printf("HI THIS IS A NEW THREAD\n");
	for(i = 0; i < item; i++){
		//Start critical section
		pthread_mutex_lock(&mutex);
		
		//if the buffer is full
		while(shared_buffer.full == BUFFER_SIZE){
			pthread_cond_wait(&notfull, &mutex);
		}

		//produce the item 
		sbuf_insert(&shared_buffer, item_value);		
		printf("producer_%d produced item %d\n", producer_number, item_value);
		item_value++;
		
		//add delay for producer
		if(global_args[0].d == 0){
			usleep(0.5);
		}	
		//send the signal
		pthread_cond_signal(&notempty);
		//End Critical section
		pthread_mutex_unlock(&mutex);
	}
	//producer_number++;	
	pthread_exit(NULL);
}

//thread_consumer - used to remove the item from the buffer and print the value
void* thread_consumer(void* arg){
	command* item_ptr = (command *)arg;
	
	int p = item_ptr->p;
	int c = item_ptr->c;
	int i = item_ptr->i;
	int consumer_number = item_ptr->consumer_number;	
	
	//printf("CONSUMER THREAD\n");
	//printf("%d, %d, %d, %d\n", p, c, i, d);
	int total_removes = (p*i)/c;
	int item_value = 0;
	int j;
	for(j = 0; j < total_removes; j++){
		//Start critical section
		pthread_mutex_lock(&mutex);
		
		//if the buffer is empty 
		while(shared_buffer.full == 0){
			pthread_cond_wait(&notempty, &mutex);
		}
		//consumers an item	
		item_value = sbuf_remove(&shared_buffer);
		printf("consumer_%d consumed item %d\n", consumer_number, item_value); 
		
		//add delay
		if(global_args[0].d == 1){
			usleep(0.5);
		}	
		//signal the sleeping threads
		pthread_cond_signal(&notfull);

		//End Critical Section
		pthread_mutex_unlock(&mutex);
	}
	//consumer_number++;
	pthread_exit(NULL);	
}

//main function of the code
int main(int argc, char** argv){
	if(argc < 5 || argc >5){
		printf("INCORRECT NUMBER OF ARGUMENTS\n");
		exit(0);
	}
	
	int producer, consumer, item, delay_option;
	int wrong_input = 0;
	producer = atoi(argv[1]);
	consumer = atoi(argv[2]);
	item     = atoi(argv[3]);
	delay_option = atoi(argv[4]);

	
	wrong_input = check_values(producer, consumer, item, delay_option);	
	
	if(wrong_input == 1){
		printf("INCORRECT VALUES\n");
		exit(0);
	}
	//intializing my global_args struct
	int z;
	for(z = 0; z < 16; z++){
		global_args[z].p = producer;
		global_args[z].c = consumer;
		global_args[z].i = item;
		global_args[z].d = delay_option;
		global_args[z].consumer_number = z;
	}
	//intializing my producer_args struct
	int e;
	for(e = 0; e < 16; e++){
		producer_args[e].i = item;
		producer_args[e].producer_number = e;
	}

	//wrong_input = check_values(producer, consumer, item, delay_option);
	
	
	//if(wrong_input == 1){
	//	printf("INCORRECT VALUES\n");
	//}

	//initialize the sbuf with default values
	sbuf_init(&shared_buffer, BUFFER_SIZE);
	
	//initialize the mutex items
	pthread_mutex_init(&mutex, 0);
	pthread_cond_init(&notempty, 0);
	pthread_cond_init(&notfull, 0);

	//Thread IDS for producers
	pthread_t producer_tids[producer];
	pthread_t consumer_tids[consumer];
		
	//create the pthread for producers
	int j;
	for(j = 0; j < producer; j++){
		pthread_create(&producer_tids[j], NULL, thread_producer, &producer_args[j]);
		//producer_args[j+1].producer_number += 1;
	}
	//pthread_create(&tids, NULL, thread_producer, &item);
	
	//create the pthread for consumers
	int k;
	for(k = 0; k < consumer; k++){
		pthread_create(&consumer_tids[k], NULL, thread_consumer, &global_args[k]);
		//global_args[k+1].consumer_number += 1;
	}
	//pthread_create(&tidss, NULL, thread_consumer, &global_args);

	//wait for pthreads to finish
	int l;
	for(l = 0; l < producer; l++){
		pthread_join(producer_tids[l], NULL);
	}

	int m;
	for(m = 0; m < consumer; m++){
		pthread_join(consumer_tids[m], NULL);
	}
	pthread_cond_destroy(&notempty);
	pthread_cond_destroy(&notfull);
	pthread_mutex_destroy(&mutex);
	//pthread_join(tids, NULL);
	//pthread_join(tidss, NULL);
	/*		
	int i;
	for(i = 0; i < BUFFER_SIZE; i++){
		printf("%d, ", shared_buffer.buf[i]);
	}
	*/
	//deinitialize the sbuf 
	sbuf_deinit(&shared_buffer);
	
	
	return 0;
}
