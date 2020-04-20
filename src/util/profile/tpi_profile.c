#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

//#define DEBUG 1
//#define ENABLE_TPI_PROFILE

#ifdef ENABLE_TPI_PROFILE
#include <mpi.h>

#define TPI_MAX_LEVEL 1
#define TPI_MAX_NTIMER  22
#define TPI_MAX_NAMELEN 40
static double timer_start, timer_stop;
static double timer_push[TPI_MAX_LEVEL], timer_pop[TPI_MAX_LEVEL];

static int max_timers = TPI_MAX_NTIMER;
static int *active_timers[TPI_MAX_LEVEL] = { NULL };
static double *timer_total[TPI_MAX_LEVEL] = { NULL };

static char stack_name[TPI_MAX_LEVEL][TPI_MAX_NAMELEN];
static int stack_name_in_use[TPI_MAX_LEVEL] = { 0 };

static const char *timer_names[TPI_MAX_NTIMER] = {
    "grand_total" /* 0 */ ,
    "sd_t_*_[1-9]" /* 1 */ ,
    "tce_sort2" /* 2 */ ,
    "tce_sortacc2" /* 3 */ ,
    "tce_sort4" /* 4 */ ,
    "tce_sortacc4" /* 5 */ ,
    "tce_sort6" /* 6 */ ,
    "tce_sortacc6" /* 7 */ ,
    "tce_sort8" /* 8 */ ,
    "tce_sortacc8" /* 9 */ ,
    "dgemm" /* 10 */ ,
    "ga_get" /* 11 */ ,
    "ga_put" /* 12 */ ,
    "ga_acc" /* 13 */ ,
    "nxtask" /* 14 */ ,
    "add_block" /* 15 */ ,
    "ga_sync" /* 16 */ ,
    "int_2e4c" /* 17 */ ,
    "ipos1" /* 18 */ ,
    "nxtask_sync" /* 19 (create, destroy GA array) */ ,
    "tce_sort_4kg" /* 20 */,
    NULL
};

static int env_print_per_rank = -1;
static int tpi_level = -1;

static inline void __read_tpi_env()
{
    /* only read from environment once */
    if (tpi_level < 0) {
        char *envval = 0;
        int level = 0;
        envval = getenv("TPI_LEVEL");
        if (envval && strlen(envval)) {
            level = atoi(envval);
            switch (level) {
            case 1:
                tpi_level = 1;
                break;
            case 2:
                tpi_level = 2;
                break;
            case 3:
                tpi_level = 3;
                break;
            default:
                tpi_level = 0;
            }
        }
        else {
            tpi_level = 0;
        }
    }

    /* only read from environment once */
    if (env_print_per_rank < 0) {
        char *envval = 0;
        env_print_per_rank = 0;

        envval = getenv("TPI_PRINT_PER_RANK");
        if (envval && strlen(envval)) {
            int intval = atoi(envval);
            if(intval == 1)
                env_print_per_rank = 1;
        }
    }
}

void tpi_start_(void)
{
    int rank = 0;
    int level = 0, i = 0;;

#if defined(DEBUG)
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    fprintf(stderr, "%4d: tpi_start \n", rank);
    fflush(stderr);
#endif

    __read_tpi_env();

    max_timers = TPI_MAX_NTIMER;

    /* initialize timers for all possible levels */
    for (level = 0; level < TPI_MAX_LEVEL; level++) {
        active_timers[level] = malloc(max_timers * sizeof(int));
        assert(active_timers[level] != NULL);
        timer_total[level] = malloc(max_timers * sizeof(double));
        assert(timer_total[level] != NULL);

        for (i = 0; i < max_timers; i++)
            active_timers[level][i] = 0;
        for (i = 0; i < max_timers; i++)
            timer_total[level][i] = 0.0;
    }

    return;
}

void tpi_stop_(void)
{
    int rank = 0;
    int level = 0;
#if defined(DEBUG)
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    fprintf(stderr, "%4d: tpi_stop \n", rank);
    fflush(stderr);
#endif

    for (level = 0; level < TPI_MAX_LEVEL; level++) {
        free(active_timers[level]);
        free(timer_total[level]);
    }

    return;
}
#else
void tpi_start_(void)
{
    /* do nothing */
}

