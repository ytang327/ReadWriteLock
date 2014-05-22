#include <pthread.h>
#include <stdio.h>
#include <cstdlib>

struct RWMutex{
    pthread_mutex_t mutex;
    pthread_cond_t read;
    pthread_cond_t write;
    int r_active;
    int w_active;
    int r_wait;
    int w_wait;
    int quota;
    int counter;
    int cycle;
};

void RWMutex_init(RWMutex* rw, int quota){
    rw->r_active = 0;
    rw->r_wait = rw->w_wait = 0;
    rw->w_active = 0;
    pthread_mutex_init(&rw->mutex, NULL);
    pthread_cond_init(&rw->read, NULL);
    pthread_cond_init(&rw->write, NULL);
    rw->counter = rw->quota = quota;
    rw->cycle = 0;

}

void RWMutex_destroy(RWMutex * rw){

    pthread_mutex_lock(&rw->mutex);
    if (rw->r_active > 0 || rw->w_active){
        pthread_mutex_unlock(&rw->mutex);
        fprintf(stderr, "There are still active readers and/or writers");
        exit(1);
    }
    if (rw->r_wait != 0 || rw->w_wait != 0){
        pthread_mutex_unlock(&rw->mutex);
        fprintf(stderr, "There are still threads waiting!");
        exit(1);
    }
    pthread_mutex_unlock(&rw->mutex);
    pthread_mutex_destroy(&rw->mutex);
    pthread_cond_destroy(&rw->read);
    pthread_cond_destroy(&rw->write);
}

/*void RWMutex_rdlock(RWMutex * rw){
    int cycle;
    pthread_mutex_lock(&rw->mutex);
    cycle = rw->cycle;
    if (--rw->counter == 0){
        ++rw->cycle;
        rw->counter = rw->quota;
        pthread_cond_broadcast(&rw->read);
    }
    if (rw->w_active || ((rw->w_wait > 0) && (cycle == rw->cycle))){
        rw->r_wait++;
        while (rw->w_active || ((rw->w_wait > 0 && cycle == rw->cycle))){
            pthread_cond_wait(&rw->read, &rw->mutex);
        }
        rw->r_wait--;
    }
    rw->r_active++;
    pthread_mutex_unlock(&rw->mutex);
}*/

void RWMutex_rdlock(RWMutex *rw){
    int cycle;
    pthread_mutex_lock(&rw->mutex);
    cycle = rw->cycle;
    while (rw->w_active || (((rw->w_wait > 0) && (cycle == rw->cycle)))){
        if (--rw->counter == 0){
            ++rw->cycle;
            rw->counter = rw->quota;
            pthread_cond_broadcast(&rw->read);
        }

        rw->r_wait++;
        pthread_cond_wait(&rw->read, &rw->mutex);
        rw->r_wait--;
    }

    rw->r_active++;
    pthread_mutex_unlock(&rw->mutex);
    
}

void RWMutex_rdunlock(RWMutex * rw){
    
    pthread_mutex_lock(&rw->mutex);
    rw->r_active--;
    if (rw->r_active == 0 && rw->w_wait > 0){
        pthread_cond_signal(&rw->write);
    }
    pthread_mutex_unlock(&rw->mutex);
}

void RWMutex_wrlock(RWMutex * rw){
    pthread_mutex_lock(&rw->mutex);
    if (rw->w_active || rw->r_active > 0){
        rw->w_wait++; 
        while (rw->w_active || rw->r_active > 0){
            pthread_cond_wait(&rw->write, &rw->mutex);
        }
        rw->w_wait--;
    } 
    rw->w_active = 1;
    pthread_mutex_unlock(&rw->mutex);
}

void RWMutex_wrunlock(RWMutex *rw){
    pthread_mutex_lock(&rw->mutex);
    rw->w_active = 0;
    if ((rw->w_wait > 0)){
        pthread_cond_signal(&rw->write); 
    }else if (rw->r_wait > 0){
        pthread_cond_broadcast(&rw->read);
    }
    pthread_mutex_unlock(&rw->mutex);
}
