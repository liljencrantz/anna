#include <sys/time.h>

//#include "duck_string_naive.c"
#include "duck_string_internal.c"

long long get_time()
{
	struct timeval time_struct;
	gettimeofday( &time_struct, 0 );
	return 1000000ll*time_struct.tv_sec+time_struct.tv_usec;
}

duck_string_append_test(size_t min_len, size_t max_len, wchar_t *msg)
{
  long long start_time = get_time();
  int j=0;
  size_t stop = 5000000;

  for(j=0;j<10;j++)
    {
      int i;
      duck_string_t a, b, c, d, e;
  
      size_t chars_done=0;
      
      duck_string_init(&a);
      duck_string_init(&b);
      duck_string_init(&c);
      duck_string_init(&d);
      duck_string_init(&e);
      duck_string_init_from_ptr(&e, L"abcdefghijklmnopqrstuvwxyz0123456789ABCD", 40);
      for(i=0; i<= max_len/20; i++)
	{
	  duck_string_append(&d, &e, 0, 40);
	}
      
      while(1)
	{
	  size_t len = min_len + rand()%(max_len-min_len);
	  size_t offset = rand()%max_len;
	  duck_string_t *dest;
	  switch(rand()%3)
	    {
	    case 0:
	      dest = &a;
	      break;
	    case 1:
	      dest = &b;
	      break;
	    case 2:
	      dest = &c;
	      break;
	    }
	  duck_string_append(dest, &d, offset, len);
	  chars_done += len;
	  
	  if(chars_done > stop) 
	    {
	      break;
	    }
	}
    }
  
  long long stop_time = get_time();
  
  wprintf(L"Copied %ls @ %f chars/s\n",
	  msg, 1000000.0*((double)stop*10)/(stop_time-start_time));
}

int main()
{
  duck_string_append_test(1, 16, L"short strings");  
  duck_string_append_test(400, 800, L"long strings");  
  return 0;
}
