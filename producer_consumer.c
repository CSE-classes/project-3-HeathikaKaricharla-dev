#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#define BUFFER_SIZE 5

char buffer[BUFFER_SIZE];
int in = 0;
int out = 0;
int count = 0;

pthread_mutex_t mutex;
pthread_cond_t not_full;
pthread_cond_t not_empty;

FILE *fp;

void *producer(void *arg) {
    char ch;

    while ((ch = fgetc(fp)) != EOF) {

        pthread_mutex_lock(&mutex);

        while (count == BUFFER_SIZE)
            pthread_cond_wait(&not_full, &mutex);

        buffer[in] = ch;
        in = (in + 1) % BUFFER_SIZE;
        count++;

        pthread_cond_signal(&not_empty);
        pthread_mutex_unlock(&mutex);
    }

    // send termination signal safely
    pthread_mutex_lock(&mutex);

    while (count == BUFFER_SIZE)
        pthread_cond_wait(&not_full, &mutex);

    buffer[in] = '\0';
    in = (in + 1) % BUFFER_SIZE;
    count++;

    pthread_cond_signal(&not_empty);
    pthread_mutex_unlock(&mutex);
        pthread_exit(NULL);
}

void *consumer(void *arg) {
    char ch;

    while (1) {

        pthread_mutex_lock(&mutex);

        while (count == 0)
            pthread_cond_wait(&not_empty, &mutex);

        ch = buffer[out];
        out = (out + 1) % BUFFER_SIZE;
        count--;

        pthread_cond_signal(&not_full);
        pthread_mutex_unlock(&mutex);

        if (ch == '\0')
            break;

        printf("%c", ch);
        fflush(stdout);
    }

    pthread_exit(NULL);
}

int main() {
    pthread_t prod, cons;

    fp = fopen("message.txt", "r");
    if (fp == NULL) {
        printf("ERROR: cannot open message.txt\n");
        return -1;
    }

    pthread_mutex_init(&mutex, NULL);
    pthread_cond_init(&not_full, NULL);
    pthread_cond_init(&not_empty, NULL);

    pthread_create(&prod, NULL, producer, NULL);
    pthread_create(&cons, NULL, consumer, NULL);

    pthread_join(prod, NULL);
    pthread_join(cons, NULL);

    fclose(fp);

    pthread_mutex_destroy(&mutex);
    pthread_cond_destroy(&not_full);
    pthread_cond_destroy(&not_empty);

    printf("\n");
    return 0;
}
