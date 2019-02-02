#include <stdio.h>
#include <sys/types.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <fcntl.h> 
#include <sys/shm.h> 
#include <sys/stat.h> 
#include <semaphore.h>
#include <unistd.h>
#include <sys/mman.h>


int minvalue;
int maxvalue;
int bincount;
int N;
char **files;
char *outfile;
double w;
int shm_fd;
int* shared_histogram;
sem_t* sem_consumer;
sem_t* sem_producer;


void consume()
{
    
    int *histogram = (int*) malloc(bincount * sizeof(int));
    memset(histogram, 0, bincount * sizeof(int));
    int i;
    for(i = 0; i < N; i++)
    {
        sem_wait(sem_consumer);
        printf("consumer\n");
        int j;
        for(j = 0; j < bincount; j++)
            histogram[j] += (shared_histogram)[j];
        sem_post(sem_producer);
    }
    FILE *out = fopen(outfile, "w");

    for(i = 0; i < bincount; i++)
        fprintf(out, "%d: %d\n", i + 1, histogram[i]);

    free(histogram);
}

void produce(int i)
{
    int *histogram = (int*) malloc(bincount * sizeof(int));
    memset(histogram, 0, bincount * sizeof(int));
    FILE *f = fopen(files[i], "r");
    double value;
    
    while(fscanf(f, "%lf", &value) != EOF)
    {
        assert(value <= maxvalue && value >= minvalue);
        int current_bin = (int)((value - minvalue) / w);
        current_bin = (current_bin >= bincount) ? bincount - 1 : current_bin;
        histogram[current_bin]++;
    }
    
    sem_wait(sem_producer);
    int j;
    for(j = 0; j < bincount; j++)
    {
        (shared_histogram)[j] = histogram[j];
    }
    printf("producer %d\n", i);
    sem_post(sem_consumer);
    free(histogram);
}

int main(int argc, char **argv)
{
    if (argc < 7)
    {
        printf("Usage: phistogram minvalue maxvalue bincount N file1 … fileN outfile\n");
        return -1;
    }
    minvalue = atoi(argv[1]);
    maxvalue = atoi(argv[2]);
    bincount = atoi(argv[3]);
    N = atoi(argv[4]);
    if(argc != 5 + N + 1)
    {
        printf("Error: Input files count does not match N \n");
        return -1;
    }
    files = &argv[5];
    outfile = argv[5 + N];
    w = 1.0 * (maxvalue - minvalue) / bincount;

    sem_consumer = sem_open("/consumer", O_CREAT,  0644, 0);
    sem_producer = sem_open("/producer", O_CREAT,  0644, 1);

    int i;
    for(i = 0; i < N; i++)
    {
        int pid = fork();
        if (pid == 0)
            break;
    }

    shm_fd = shm_open("/syn_phistogram", O_CREAT | O_RDWR, 0666);
    ftruncate(shm_fd, bincount * sizeof(int));  
    shared_histogram = (int*) mmap(0, bincount * sizeof(int), PROT_WRITE, MAP_SHARED, shm_fd, 0); 

    
    if(i == N)
    {
        consume();
    }
    else
    {
        produce(i);
        exit(0);
    }


    shm_unlink("/syn_phistogram"); 
    
    sem_close(sem_consumer);
    sem_unlink("/consumer");
    
    sem_close(sem_producer);
    sem_unlink("/producer");

}