/*
 * kv_bench.c 
 * Author : Heekwon Park
 * E-mail : heekwon.p@samsung.com
 *
 * Main function
 *  - Build key list(or get keys from key file)
 *  - Pthread configuration
 *  - Key file management
 *  - Performance report
 */
#include <string.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <errno.h>
#include <unistd.h>
#include <limits.h>
#include <stdbool.h>
#include <pthread.h>
#include <linux/unistd.h>
#include <sys/syscall.h>

#include "kv_trace.h"
#include "kv_bench.h"
#include "kv_worker.h"
#include "linux_nvme_ioctl.h"

cycles_t msec_value;
cycles_t usec_value;
cycles_t nsec_value;
cycles_t cycle_value;

static void usage(char *prg_name, char *msg)
{
    ERRLOG("%s\nUsage: %s options\n"
           " -d Device path(default : %s)\n"
           " -c Thread configuration file name(default : %s)\n"
           " -k Key file name(default : %s)\n"
           " -s Key size for new keys(default : %d byte)\n"
           " -v Value size(default %dKB, Min & Max size : %dB ~ %dMB)\n"
           " -u Use unique value for each key(\"Y(Yes)\" \"N(No)\" default : Y)\n"
           " -h or ? help\n" 
           " * The keys generated by the put requests are stored in the key file.\n"
           " * You must provide a valid key file on a Get or Del request without a Put request.\n"
           " * If a list of keys is not provided in the Put request, the put request uses the TSC to generate the key.\n" 
           " * A value is filled with its key pattern + update count. \n"
            , msg, prg_name,
            DEFAULT_DEV_PATH,
            DEFAULT_CONFIG,
            DEFAULT_KEY_FILE,
            DEFAULT_KEY_SIZE,
            DEFAULT_VALUE_SIZE/TO_KBYTE,
            MIN_VALUE_SIZE,
            MAX_VALUE_SIZE/TO_MBYTE
            );
}

static unsigned long long get_size(const char *arg)
{
    char *p;
    unsigned long long size;
    size = strtoul(arg, &p, 0);
    if (size == 0) return 0;

    switch(*p){
      case 'G': case 'g': size*=1024;
      case 'M': case 'm': size*=1024;
      case 'K': case 'k': size*=1024;
    }
    return size;
}

static void key_generator(char *key, int key_size){
    char tsc[NAME_LEN];
    unsigned long count, unit_size, tsc_size;
    sprintf(tsc, "%llx", get_cycles());
    tsc_size = strlen(tsc);

    count = key_size;
    do{
        unit_size = ((count > tsc_size) ?  tsc_size : count);
        memcpy(key, tsc, unit_size);
        count -= unit_size;
        key += unit_size;
    }while(count);

}

/* 
 * Determines the number of threads 
 * by calculating the number of valid rows 
 * in the thread configuration file. 
 * A comment starts with "#"
 */
int get_thread_count(FILE *fp){
    char *line;
   size_t len;
    int ret = 0;
    ssize_t read_size;
    while ((read_size = getline(&line, &len, fp)) != -1) {
        if(strncmp("#",line,1))
            ret++;
    }
    free(line);
    fseek(fp, 0 , SEEK_SET);
    return ret;
}


/*
 * Get key from the key file 
 * if nr_key is greater than the number of keys in the key file,
 * create new key using TSC.
 */