void tpi_stop_(void)
{
    /* do nothing */
}
#endif

#ifdef ENABLE_TPI_PROFILE
static inline void __push_name(char *name, int level, int n)
{
    int rank = 0;

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
#if defined(DEBUG)
    fprintf(stderr, "%4d: tpi_push_name \n", rank);
    fflush(stderr);
#endif

    if (stack_name_in_use[level] == 1) {
        fprintf(stderr, "tpi_push_name: name[%d] is already in use by %s \n", level, stack_name);
        fflush(stderr);
    }

    if (n > TPI_MAX_NAMELEN) {
        fprintf(stderr, "tpi_push_name: name[%d] is too long (max = %d) \n", level,
                TPI_MAX_NAMELEN);
        fflush(stderr);
    }

    memset(stack_name[level], '\0', TPI_MAX_NAMELEN);
    strncpy(stack_name[level], name, n);

    if (rank == 0) {
        fprintf(stdout, "TPI push name: %s \n", name);
        fflush(stdout);
    }

    int i;
    for (i = 0; i < max_timers; i++)
        active_timers[level][i] = 0;
    for (i = 0; i < max_timers; i++)
        timer_total[level][i] = 0.0;

    stack_name_in_use[level] = 1;

    timer_push[level] = MPI_Wtime();
    return;
}

static inline void __pop_name(char *name, int level, int n)
{
    int rank = 0;
    int size = 1;
    int i;

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
# ifdef DEBUG
    fprintf(stderr, "%4d: tpi_pop_name \n", rank);
    fflush(stderr);
# endif

    timer_pop[level] = MPI_Wtime();

    /* total time */
    timer_total[level][0] = (timer_pop[level] - timer_push[level]);
    active_timers[level][0] = 1;

    if (rank == 0) {
        fprintf(stdout, "TPI pop name: %s \n", name);
        fflush(stdout);
    }

    if (stack_name_in_use[level] == 0) {
        fprintf(stderr, "tpi_pop_name: stack_name[%d] is undefined \n", level);
        fflush(stderr);
    }

    if (0 != strncmp(name, stack_name[level], n)) {
        fprintf(stderr, "tpi_pop_name: pop name does not match push name[%d] (%s,%s) \n",
                level, name, stack_name[level]);
        fflush(stderr);
    }

    if (env_print_per_rank == 1) {
        for (i = 0; i < max_timers; i++) {
            if (active_timers[level][i] > 0) {
                if (timer_names[i] != NULL) {
                    fprintf(stderr, "rank = %d name = %s timer = %s       subtotal = %lf \n",
                            rank, stack_name[level], timer_names[i], timer_total[level][i]);
                    fflush(stderr);
                }
                else {
                    fprintf(stderr, "rank = %d name = %s timer = %d-undefined subtotal = %lf \n",
                            rank, stack_name[level], i, timer_total[level][i]);
                    fflush(stderr);
                }
            }
        }
    }
    else {

        int *active_timers_sum = NULL;
        double *timers_min = NULL;
        double *timers_max = NULL;
        double *timers_avg = NULL;

        active_timers_sum = malloc(max_timers * sizeof(int));
        timers_min = malloc(max_timers * sizeof(double));
        timers_max = malloc(max_timers * sizeof(double));
        timers_avg = malloc(max_timers * sizeof(double));
        assert(active_timers_sum != NULL && timers_min != NULL && timers_max != NULL &&
               timers_avg != NULL);

        if (rank == 0)
            for (i = 0; i < max_timers; i++)
                timers_min[i] = 10000000.0;
        if (rank == 0)
            for (i = 0; i < max_timers; i++)
                timers_max[i] = 0.0;
        if (rank == 0)
            for (i = 0; i < max_timers; i++)
                timers_avg[i] = -1.0;

        MPI_Reduce(active_timers[level], active_timers_sum, max_timers,
                   MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);
        MPI_Reduce(timer_total[level], timers_min, max_timers,
                   MPI_DOUBLE, MPI_MIN, 0, MPI_COMM_WORLD);
        MPI_Reduce(timer_total[level], timers_max, max_timers,
                   MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);
        MPI_Reduce(timer_total[level], timers_avg, max_timers,
                   MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD);

        double ascal = 1.0 / size;
        if (rank == 0)
            for (i = 0; i < max_timers; i++)
                timers_avg[i] *= ascal;

        for (i = 0; i < max_timers; i++) {
            if (rank == 0 && active_timers_sum[i] > 0) {
                if (timer_names[i] != NULL) {
                    fprintf(stderr, "name = %s timers = %s min = %lf max = %lf avg = %lf \n",
                            stack_name[level], timer_names[i], timers_min[i], timers_max[i],
                            timers_avg[i]);
                    fflush(stderr);
                }
                else {
                    fprintf(stderr,
                            "name = %s timers = %9d (undefined) min = %lf max = %lf avg = %lf \n",
                            stack_name[level], i, timers_min[i], timers_max[i], timers_avg[i]);
                    fflush(stderr);
                }
            }
        }

        free(active_timers_sum);
        free(timers_min);
        free(timers_max);
        free(timers_avg);
    }

    fflush(stderr);

    memset(stack_name[level], '\0', TPI_MAX_NAMELEN);
    stack_name_in_use[level] = 0;

    return;
}

