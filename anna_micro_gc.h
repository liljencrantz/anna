
typedef struct 
{
    
}
    amgc_context_t;

#define AMGC_NODE 0
#define AMGC_STACK 1
#define AMGC_TYPE 2
#define AMGC_OBJECT 3



//typedef struct void(*amgc_function_t)(void *ptr, amgc_context_t *context);

void amgc_init();
void *amgc_alloc(size_t s, int type);

void amgc_run_gc();

