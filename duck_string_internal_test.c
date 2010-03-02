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
	if(cnt %1000 == 0)
	{
	    wprintf(L"Lap %d\n", cnt);
	    
	}
	
	duck_string_truncate(a, mini(64, duck_string_get_length(a)));
	
	switch(rand() % 3)
	{
	    case 0:
	    {
		
		duck_string_destroy(f);
		duck_string_init_from_string(f, L"tralalahejhoppsansa", 18);
		duck_string_append(a, f, 0, 18);
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
		duck_string_append(a, c, 0, rand() % (duck_string_get_length(c)+1));
		duck_string_append(a, e, 0, rand() % (duck_string_get_length(e)+1));
		
		wchar_t *data = L"abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
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
		}
		
		for(i=0; i<length; i++)
		{
		    if(duck_string_get_char(b, i) != duck_string_get_char(a, i))
		    {
			wprintf(L"Truncation error.\n");
		    	
		    }
		    
		}
		
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
    duck_string_t a, b, c, d, e, f;
    duck_string_init(&a);
    duck_string_init(&e);
    duck_string_init(&f);
    duck_string_init_from_string(&b, L"TRALALA", 7);
    duck_string_init_from_string(&c, L"TRALALA", 3);
    duck_string_init_from_string(&d, L"ABCgggggg", 9);

    srand(time(0));
    
    duck_string_random_test(&a, &b, &c, &d, &e, &f, 100000);

    duck_string_destroy(&a);
    duck_string_destroy(&b);
    duck_string_destroy(&c);
    duck_string_destroy(&d);
    duck_string_destroy(&e);
    duck_string_destroy(&f);
}