static void build_key_list(struct key_struct **exist_key, struct key_struct **new_key, unsigned long *nr_exist_key, unsigned long *nr_new_key, FILE *fp, unsigned long nr_key, int key_size)
{
    unsigned long i, j;;
    size_t len;
    char *line = NULL;
    char *token;
    char seps[]  = " \n";
    ssize_t read_size;

    /* Allocate key memory */
    /*for( i = 0; i < nr_key ; i++){
        exist_key[i]->key = (char *) malloc(MAX_KEY_SIZE);
        new_key[i]->key = (char *) malloc(MAX_KEY_SIZE);
    }*/

    /* Get keys from the key file and then set exist key list */
    for (i = 0 ; i < nr_key && (read_size = getline(&line, &len, fp)) != -1 ; i++){

        struct key_struct *key = malloc(sizeof(struct key_struct));
        posix_memalign((void **)&key->key, 16, MAX_KEY_SIZE);
        /* The keys in the file contain '\n', So, we need to exclude '\n'. */
        token = strtok( line, seps );
        strcpy(key->key, token);
        /* Get next token: key size */
        token = strtok( NULL, seps );
        if(!token) ERRLOG("The given key file has been corrupted\n");
        key->key_size = atol(token);
        /* Get next token: value size */
        token = strtok( NULL, seps );
        if(!token) ERRLOG("The given key file has been corrupted\n");
        key->value_size = atol(token);
        /* Get next token: Write(overwrite) count*/
        token = strtok( NULL, seps );
        if(!token) ERRLOG("The given key file has been corrupted\n");
        key->written_cnt = atoi(token);
        exist_key[i] = key;
        (*nr_exist_key)++;
    }
    free(line);

    /* Create keys using TSC and then set new key list*/
    for( j = 0 ; i < nr_key ; i++, j++){
        struct key_struct *key = malloc(sizeof(struct key_struct));
        posix_memalign((void **)&key->key, 16, MAX_KEY_SIZE);
        key_generator(key->key, key_size);
        key->key_size= key_size;
        key->value_size= 0;
        key->written_cnt = 0;
        new_key[j] = key;
        (*nr_new_key)++;
        /* no keys are remained in the key file */
    }

    return;
}


