/* Define some elementary types: */
#define TRUE 1
#define FALSE 0
typedef unsigned char byte;
typedef unsigned short word;
typedef unsigned long lword;

#define MIN(A,B) ((A) < (B) ? (A) : (B))
#define MAX(A,B) ((A) > (B) ? (A) : (B))

lword min(lword v1, lword v2) {	return MIN(v1,v2); }
lword max(lword v1, lword v2) {	return MAX(v1,v2); }

#define DEBUG_MEM_ALLOC
#ifdef DEBUG_MEM_ALLOC

lword alloced_space = 0L;
bool trace_mem_alloc = FALSE;

void *my_malloc(lword size)
{
	void *mem = malloc(size+8);
	
	alloced_space += size;
	
	if (trace_mem_alloc)
		printf("\nmalloc(%ld)", size);
	
	if (mem == NULL)
	{	fprintf(stderr, " FATAL ERROR: malloc for size %ld failed.\n", size);
		{	int i = 0; i = 1/i; }
	}
	
	strcpy((char*)mem, "FJF");
	((byte*)mem) += 4;

	*(lword*)mem = size;
	((byte*)mem) += 4;
	
	
	// initialize
	for (size_t i = 0; i < size; i++)
		((byte*)mem)[i] = 0;

	if (trace_mem_alloc)
		printf(" = #%08lX# total=%ld", (lword)mem, alloced_space);
	 
	return mem;
}

void my_free(void *mem)
{
	if (mem == NULL) return;

	((byte*)mem) -= 4;
	alloced_space -= *(lword*)mem;

	if (trace_mem_alloc)
		printf("\nfree(#%08lX#) total=%ld", (lword)mem, alloced_space);
	
	((byte*)mem) -= 4;
	if (strcmp((char*)mem, "FJF"))
	{	fprintf(stderr, " FATAL ERROR: illegale free for #%08lX#.\n", (lword)mem);
		{	int i = 0; i = 1/i; }
	}
	
	//free(mem);
}

void my_assert_alloced(void *mem)
{
	if (trace_mem_alloc)
		printf("\ncheck(#%08lX#)", (lword)mem);

	if (mem == NULL) return;
	
	
	((byte*)mem) -= 8;
	if (strcmp((char*)mem, "FJF"))
	{	fprintf(stderr, " FATAL ERROR: pointer at #%08lX# is illegal.\n", (lword)mem);
		{	int i = 0; i = 1/i; }
	}
}

/* Allocate a record of type T: */
#define ALLOC(T)    (T*)my_malloc(sizeof(T)) 
/* Allocate an array of N records of type T: */
#define NALLOC(N,T) (T*)my_malloc((N)*sizeof(T))
/* Allocate space in which the string L would fit: */
#define SALLOC(L)   (char*)my_malloc((strlen(L)+1)*sizeof(char))

#define ASSERT_ALLOCED(L) my_assert_alloced(L)

#define FREE(L)    my_free(L); L = NULL

#else // DEBUG_MEM_ALLOC

#define ALLOC(T)    (T*)malloc(sizeof(T)) 
/* Allocate an array of N records of type T: */
#define NALLOC(N,T) (T*)malloc((N)*sizeof(T))
/* Allocate space in which the string L would fit: */
#define SALLOC(L)   (char*)malloc((strlen(L)+1)*sizeof(char))

#define ASSERT_ALLOCED(L) 

#define FREE(L)    if (L != NULL) free(L); L = NULL

#endif // DEBUG_MEM_ALLOC


/* Make an allocated copy of a string: */
char *strcopy(char *s)
{
  char *r = SALLOC(s);
  strcpy(r,s);
  return r;
}

#define FOREACH_IN_LIST_REF_N(T,E,L,N) CMMFPointer<T>*E; for(E=&L;!(*E).IsNull();E=&(*E)->N)
#define FOREACH_IN_LIST_N(T,E,L,N) for(T*E=L;E!=NULL;E=E->N)
#define DELETE_LIST_N(T,L,N) for (T*e=L;e!=NULL;){T*n=e->N;delete e;e=n;}
#define NTH_IN_LIST_N(T,E,M,L,F,N) T*E=L; for(word i=F;i<M&&E!=NULL;i++,E=E->N);
#define NTH1_IN_LIST_N(T,E,M,L,N) NTH_IN_LIST_N(T,E,M,L,1,N)
#define NTH0_IN_LIST_N(T,E,M,L,N) NTH_IN_LIST_N(T,E,M,L,0,N)

#define FOREACH_IN_LIST_REF(T,E,L) FOREACH_IN_LIST_REF_N(T,E,L,next)
#define FOREACH_IN_LIST(T,E,L) FOREACH_IN_LIST_N(T,E,L,next)
#define DELETE_LIST(T,L) DELETE_LIST_N(T,L,next)
#define NTH1_IN_LIST(T,E,N,L) NTH1_IN_LIST_N(T,E,N,L,next)
#define NTH0_IN_LIST(T,E,N,L) NTH0_IN_LIST_N(T,E,N,L,next)



#define ASSERT(X) if (!(X)) {fprintf(stdout, __FILE__ ":%d Assert " #X " failed\n", __LINE__);\
         {int i; i = 0; i = 3/i; }}

