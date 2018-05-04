#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>

#include <errno.h>
#include "tm/Invyswell.hpp"
#include "tm/SpecSW.hpp"
#include "tm/WriteSet.hpp"
//#include "tm/IrrevocSW.hpp"
//#include "tm/SglSW.hpp"
//#include "tm/LightHW.hpp"
//#include "tm/BFHW.hpp"

#define CFENCE  __asm__ volatile ("":::"memory")
#define MFENCE  __asm__ volatile ("mfence":::"memory")

#define NO_OF_ACCOUNTS 1000000
#define INITIAL_BALANCE 1000
#define TOTAL_TRANSACTIONS 10//0000
#define BALANCE_TRANSFER 50
#define HASH 1
#define LOCAL_TRANSFER 2//10

/*typedef struct account
{
        unsigned int id;
        unsigned int balance;
}account;
*/
uint64_t accounts[NO_OF_ACCOUNTS];

volatile int counter = 0;
volatile int lock = 0;

//int no_of_total_threads;
uint64_t no_of_total_threads;
int local_transaction;
int random_numbers[TOTAL_TRANSACTIONS*LOCAL_TRANSFER][2];

//pthread_mutex_t lock[NO_OF_ACCOUNTS/HASH];

inline unsigned long long get_real_time() {
        struct timespec time;
    clock_gettime(CLOCK_MONOTONIC_RAW, &time);

    return time.tv_sec * 1000000000L + time.tv_nsec;
}

/**
 *  Support a few lightweight barriers
 */
void
barrier(int which)
{
    static volatile int barriers[16] = {0};
    CFENCE;
    __sync_fetch_and_add(&barriers[which], 1);
    while (barriers[which] != no_of_total_threads) { }
    CFENCE;
}

void
signal_callback_handler(int signum)
{
   // Terminate program
   exit(signum);
}

volatile bool ExperimentInProgress = true;
static void catch_SIGALRM(int sig_num)
{
    ExperimentInProgress = false;
}

void* th_run(void * args)
{
        long id = (long)args;
	tx_id = id;

        volatile long int x=0;
        uint64_t src_balance, dst_balance;
        unsigned long src_account, dst_account;

	thread_init(id);
        barrier(0);
        for (int i=0; i<local_transaction; i++) {

          	SpecSW_TX_BEGIN

                for(int j=0; j<LOCAL_TRANSFER; j++){

                        dst_account = random_numbers[x+id*local_transaction*LOCAL_TRANSFER][0];
                        src_account = random_numbers[x+id*local_transaction*LOCAL_TRANSFER][1];
                        //remember serializing in chronological order
                        x++;

                        if((src_balance = SpecSW_tx_read(&accounts[src_account])) < 0)
                                continue;//goto begin;
                        if((dst_balance = SpecSW_tx_read(&accounts[dst_account])) < 0)
                                continue;//goto begin;

                        if(src_balance >= BALANCE_TRANSFER)
                        {
                                SpecSW_tx_write(&accounts[src_account], src_balance - BALANCE_TRANSFER);
                                SpecSW_tx_write(&accounts[dst_account], dst_balance + BALANCE_TRANSFER);
                        }
                }

                SpecSW_tx_end();
        }

        return 0;
}

unsigned long random_number(unsigned long min_num, unsigned long max_num)
{
    unsigned long result = 0, low_num = 0, hi_num = 0;

    if (min_num < max_num)
    {
        low_num = min_num;
        hi_num = max_num + 1; // include max_num in output
    } else {
        low_num = max_num + 1; // include max_num in output
        hi_num = min_num;
    }

    result = (unsigned long)((rand() % (hi_num - low_num)) + low_num);
//    result = (unsigned long)((get_real_time() % (hi_num - low_num)) + low_num);
    return result;
}

void generate_random_numbers()
{
        unsigned long min, max;
        int x=0;

        srand(time(NULL));
        for(uint64_t i=0; i<no_of_total_threads; i++)
        {
                min = 0;//i * NO_OF_ACCOUNTS / no_of_total_threads;
                max = NO_OF_ACCOUNTS;//(i + 1) * NO_OF_ACCOUNTS / no_of_total_threads - 1;

//              printf("min = %lu max = %lu\n", min, max);
                for(int j=0; j<local_transaction*LOCAL_TRANSFER; j++)
                {
label:                  random_numbers[x][0] = random_number(min, max);
                        random_numbers[x][1] = random_number(min, max);
                        if(random_numbers[x][0] == random_numbers[x][1])
                                goto label;
//                      printf("%d %d\n", random_numbers[x][0], random_numbers[x][1]);
                        x++;
                }
        }
}

unsigned long long sum_of_accounts()
{
        unsigned long long sum = 0;

        for(int i=0; i<NO_OF_ACCOUNTS; i++)
                sum += accounts[i];

        return sum;
}

void init_accounts()
{
        for(int i=0; i<NO_OF_ACCOUNTS; i++)
        {
                accounts[i] = INITIAL_BALANCE;
        }
}

int main(int argc, char* argv[])
{
	signal(SIGINT, signal_callback_handler);
	tm_sys_init();

        unsigned long long sum = 0, i;
        if (argc < 2) {
                printf("Usage test threads#\n");
                exit(0);
        }

        no_of_total_threads = atoi(argv[1]);
        local_transaction = TOTAL_TRANSACTIONS/no_of_total_threads;

        init_accounts();
        sum = sum_of_accounts();
        printf("Total Balance before transactions = %llu\n", sum);

        generate_random_numbers();
/*
        for(i=0; i<NO_OF_ACCOUNTS/HASH; i++)
        {
                if(pthread_mutex_init(&locks[i].lc, NULL) != 0)
                {
                        printf("\n mutex init failed\n");
                        return 1;
                }

                locks[i].isLocked = 0;
                locks[i].version = 0;
        }
*/
        pthread_attr_t thread_attr;
        pthread_attr_init(&thread_attr);

        pthread_t client_th[300];
        long ids = 1;
        for (i = 1; i<no_of_total_threads; i++) {
                pthread_create(&client_th[ids-1], &thread_attr, th_run, (void*)ids);
                ids++;
        }

        unsigned long long start = get_real_time();

        th_run(0);

        for (int i=0; i<ids-1; i++) {
                pthread_join(client_th[i], NULL);
        }

        printf("Total time = %lld ns\n", get_real_time() - start);
        /*
        for(i=0; i<NO_OF_ACCOUNTS/HASH; i++)
                pthread_mutex_destroy(&locks[i].lc);
*/
        sum = sum_of_accounts();
        printf("Total Balance after transactions = %llu\n", sum);

        return 0;
}
//Build with 
//g++ test_threads.cpp -o test -lpthread
