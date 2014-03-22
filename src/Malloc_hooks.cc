
#define MALLOC_HOOKS_ENABLED 0

#if MALLOC_HOOKS_ENABLED

#include <malloc.h>
     
void *(* old_malloc_hook)(size_t size, const void *caller) = 0;
void (* old_free_hook)(void *ptr, const void *caller) = 0;

static void init_hook();
static void * malloc_hook(size_t, const void *);
static void free_hook(void *, const void *);
     
   // Override initializing hook from the C library
void (*__malloc_initialize_hook)() = init_hook;
     
static void
init_hook (void)
{
  old_malloc_hook = __malloc_hook;
  old_free_hook = __free_hook;
  __malloc_hook = malloc_hook;
  __free_hook = free_hook;
}
     
static void *
malloc_hook (size_t size, const void *caller)
{
   void *result;
   /* Restore all old hooks */
   __malloc_hook = old_malloc_hook;
   __free_hook = old_free_hook;
   /* Call recursively */
   result = malloc (size);
   /* Save underlying hooks */
   old_malloc_hook = __malloc_hook;
   old_free_hook = __free_hook;
   /* printf might call malloc, so protect it too. */
   printf ("malloc (%u) returns %p\n", (unsigned int) size, result);
   /* Restore our own hooks */
   __malloc_hook = malloc_hook;
   __free_hook = free_hook;
   return result;
}
static void
free_hook (void *ptr, const void *caller)
{
   /* Restore all old hooks */
   __malloc_hook = old_malloc_hook;
   __free_hook = old_free_hook;

   /* Call recursively */
   free (ptr);

   /* Save underlying hooks */
   old_malloc_hook = __malloc_hook;
   old_free_hook = __free_hook;

   /* printf might call free, so protect it too. */
   printf ("freed pointer %p\n", ptr);

   /* Restore our own hooks */
   __malloc_hook = malloc_hook;
   __free_hook = free_hook;
}

#endif