int set_thread_config(FILE *fp, struct env_set *env, struct thread_param *param, int nr_threads)
{
    char *line;
    char *token;
    char seps[]  = " \n";
    size_t len;
    int ret = 0;
    int key_fd;
    FILE *key_fp, *tmp;
    unsigned long max_shared_key_cnt = 0;
    struct per_thread_config *config;
    int i, idx = 0;
    ssize_t read_size;

    while ((read_size = getline(&line, &len, fp)) != -1) {
        if(strncmp("#", line, 1)){
            char config_str[NR_THREAD_CONFIG][NAME_LEN]; 
            int cfg_idx = 0;
            token = strtok( line, seps );
            /* Get each configuration values */
            while( token != NULL ){
                if(cfg_idx==NR_THREAD_CONFIG) 
                    ERRLOG("Wrong thread configuration format! Check the config file\n");
                strcpy(config_str[cfg_idx++], token);
                /* While there are tokens in "string" */
                /* Get next token: */
                token = strtok( NULL, seps );
            }
            if(cfg_idx != NR_THREAD_CONFIG) ERRLOG("Wrong thread configuration format! Check the config file\n");

            //////////////////////////* Initialze struct per_thread_config *//////////////////////////
            config = malloc(sizeof(struct per_thread_config));
            config->io_ratio[ASYNC] = atoi(config_str[ASYNC_RATIO]);
            config->io_ratio[SYNC] = 100 - config->io_ratio[ASYNC];
            config->cmd_ratio[PUT] = atoi(config_str[PUT_RATIO]);
            config->cmd_ratio[GET] = atoi(config_str[GET_RATIO]);
            config->cmd_ratio[DEL] = 100 - config->cmd_ratio[PUT] - config->cmd_ratio[GET] ;
            for( i = ASYNC ; i < MAX_IO_TYPE ; i++)
                if(config->io_ratio[i] < 0 || config->io_ratio[i] > 100)
                    ERRLOG("Wrong thread configuration format!\n Check the config file\n");
            for( i = PUT; i < MAX_CMD_TYPE ; i++)
                if(config->cmd_ratio[i] < 0 || config->cmd_ratio[i] > 100)
                    ERRLOG("Wrong thread configuration format!\n Check the config file\n");
            if (strncmp(config_str[PRIVATE_KEY],"T", 1) && strncmp(config_str[PRIVATE_KEY],"t", 1))
                config->use_private_key = false;
            else
                config->use_private_key = true;

            if (strncmp(config_str[ACCESS_PATTERN],"T", 1) && strncmp(config_str[ACCESS_PATTERN],"t", 1))
                config->seq_access = false;
            else
                config->seq_access = true;
            config->nr_key = atol(config_str[NR_KEYS]);
            config->nr_req = atol(config_str[NR_REQ]);
            /////////////////////////////////////////////////////////////////////////////////////////
            /* Get the number of shared keys */
            if(!config->use_private_key && max_shared_key_cnt < config->nr_key){
                max_shared_key_cnt = config->nr_key;
#ifdef PRINT_LOG
                LOG("set max_shared_key_cnt = %ld\n", max_shared_key_cnt);
#endif
            }
            param[idx].env = env;
            param[idx].config = config;
            param[idx].exclude_flush.iops = 0.0;
            param[idx].exclude_flush.throughput= 0.0;
            param[idx].include_flush.iops = 0.0;
            param[idx].include_flush.throughput= 0.0;
            param[idx].tid = idx;
            idx++;
        }
    }
    free(line);
    line =NULL;
    fseek(fp, 0 , SEEK_SET);

    /* open key list file & read keys from the file */
    key_fd = open(env->key_file, O_RDWR | O_CREAT, 0666);
    if ( key_fd < 0){
        perror("fail open");
        exit(1);
    }
    key_fp = fdopen(key_fd, "r+");
    if (key_fp == NULL){
        perror("fail fdopen");
        close(key_fd);
        exit(1);
    }
    /* Initialize shared keys */
    if(max_shared_key_cnt){
        env->exist_shared_key = malloc(max_shared_key_cnt * sizeof(struct key_struct *));
        env->new_shared_key = malloc(max_shared_key_cnt * sizeof(struct key_struct *));
        env->nr_exist_shared_key = 0;
        env->nr_new_shared_key = 0;

        /*for(idx = 0 ; idx < max_shared_key_cnt ; idx++){
            env->exist_shared_key[idx] = malloc(sizeof(struct key_struct));
            env->new_shared_key[idx] = malloc(sizeof(struct key_struct));
        }*/
        build_key_list(env->exist_shared_key, env->new_shared_key, &env->nr_exist_shared_key, &env->nr_new_shared_key, key_fp, max_shared_key_cnt, env->key_size);
        env->nr_shared_key = max_shared_key_cnt;
#ifdef PRINT_LOG
        LOG("shared keys = exist(%ld), new(%ld)\n", env->nr_exist_shared_key, env->nr_new_shared_key);
#endif
    }

    /* Initialize private keys */
    for(i = 0 ; i < nr_threads ; i++){
        config =  param[i].config;
        if(config->use_private_key){
            config->exist_key = malloc(config->nr_key * sizeof(struct key_struct *));
            config->new_key = malloc(config->nr_key * sizeof(struct key_struct *));
            config->nr_exist_key = malloc(sizeof(unsigned long));
            *config->nr_exist_key = 0;
            config->nr_new_key = malloc(sizeof(unsigned long));
            *config->nr_new_key = 0;
            /*
            for(idx = 0 ; idx < config->nr_key; idx++){
                config->exist_key[idx]  = malloc(sizeof(struct key_struct));
                config->new_key[idx] = malloc(sizeof(struct key_struct));
            }
            */
            build_key_list(config->exist_key, config->new_key, config->nr_exist_key, config->nr_new_key, key_fp, config->nr_key, env->key_size);
        }else{
            config->exist_key = env->exist_shared_key; 
            config->new_key = env->new_shared_key;
            config->nr_exist_key = &env->nr_exist_shared_key; 
            config->nr_new_key = &env->nr_new_shared_key;
#ifdef PRINT_LOG
            LOG("assign shared keys to a thread= exist(%ld), new(%ld)\n", *config->nr_exist_key, *config->nr_new_key);
#endif
        }
    }
        /* Keep only unused keys in the key file */
    tmp = fopen("./kv_tmp", "w");

    while ( (read_size = getline(&line, &len, key_fp)) != -1)
        fprintf(tmp,"%s", line);
    if(line) free(line);
    fclose(key_fp);
    fclose(tmp);
    remove(env->key_file);
    rename("./kv_tmp", env->key_file);
    return ret;
}


static void create_default_thread_config_file(void)
{
    FILE *file;
    if(!(file = fopen(DEFAULT_CONFIG, "a")))
        ERRLOG("%s open error: error is %s(errno=%d)\n", DEFAULT_CONFIG, strerror(errno), errno);
    chmod(DEFAULT_CONFIG, S_IRUSR|S_IWUSR);
    fprintf(file, "#Format(7 columns) : async_ratio(0~100) put_raito(0~100) get_ratio(0~100)" 
            "private_key[T(private)/F(shared)] seq_access[T(sequntial)/F(random)] #_of_keys #_of_requests\n");
    fprintf(file, "# Threads using shared keys should have the same number of keys\n");
    fprintf(file, "# Example\n"
                "# Async : Sync ratio                       = 70 : 30\n"
                "# PUT : GET : DEL ratio                    = 100 : 0 : 0\n"
                "# Use private(unique) keys                 : Private\n"
                "# (Private = T/t, Shared = F/f)           \n"
                "# Access pattern                           : Random\n"
                "# (Sequential = T/t, Random = F/f)           \n"
                "# The number of keys                       : 10000\n"
                "# The number of request                    : 100000\n");
    fprintf(file, "70 100 0 t f 10000 100000\n");
    fclose(file);
}

