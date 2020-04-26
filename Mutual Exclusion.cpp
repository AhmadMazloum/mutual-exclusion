#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <iostream>
#include <stdlib.h>
#include <sys/wait.h>
#include <dispatch/dispatch.h>
#include <queue>
#define Q_size 1024
#define Messages_size 16
dispatch_semaphore_t mutex;
dispatch_semaphore_t e;
dispatch_semaphore_t n;
dispatch_semaphore_t counter_sem;
std::queue <int> Integer;
int counter;

void* counterInc(void* arg){
    int * iteration = (int *) arg;
    printf("Counter Thread %d : received a message \n",*iteration);
    dispatch_semaphore_wait(counter_sem,DISPATCH_TIME_FOREVER);
    counter++;
    printf("Counter Thread %d : now adding to counter, counter value = %d \n",*iteration,counter);
    dispatch_semaphore_signal(counter_sem);
    iteration++;
    return nullptr;
}

void* consume(void* arg){
    int index = 0;
    while (true) {
        if (Integer.size() == 0) {
            printf("Collector Thread: nothing is in the Buffer !!!\n");
            sleep(10);
        }
        dispatch_semaphore_wait(n,DISPATCH_TIME_FOREVER);
        dispatch_semaphore_wait(mutex,DISPATCH_TIME_FOREVER);
        printf("Collector Thread: reading from buffer at position %d\n",index);
        index++;
        index = index % Q_size;
        Integer.pop();
        dispatch_semaphore_signal(mutex);
        dispatch_semaphore_signal(e);
        sleep(5);
    }
    return nullptr;
}

void* produce(void* arg){
    int index = 0;
    while (true) {
        printf("Moniter Thread: waiting to read counter\n");
        if(counter == 0)
            sleep(10);
        if (Integer.size() == Q_size) {
            printf("Moniter Thread: Buffer is Full \n");
            sleep(10);
        }
        dispatch_semaphore_wait(e,DISPATCH_TIME_FOREVER);
        dispatch_semaphore_wait(mutex,DISPATCH_TIME_FOREVER);
        dispatch_semaphore_wait(counter_sem,DISPATCH_TIME_FOREVER);
        printf("Moniter Thread: reading count %d \n",counter);
        printf("Moniter Thread: writing to buffer at position %d \n",index);
        index++;
        index = index % Q_size;
        Integer.push(counter);
        counter = 0;
        dispatch_semaphore_signal(counter_sem);
        dispatch_semaphore_signal(mutex);
        dispatch_semaphore_signal(n);
        sleep(5);
    }
    return nullptr;
}

int main(int argc, const char * argv[]) {
    mutex = dispatch_semaphore_create(1);
    e = dispatch_semaphore_create(Q_size);
    n = dispatch_semaphore_create(0);
    counter_sem = dispatch_semaphore_create(1);
    pthread_t mmoniter,mcollector;
    pthread_t mcounter [Messages_size];
    pthread_create(&mmoniter, NULL, produce, NULL);
    pthread_create(&mcollector, NULL, consume, NULL);
    int i = 0;
    int * send;
    while (true) {
        send = &i;
        pthread_create(&mcounter[i], NULL, counterInc, (void*)send);
        int sleepTime = rand() % 5 + 1;
        sleep(sleepTime);
        i++;
        i = i % Messages_size;
    }
//    wont join because in an infinite loops
//    pthread_join(mmoniter, NULL);
//    pthread_join(mcollector, NULL);
//    for (i = 0; i < 128; i++) {
//        pthread_join(mcounter[i], NULL);
//    }
    return 0;
}
