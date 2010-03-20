#include <sys/time.h>

#include "anna_string_naive.c"
//#include "anna_string_internal.c"

long long get_time()
{
	struct timeval time_struct;
	gettimeofday( &time_struct, 0 );
	return 1000000ll*time_struct.tv_sec+time_struct.tv_usec;
}

anna_string_append_test(size_t min_len, size_t max_len, wchar_t *msg)
{
  long long start_time = get_time();
  int j=0;
  size_t stop = 5000000;

  for(j=0;j<10;j++)
    {
      int i;
      anna_string_t a, b, c, d, e;
  
      size_t chars_done=0;
      
      anna_string_init(&a);
      anna_string_init(&b);
      anna_string_init(&c);
      anna_string_init(&d);
      anna_string_init(&e);
      anna_string_init_from_ptr(&e, L"abcdefghijklmnopqrstuvwxyz0123456789ABCD", 40);
      for(i=0; i<= max_len/20; i++)
	{
	  anna_string_append(&d, &e, 0, 40);
	}
      
      while(1)
	{
	  size_t len = min_len + rand()%(max_len-min_len);
	  size_t offset = rand()%max_len;
	  anna_string_t *dest;
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
	  anna_string_append(dest, &d, offset, len);
	  chars_done += len;
	  
	  if(chars_done > stop) 
	    {
	      break;
	    }
	}
    }
  
  long long stop_time = get_time();
  
  wprintf(L"Appended %ls @ %f million chars/s\n",
	  msg, 1.0*((double)stop*10)/(stop_time-start_time));
}

int main()
{
    anna_string_append_test(1, 16, L"short strings (1-16 chars)");  
    anna_string_append_test(1, 800, L"mixed strings (1-800 chars)");  
    anna_string_append_test(400, 800, L"long strings (400-800 chars)");  
    return 0;
}