int main(int argc, char *argv[])
{
    int opt = 0;
    FILE *fp;

    struct env_set env = {
        .dev                = DEFAULT_DEV_PATH,
        .fd                 = 0,
        .nsid               = 0,
        .thread_config_file = DEFAULT_CONFIG,
        .key_file           = DEFAULT_KEY_FILE,
        .key_size           = DEFAULT_KEY_SIZE,
        .value_size         = DEFAULT_VALUE_SIZE,	
        .unique_value       = true,
    };

    struct thread_param *param;

    int nr_threads;
    int rc;
    pthread_t *thread;
    pthread_attr_t attr;
    void *status;

    int i;
    struct stat check;
    struct performance_result total_e_flush = {
        .msec = 0, 
        .iops = 0.0,
        .throughput = 0.0,
    };
    struct performance_result total_i_flush = {
        .msec = 0, 
        .iops = 0.0,
        .throughput = 0.0,
    };
    struct performance_result worst_thread_aft_flush = {
        .msec = 0, 
        .iops = 0.0,
        .throughput = 0.0,
    };
    struct performance_result worst_thread_bef_flush= {
        .msec = 0, 
        .iops = 0.0,
        .throughput = 0.0,
    };

    unsigned long long total_req=0;

    /* If default thread configuration file does not exist, create one */
    if (stat (DEFAULT_CONFIG, &check) != 0 && errno == ENOENT ){
        create_default_thread_config_file();
    }

    while((opt = getopt(argc, argv, "d:c:k:s:v:u:h:?")) != -1) {
        switch(opt) {
            case 'd':
                strcpy(env.dev, optarg);
                errno = 0;
                if (stat (env.dev, &check) != 0 && errno == ENOENT ) {
                    char msg[NAME_LEN*2];
                    sprintf(msg, "Device does not exist (Given path : %s)", env.dev);
                    usage(argv[0], msg);
                }
                break;
            case 'c':
                strcpy(env.thread_config_file, optarg);
                errno = 0;
                if (stat (env.thread_config_file, &check) != 0 && errno == ENOENT ) {
                    char msg[NAME_LEN*2];
                    sprintf(msg, "Thread configuration file does not exist (Given path : %s)\n"
                            "Format(7 columns) : async_ratio(0~100) put_ratio(0~100) get_ratio(0~100)" 
                            "private_key(T/F) seq_access(T/F) #_of_keys #_of_requests\n"
                            "NOTE THAT : sync ratio = 100-async && del ratio = 100-(put+get)\n"
                            "            Number of ROWs == Number of threads\n"
                            "            A comment should be start with #.", env.thread_config_file);
                    usage(argv[0], msg);
                }
                break;
            case 'k':
                strcpy(env.key_file, optarg);
                break;
            case 's':
                if ((env.key_size = (int)get_size(optarg)) <= 0 || env.key_size > MAX_KEY_SIZE){
                    char msg[NAME_LEN*2];
                    sprintf(msg, "key size must be between 8~255 byte(Given size : %d)", env.key_size);
                    usage(argv[0], msg);
                }
                break;
            case 'v':
                if ((env.value_size = (unsigned long)get_size(optarg)) <= 0 || env.value_size > MAX_VALUE_SIZE){
                    char msg[NAME_LEN*2];
                    sprintf(msg, "Wrong value size(Given size : %ld)", env.value_size);
                    usage(argv[0], msg);
                }
                break;
            case 'u': /* Use unique value for each key*/
                /* true by default */
                if (strncmp(optarg,"Y", 1) && strncmp(optarg,"y",1))
                    env.unique_value = false;
                break;
            case 'h':
            case '?':
            default:
                usage(argv[0], "HELP");
                break;
        }
    }
    pthread_spin_init(&env.lock, 0);
    time_init(&msec_value, &usec_value, &nsec_value, &cycle_value);
    
    ////////////////////* Configuration *///////////////////
    if(!(fp = fopen(env.thread_config_file, "r")))
        ERRLOG("%s open fail\n", env.thread_config_file);
    /* Get the # of threads*/
    nr_threads = get_thread_count(fp);
    if(!nr_threads) ERRLOG("No valid configuration format exists in %s\n", env.thread_config_file);
    thread = (pthread_t *)malloc(nr_threads * sizeof(pthread_t));
    param = (struct thread_param *)malloc(nr_threads * sizeof(struct thread_param));
    /* Thread configuration */
    set_thread_config(fp, &env, param, nr_threads);
    fclose(fp);
    ///////////////////////////////////////////////////////

    /* Key-Value Device Open & get namespace ID */
    env.fd = open(env.dev, O_RDWR);
    if (env.fd < 0) ERRLOG("fail to open device %s\n", env.dev);

    env.nsid = ioctl(env.fd, NVME_IOCTL_ID);
    if (env.nsid == (unsigned) -1) ERRLOG("fail to get nsid for %s\n", env.dev);

    /* Launch threads */
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

    for(i = 0 ; i < nr_threads ; i++){
        rc = pthread_create(&thread[i], &attr, worker_func, (void *)&param[i]);
        if(rc) ERRLOG("return code from pthread_create() is %d\n", rc);
    }
    pthread_attr_destroy(&attr);
    for(i=0; i < nr_threads ; i++){
        rc = pthread_join(thread[i], &status);
        if(rc) ERRLOG("return code from pthread_join() is %d\n", rc);
#ifdef PRINT_LOG
        LOG("completed join with thread %d having a status of %ld\n", i, (long)status);
#endif
    }
    close(env.fd);

    for(i = 0 ; i < nr_threads ; i++){
        if(i==0){
            printf("=======|========|========| %2d | Thread | CONFiguration |=======|=======|=======|======|\n", i);
            printf("CONF | %5s | %5s | %3s | %3s | %3s | %7s | %9s | %8s | %8s |\n"
                    "CONF | %5d | %5d | %3d | %3d | %3d | %7s | %9s | %8ld | %8ld |\n"
                    , "async", "sync", "put", "get", "del", "private", "sequntial", "nr_key", "nr_req", param[i].config->io_ratio[ASYNC], param[i].config->io_ratio[SYNC], param[i].config->cmd_ratio[PUT], param[i].config->cmd_ratio[GET], param[i].config->cmd_ratio[DEL], param[i].config->use_private_key?"true":"false", param[i].config->seq_access?"true":"false", param[i].config->nr_key, param[i].config->nr_req);
            printf("=======|========|=======|=======|=======|========|=======|=======|=======|======|\n");
        }
#ifdef PRINT_LOG
        LOG("[Elapsed time] Total Elaped Time : B/ Flush = |%.3f| Sec, A/ Flush = |%.3f| Sec\n", param[i].exclude_flush.msec/1000.0, param[i].include_flush.msec/1000.0);
        LOG("[%ldKB value Performance] Thruput(B/ Flush) : |%.2f| MB/s, Thruput(A/ Flush) : |%.2f| MB/s\n", env.value_size/1024, param[i].exclude_flush.throughput, param[i].include_flush.throughput);
#endif
        printf("[|%d Thread |%ldB value|Performance|]| IOPS(B/ Flush)| : |%.2f|k IOPS|,| IOPS(A/ Flush)| : |%.2f|k IOPS|\n", i, env.value_size, param[i].exclude_flush.iops, param[i].include_flush.iops);
        total_e_flush.msec += param[i].exclude_flush.msec;
        total_e_flush.throughput += param[i].exclude_flush.throughput;
        total_e_flush.iops += param[i].exclude_flush.iops;
        total_i_flush.msec += param[i].include_flush.msec;
        if(param[i].exclude_flush.msec > worst_thread_bef_flush.msec)
            worst_thread_bef_flush.msec = param[i].exclude_flush.msec;
        if(param[i].include_flush.msec > worst_thread_aft_flush.msec)
            worst_thread_aft_flush.msec = param[i].include_flush.msec;
        total_i_flush.throughput += param[i].include_flush.throughput;
        total_i_flush.iops += param[i].include_flush.iops;
        total_req += param[i].config->nr_req;
    }
    worst_thread_aft_flush.throughput = ((double)total_req * ((double)env.value_size/1024.0/1024.0))/((double)worst_thread_aft_flush.msec/1000.0);
    worst_thread_aft_flush.iops = ((double)total_req)/(worst_thread_aft_flush.msec/1000.0)/1000.0;
    worst_thread_bef_flush.throughput = ((double)total_req * ((double)env.value_size/1024.0/1024.0))/((double)worst_thread_bef_flush.msec/1000.0);
    worst_thread_bef_flush.iops = ((double)total_req)/(worst_thread_bef_flush.msec/1000.0)/1000.0;

#ifdef PRINT_LOG
    printf("=================================== Overall(Sum) =================================\n");
    printf("=================================== performance  =================================\n");
    LOG("[Elapsed time] Total Elaped Time : B/ Flush = |%.3f| Sec, A/ Flush = |%.3f| Sec\n", total_e_flush.msec/1024.0, total_i_flush.msec/1000.0);
    LOG("[%ldKB value Performance] Thruput(B/ Flush) : |%.2f| MB/s, Thruput(A/ Flush) : |%.2f| MB/s\n", env.value_size/1024, total_e_flush.throughput, total_i_flush.throughput);
    LOG("[%ldKB value Performance] IOPS(B/ Flush) : |%.2f|k IOPS, IOPS(A/ Flush) : |%.2f|k IOPS\n", env.value_size/1024, total_e_flush.iops, total_i_flush.iops);
    printf("===================================================================================\n");
#endif
    printf("========|=========|=========| Overall | performance |=========|========|========|\n");
#ifdef PRINT_LOG
    LOG("[Elapsed time] Total Elaped Time : After Flush = |%.3f| Sec\n", worst_thread_aft_flush.msec/1024.0);
    LOG("[%ldKB value Performance] Thruput(A/ Flush) : |%.2f| MB/s\n", env.value_size/1024, worst_thread_aft_flush.throughput);
#endif
    printf("[Total |%ldB value|Performance|]| IOPS(B/ Flush)| : |%.2f|k IOPS|,| IOPS(A/ Flush)| : |%.2f|k IOPS|\n", env.value_size, worst_thread_bef_flush.iops, worst_thread_aft_flush.iops);
    printf("********|********|********|********|********|********|********|********|********|\n");
    /* Save Key List( append mode ) */
    fp = fopen(env.key_file, "a");

    /* Save Written Shared Key List */
    if(env.nr_shared_key){
        for(i = 0 ; i < env.nr_exist_shared_key ; i++){
            if(env.exist_shared_key[i]->written_cnt)
                fprintf(fp, "%s %ld %ld %d\n", env.exist_shared_key[i]->key, env.exist_shared_key[i]->key_size, env.exist_shared_key[i]->value_size, env.exist_shared_key[i]->written_cnt);
        }
    }

    /* Save Written Private Key List */
    for(i = 0 ; i < nr_threads ; i++){
        struct per_thread_config *config = param[i].config;
        int j;
        if(config->use_private_key){
            for(j = 0 ; j < *config->nr_exist_key; j++){
                if(config->exist_key[j]->written_cnt)
                    fprintf(fp, "%s %ld %ld %d\n", config->exist_key[j]->key, config->exist_key[j]->key_size, config->exist_key[j]->value_size, config->exist_key[j]->written_cnt);
            }
        }
    }
    fclose(fp);
    pthread_spin_destroy(&env.lock);

    /* Shared Key List Free */
    if(env.nr_shared_key){
        for(i = 0 ; i < env.nr_exist_shared_key ; i++){
            free(env.exist_shared_key[i]->key);
            free(env.exist_shared_key[i]);
        }
        free(env.exist_shared_key);

        for(i = 0 ; i < env.nr_new_shared_key ; i++){
            free(env.new_shared_key[i]->key);
            free(env.new_shared_key[i]);
        }
        free(env.new_shared_key);
    }

    /* Private Key List Free */
    for(i = 0 ; i < nr_threads ; i++){
        struct per_thread_config *config = param[i].config;
        int j;
        if(config->use_private_key){
            for(j = 0 ; j < *config->nr_exist_key; j++){
                free(config->exist_key[j]->key);
                free(config->exist_key[j]);
            }
            free(config->exist_key);
            free(config->nr_exist_key);

            for(j = 0 ; j < *config->nr_new_key; j++){
                free(config->new_key[j]->key);
                free(config->new_key[j]);
            }
            free(config->new_key);
            free(config->nr_new_key);
        }
        free(config);
    }
    free(thread);
    free(param);
    return 0;
}
