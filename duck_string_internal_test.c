
//#include "duck_string_naive.c"
#include "duck_string_internal.c"

duck_string_random_test(duck_string_t *a,
			duck_string_t *b,
			duck_string_t *c,
			duck_string_t *d,
			duck_string_t *e,
			duck_string_t *f,
			int count)
{
    int cnt;
    
    for(cnt=0; cnt<count; cnt++)
    {
	
	duck_string_truncate(a, mini(4096, duck_string_get_length(a)));
	duck_string_truncate(b, mini(4096, duck_string_get_length(b)));
	duck_string_truncate(c, mini(4096, duck_string_get_length(c)));
	duck_string_truncate(d, mini(4096, duck_string_get_length(d)));
	duck_string_truncate(e, mini(4096, duck_string_get_length(e)));
	duck_string_truncate(f, mini(4096, duck_string_get_length(f)));
	//duck_string_print(a);
	
	switch(rand() % 5)
	{
	    case 0:
	    {	
	        duck_string_destroy(f);
		duck_string_init_from_ptr(f, L"valsi udfgha sljdcv asldfka sfgyerkfs djchakjyg", 45);
		duck_string_append(a, f, 0, 45);
		size_t offset = rand()%(duck_string_get_length(f)+1);
		size_t length = rand()%(duck_string_get_length(f)-offset+1);
		
		duck_string_substring(b, a, offset, length);
		int i;
		for(i=0; i<length; i++)
		{
		    if(duck_string_get_char(b, i) != duck_string_get_char(a, i+offset))
		    {
			wprintf(L"Substring error. Length = %d, offset = %d, a[%d] = %lc, b[%d] = %lc\n", 
				length, offset,
				i+offset, duck_string_get_char(a, i+offset),
				i, duck_string_get_char(b, i));
			duck_string_print(a);
			duck_string_print(b);
			exit(1);			
		    }	    
		}
		break;	    
	    }
	    
	    case 1:
	    {
	      
		int i;
		for(i=0; i<4; i++)
		{
		    duck_string_append(a, c, 0, rand() % (duck_string_get_length(c)+1));
		    duck_string_append(a, e, 0, rand() % (duck_string_get_length(e)+1));
		}
		
		wchar_t *data = L"abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
		int len = wcslen(data);
		
		size_t offset = rand()%(duck_string_get_length(a)+1);
		size_t length = rand()%(duck_string_get_length(a)-offset+1);
		
		for(i=0; i < length;i++)
		{
		    duck_string_set_char(a, i+offset, data[i%len]);
		}
		
		for(i=0; i < length;i++)
		{
		    if( duck_string_get_char(a, i+offset) != data[i%len])
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
	    
		size_t length = rand()%(duck_string_get_length(a)+1);
		duck_string_substring(b, a, 0, duck_string_get_length(a));
		duck_string_truncate(a, length);
		if(length != duck_string_get_length(a))
		{
		    wprintf(L"Wrong length after truncation!\n");
		    exit(1);
		}
		
		for(i=0; i<length; i++)
		{
		    if(duck_string_get_char(b, i) != duck_string_get_char(a, i))
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
	    
	    size_t src_offset = rand()%(duck_string_get_length(c)+1);
	    size_t src_length = rand()%(duck_string_get_length(c)-src_offset+1);
	    
	    size_t dest_offset = rand()%(duck_string_get_length(a)+1);
	    size_t dest_length = rand()%(duck_string_get_length(a)-dest_offset+1);
	    
	    duck_string_substring(b, a, 0, duck_string_get_length(a));
	    duck_string_replace(a, c, dest_offset, dest_length, src_offset, src_length);
	    if(duck_string_get_length(a) != duck_string_get_length(b)-dest_length+src_length)
	      {
		wprintf(L"String replacement error. Tried to replace %d .. %d of string(%d)\n",
			dest_offset, dest_offset+dest_length, duck_string_get_length(b));
		duck_string_print(b);
		wprintf(L"with %d .. %d of string(%d)\n",
			src_offset, src_offset+src_length, duck_string_get_length(c));
		duck_string_print(c);
		wprintf(L"got string:\n", i);
		duck_string_print(a);
		wprintf(L"Got string length %d, but expected %d.\n",duck_string_get_length(a),
			duck_string_get_length(b)-dest_length+src_length);
		exit(1);		
	      }

	    
	    for(i=0; i<duck_string_get_length(a); i++) {
	      wchar_t c1 = duck_string_get_char(a, i);
	      wchar_t c2;
	      if(i < dest_offset)
		{
		  c2 = duck_string_get_char(b, i);
		}
	      else if (i < dest_offset + src_length) 
		{
		  c2 = duck_string_get_char(c, i-dest_offset+src_offset);
		}
	      else
		{
		  c2 = duck_string_get_char(b, i+dest_length-src_length);		  
		}
	      if(c1 != c2)
		{
		  wprintf(L"String replacement error. Tried to replace %d .. %d of string(%d)\n",
			  dest_offset, dest_offset+dest_length, duck_string_get_length(b));
		  duck_string_print(b);
		  wprintf(L"with %d .. %d of string(%d)\n",
			  src_offset, src_offset+src_length, duck_string_get_length(c));
		  duck_string_print(c);
		  wprintf(L"but got error at char %d of resulting string:\n", i);
		  duck_string_print(a);
		  wprintf(L"Got %lc, but expected %lc.\n", c1, c2);
		  exit(1); 
		}
	    }
	    break;
	    
	  }
	case 4:
	  {
	    duck_string_destroy(f);
	    duck_string_init_from_ptr(f, L"valsi udfgha sljdcv asldfka sfgyerkfs djchakjyg fasdf sdf valsi udfgha sljdcv asldfka sfgyerkfs djchakjyg fasdf sdf valsi udfgha sljdcv asldfka sfgyerkfs djchakjyg fasdf sdf valsi udfgha sljdcv asldfka sfgyerkfs djchakjyg fasdf sdf valsi udfgha sljdcv asldfka sfgyerkfs djchakjyg fasdf sdf valsi udfgha sljdcv asldfka sfgyerkfs djchakjyg fasdf sdf valsi udfgha sljdcv asldfka sfgyerkfs djchakjyg fasdf sdf valsi udfgha sljdcv asldfka sfgyerkfs djchakjyg fasdf sdf ", 400);
	    int i;
	    duck_debug=1;
	    
	    for(i=0; i<240; i++)
	      {
		duck_string_append(a, f, 0, rand()%400);
		duck_string_append(b, f, 0, rand()%400);
	      }

	    duck_debug=0;
	    
	  }
	  
	}
	
	duck_string_t *tmp = a;
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
      
      duck_string_t a, b, c, d, e, f;
      duck_string_init(&a);
      duck_string_init(&e);
      duck_string_init(&f);
      duck_string_init_from_ptr(&b, L"TR ALALA klsdf jkasfg askjfgak sjdfhg askjfh agskjdfh askjdfh asgkjdf g", 7);
      duck_string_init_from_ptr(&c, L"TRA LAL Afas dkla sdj", 3);
      duck_string_init_from_ptr(&d, L"ABC gggg ggafl skjdhf klasjdh", 9);
      
      duck_string_random_test(&a, &b, &c, &d, &e, &f, 1000);
      
      duck_string_destroy(&a);
      duck_string_destroy(&b);
      duck_string_destroy(&c);
      duck_string_destroy(&d);
      duck_string_destroy(&e);
      duck_string_destroy(&f);
    }
  return 0;
}