void tpi_push_name_level1_(char *name, int n)
{
    if (tpi_level >= 1) {
        return __push_name(name, 0, n);
    }
}

void tpi_push_name_level2_(char *name, int n)
{
    if (tpi_level >= 2) {
        return __push_name(name, 1, n);
    }
}

void tpi_push_name_level3_(char *name, int n)
{
    if (tpi_level >= 3) {
        return __push_name(name, 2, n);
    }
}


void tpi_pop_name_level1_(char *name, int n)
{
    if (tpi_level >= 1) {
        return __pop_name(name, 0, n);
    }
}

void tpi_pop_name_level2_(char *name, int n)
{
    if (tpi_level >= 2) {
        return __pop_name(name, 1, n);
    }
}

void tpi_pop_name_level3_(char *name, int n)
{
    if (tpi_level >= 3) {
        return __pop_name(name, 2, n);
    }
}
#else
void tpi_push_name_level1_(char *name, int n)
{
    /* do nothing */
}

void tpi_push_name_level2_(char *name, int n)
{
    /* do nothing */
}

void tpi_push_name_level3_(char *name, int n)
{
    /* do nothing */
}

void tpi_pop_name_level1_(char *name, int n)
{
    /* do nothing */
}

void tpi_pop_name_level2_(char *name, int n)
{
    /* do nothing */
}

void tpi_pop_name_level3_(char *name, int n)
{
    /* do nothing */
}
#endif

#ifdef ENABLE_TPI_PROFILE
void tpi_start_timer_(int *id)
{
    int level;
#if defined(DEBUG)
    int rank = 0;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    fprintf(stderr, "%4d: tpi_start_timer \n", rank);
    fflush(stderr);
#endif

    if ((*id) > max_timers) {
        fprintf(stderr, "tpi_start_timer: timer undefined %d\n", (*id));
        fflush(stderr);
    }

    for (level = 0; level < TPI_MAX_LEVEL; level++) {
        if (stack_name_in_use[level]) {
            active_timers[level][*id] = 1;
        }
    }

    timer_start = MPI_Wtime();

    return;
}

void tpi_stop_timer_(int *id)
{
    int level = 0;
#if defined(DEBUG)
    int rank = 0;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    fprintf(stderr, "%4d: tpi_stop_timer \n", rank);
    fflush(stderr);
#endif

    timer_stop = MPI_Wtime();

    if ((*id) > max_timers) {
        fprintf(stderr, "tpi_stop_timer: timer undefined %d\n", (*id));
        fflush(stderr);
    }

    for (level = 0; level < TPI_MAX_LEVEL; level++) {
        if (stack_name_in_use[level] == 0)
            continue;

        if (active_timers[level][*id] == 0) {
            fprintf(stderr, "tpi_stop_timer: timer[%d,%d] not started \n", level, *id);
            fflush(stderr);
            continue;
        }

        timer_total[level][*id] += (timer_stop - timer_start);
    }

    return;
}
#else
void tpi_start_timer_(int *id)
{
    /* do nothing */
}

void tpi_stop_timer_(int *id)
{
    /* do nothing */
}
#endif
