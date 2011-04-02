#include <stdlib.h>
#include <stdio.h>
#include <wchar.h>
#include <assert.h>
#include <string.h>
#include <time.h>

#include "anna_string_internal.h"
#include "anna.h"

static void anna_string_random_test(
    anna_string_t *a,
    anna_string_t *b,
    anna_string_t *c,
    anna_string_t *d,
    anna_string_t *e,
    anna_string_t *f,
    int count)
{
    int cnt;
    
    for(cnt=0; cnt<count; cnt++)
    {
	
	asi_truncate(a, mini(4096, asi_get_length(a)));
	asi_truncate(b, mini(4096, asi_get_length(b)));
	asi_truncate(c, mini(4096, asi_get_length(c)));
	asi_truncate(d, mini(4096, asi_get_length(d)));
	asi_truncate(e, mini(4096, asi_get_length(e)));
	asi_truncate(f, mini(4096, asi_get_length(f)));
	//asi_print_debug(a);
	
	switch(rand() % 5)
	{
	    case 0:
	    {	
	        asi_destroy(f);
		asi_init_from_ptr(f, L"valsi udfgha sljdcv asldfka sfgyerkfs djchakjyg", 45);
		asi_append(a, f, 0, 45);
		size_t offset = rand()%(asi_get_length(f)+1);
		size_t length = rand()%(asi_get_length(f)-offset+1);
		
		asi_substring(b, a, offset, length);
		int i;
		if(asi_get_length(b) != length)
		{
		    wprintf(L"Substring error. Wrong length. Expected %d, got %d\n",
			    length, asi_get_length(b));
		    asi_print_debug(b);
		    CRASH;
		}
		for(i=0; i<length; i++)
		{
		    if(asi_get_char(b, i) != asi_get_char(a, i+offset))
		    {
			wprintf(L"Substring error. Length = %d, offset = %d, a[%d] = %lc, b[%d] = %lc\n", 
				length, offset,
				i+offset, asi_get_char(a, i+offset),
				i, asi_get_char(b, i));
			asi_print_debug(a);
			asi_print_debug(b);
			CRASH;
		    }	    
		}
		break;	    
	    }
	    
	    case 1:
	    {
	      
		int i;
		for(i=0; i<4; i++)
		{
		    asi_append(a, c, 0, rand() % (asi_get_length(c)+1));
		    asi_append(a, e, 0, rand() % (asi_get_length(e)+1));
		}
		
		wchar_t *data = L"abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
		int len = wcslen(data);
		
		size_t offset = rand()%(asi_get_length(a)+1);
		size_t length = rand()%(asi_get_length(a)-offset+1);
		
		for(i=0; i < length;i++)
		{
		    asi_set_char(a, i+offset, data[i%len]);
		}
		
		for(i=0; i < length;i++)
		{
		    if( asi_get_char(a, i+offset) != data[i%len])
		    {
			wprintf(L"ERROR2!!!\n");		    
			exit(1);
		    }	
		}
		break;
		
	    }
	    case 2:
	    {
		int i;
	    
		size_t length = rand()%(asi_get_length(a)+1);
		asi_substring(b, a, 0, asi_get_length(a));
		asi_truncate(a, length);
		if(length != asi_get_length(a))
		{
		    wprintf(L"Wrong length after truncation!\n");
		    exit(1);
		}
		
		for(i=0; i<length; i++)
		{
		    if(asi_get_char(b, i) != asi_get_char(a, i))
		    {
			wprintf(L"Truncation error.\n");
			exit(1);		    	
		    }   
		}		
		break;
	    }
	case 3:
	  {
	    int i;
	    
	    size_t src_offset = rand()%(asi_get_length(c)+1);
	    size_t src_length = rand()%(asi_get_length(c)-src_offset+1);
	    
	    size_t dest_offset = rand()%(asi_get_length(a)+1);
	    size_t dest_length = rand()%(asi_get_length(a)-dest_offset+1);
	    
	    asi_substring(b, a, 0, asi_get_length(a));
	    asi_replace(a, c, dest_offset, dest_length, src_offset, src_length);
	    if(asi_get_length(a) != asi_get_length(b)-dest_length+src_length)
	      {
		wprintf(L"String replacement error. Tried to replace %d .. %d of string(%d)\n",
			dest_offset, dest_offset+dest_length, asi_get_length(b));
		asi_print_debug(b);
		wprintf(L"with %d .. %d of string(%d)\n",
			src_offset, src_offset+src_length, asi_get_length(c));
		asi_print_debug(c);
		wprintf(L"got string:\n", i);
		asi_print_debug(a);
		wprintf(L"Got string length %d, but expected %d.\n",asi_get_length(a),
			asi_get_length(b)-dest_length+src_length);
		exit(1);		
	      }

	    
	    for(i=0; i<asi_get_length(a); i++) {
	      wchar_t c1 = asi_get_char(a, i);
	      wchar_t c2;
	      if(i < dest_offset)
		{
		  c2 = asi_get_char(b, i);
		}
	      else if (i < dest_offset + src_length) 
		{
		  c2 = asi_get_char(c, i-dest_offset+src_offset);
		}
	      else
		{
		  c2 = asi_get_char(b, i+dest_length-src_length);		  
		}
	      if(c1 != c2)
		{
		  wprintf(L"String replacement error. Tried to replace %d .. %d of string(%d)\n",
			  dest_offset, dest_offset+dest_length, asi_get_length(b));
		  asi_print_debug(b);
		  wprintf(L"with %d .. %d of string(%d)\n",
			  src_offset, src_offset+src_length, asi_get_length(c));
		  asi_print_debug(c);
		  wprintf(L"but got error at char %d of resulting string:\n", i);
		  asi_print_debug(a);
		  wprintf(L"Got %lc, but expected %lc.\n", c1, c2);
		  exit(1); 
		}
	    }
	    break;
	    
	  }
	case 4:
	  {
	    asi_destroy(f);
	    asi_init_from_ptr(f, L"valsi udfgha sljdcv asldfka sfgyerkfs djchakjyg fasdf sdf valsi udfgha sljdcv asldfka sfgyerkfs djchakjyg fasdf sdf valsi udfgha sljdcv asldfka sfgyerkfs djchakjyg fasdf sdf valsi udfgha sljdcv asldfka sfgyerkfs djchakjyg fasdf sdf valsi udfgha sljdcv asldfka sfgyerkfs djchakjyg fasdf sdf valsi udfgha sljdcv asldfka sfgyerkfs djchakjyg fasdf sdf valsi udfgha sljdcv asldfka sfgyerkfs djchakjyg fasdf sdf valsi udfgha sljdcv asldfka sfgyerkfs djchakjyg fasdf sdf ", 400);
	    int i;
	    
	    for(i=0; i<240; i++)
	      {
		asi_append(a, f, 0, rand()%400);
		asi_append(b, f, 0, rand()%400);
	      }
	    
	  }
	  
	}
	
	anna_string_t *tmp = a;
	a=b;
	b=c;
	c=d;
	d=e;
	e=f;
	f=tmp;
    }
}

int main()
{
  int i;
  srand(time(0));
  for(i=0; i < 30; i++)
    {
      
      anna_string_t a, b, c, d, e, f;
      asi_init(&a);
      asi_init(&e);
      asi_init(&f);
      asi_init_from_ptr(&b, L"TR ALALA klsdf jkasfg askjfgak sjdfhg askjfh agskjdfh askjdfh asgkjdf g", 7);
      asi_init_from_ptr(&c, L"TRA LAL Afas dkla sdj", 3);
      asi_init_from_ptr(&d, L"ABC gggg ggafl skjdhf klasjdh", 9);
      
      anna_string_random_test(&a, &b, &c, &d, &e, &f, 1000);
      
      asi_destroy(&a);
      asi_destroy(&b);
      asi_destroy(&c);
      asi_destroy(&d);
      asi_destroy(&e);
      asi_destroy(&f);
    }

  return 0;
}
