#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/time.h>
#include <sys/param.h>
#include <sched.h>

#define K 200

struct Node {
    int data;
    struct Node* next;
};

struct list {
    struct Node* header;
    struct Node* tail;
};

pthread_mutex_t mutex_lock;
struct list* List;

void bind_thread_to_cpu(int cpuid) {
    cpu_set_t mask;
    CPU_ZERO(&mask);
    CPU_SET(cpuid, &mask);
    sched_setaffinity(0, sizeof(cpu_set_t), &mask);
}

struct Node* generate_data_node() {
    struct Node* ptr = (struct Node*)malloc(sizeof(struct Node));
    if (ptr != NULL)
        ptr->next = NULL;
    return ptr;
}

void* producer_thread(void* arg) {
    bind_thread_to_cpu(*((int*)arg));

    struct Node *local_head = NULL, *local_tail = NULL;
    struct Node *ptr;
    int counter = 0;

    // 🔥 build LOCAL list (no locking)
    while (counter < K) {
        ptr = generate_data_node();
        if (ptr != NULL) {
            ptr->data = 1;

            if (local_head == NULL)
                local_head = local_tail = ptr;
            else {
                local_tail->next = ptr;
                local_tail = ptr;
            }
        }
        counter++;
    }

    // 🔥 attach local list to global list (ONE LOCK)
    pthread_mutex_lock(&mutex_lock);

    if (List->header == NULL)
        List->header = local_head;
    else
        List->tail->next = local_head;

    if (local_tail != NULL)
        List->tail = local_tail;

    pthread_mutex_unlock(&mutex_lock);

    pthread_exit(NULL);
}

int main(int argc, char* argv[]) {
    int i, num_threads;
    int NUM_PROCS;
    int* cpu_array = NULL;

    struct timeval starttime, endtime;

    num_threads = atoi(argv[1]);
    pthread_t producer[num_threads];

    NUM_PROCS = sysconf(_SC_NPROCESSORS_CONF);

    cpu_array = malloc(NUM_PROCS * sizeof(int));
    for (i = 0; i < NUM_PROCS; i++)
        cpu_array[i] = i;

    pthread_mutex_init(&mutex_lock, NULL);

    List = malloc(sizeof(struct list));
    List->header = List->tail = NULL;

    gettimeofday(&starttime, NULL);

    for (i = 0; i < num_threads; i++) {
        pthread_create(&producer[i], NULL, producer_thread,
                       &cpu_array[i % NUM_PROCS]);
    }

    for (i = 0; i < num_threads; i++) {
        pthread_join(producer[i], NULL);
    }

    gettimeofday(&endtime, NULL);

    printf("Total run time is %ld microseconds.\n",
           (endtime.tv_sec - starttime.tv_sec) * 1000000 +
           (endtime.tv_usec - starttime.tv_usec));

    return 0;
}
