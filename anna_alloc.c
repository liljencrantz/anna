#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <wchar.h>
#include <assert.h>
#include <string.h>

#include "common.h"
#include "anna.h"
#include "anna_alloc.h"
#include "anna_vm.h"
#include "anna_function.h"
#include "anna_member.h"
#include "anna_int.h"
#include "anna_list.h"
#include "anna_hash.h"

array_list_t anna_alloc = AL_STATIC;
int anna_alloc_count=0;
int anna_alloc_gc_block_counter;
int anna_alloc_run_finalizers=1;

static void anna_alloc_unmark(void *obj)
{
    *((int *)obj) &= (~ANNA_USED);
}

__pure static inline int anna_object_member_is_blob(anna_type_t *type, size_t off)
{
    return type->member_blob[off];
    
}

__pure static inline int anna_object_member_is_alloc(anna_type_t *type, size_t off)
{
    return type->member_blob[off] == ANNA_GC_ALLOC;
    
}

__pure static inline int anna_type_member_is_blob(anna_type_t *type, size_t off)
{
    return type->static_member_blob[off];
}

__pure static inline int anna_type_member_is_alloc(anna_type_t *type, size_t off)
{
    return type->static_member_blob[off] == ANNA_GC_ALLOC;
}

void anna_alloc_mark_type(anna_type_t *type);
//static void anna_alloc_mark(void *obj);
static void anna_alloc_mark_node(anna_node_t *o);

static void anna_alloc_mark_function(anna_function_t *o)
{
    if( o->flags & ANNA_USED)
	return;
    o->flags |= ANNA_USED;
        
    if(o->body)
	anna_alloc_mark_node((anna_node_t *)o->body);
    if(o->definition)
	anna_alloc_mark_node((anna_node_t *)o->definition);
    anna_alloc_mark_node((anna_node_t *)o->attribute);
    anna_alloc_mark_type(o->return_type);
    anna_alloc_mark_object(o->wrapper);
    if(o->this)
	anna_alloc_mark_object(o->this);
    if(o->stack_template)
	anna_alloc_mark_stack_template(o->stack_template);
    int i;
    for(i=0; i<o->input_count; i++)
    {
#ifdef ANNA_CHECK_GC
	if(!o->input_type[i])
	{
	    wprintf(L"No type specified for input argument %d of function %ls\n", 
		    i+1, o->name);
	    CRASH;
	}
#endif	
	anna_alloc_mark_type(o->input_type[i]);
    }

    if(o->code)
	anna_vm_mark_code(o);
}

void anna_alloc_mark_stack_template(anna_stack_template_t *o)
{
    if( o->flags & ANNA_USED)
	return;
    o->flags |= ANNA_USED;
    
    if(o->parent)
	anna_alloc_mark_stack_template(o->parent);
    if(o->function)
	anna_alloc_mark_function(o->function);
    if(o->wrapper)
	anna_alloc_mark_object(o->wrapper);
    int i;
    for(i=0; i<o->count; i++)
    {
	if(o->member[i])
	    anna_alloc_mark_object(o->member[i]);
	if(o->member_type[i])
	    anna_alloc_mark_type(o->member_type[i]);
	if(o->member_declare_node[i])
	    anna_alloc_mark_node((anna_node_t *)o->member_declare_node[i]);
    }
    for(i=0; i<al_get_count(&o->import); i++)
    {
	anna_alloc_mark_stack_template((anna_stack_template_t *)al_get(&o->import, i));
    }
}

static void anna_alloc_mark_node(anna_node_t *o)
{
    if( o->flags & ANNA_USED)
	return;
    o->flags |= ANNA_USED;

    anna_node_t *this = o;
    
    switch(this->node_type)
    {
	
	case ANNA_NODE_NULL:
	case ANNA_NODE_INT_LITERAL:
	case ANNA_NODE_STRING_LITERAL:
	case ANNA_NODE_CHAR_LITERAL:
	case ANNA_NODE_FLOAT_LITERAL:
	case ANNA_NODE_IDENTIFIER:
	{
	    break;
	}
	
	case ANNA_NODE_SPECIALIZE:
	{
	    anna_error(this, L"Unimplemented node type during gc. Come back tomorrow.");
	    CRASH;
	    break;
	}		
	
	case ANNA_NODE_CAST:
	case ANNA_NODE_CONSTRUCT:
	case ANNA_NODE_CALL:
	{	    
	    anna_node_call_t *n = (anna_node_call_t *)this;
	    int i;
	    
	    anna_alloc_mark_node(n->function);
	    for(i=0; i<n->child_count; i++)
	    {
		anna_alloc_mark_node(n->child[i]);
	    }
	    
	    break;
	}
	
	
	case ANNA_NODE_MEMBER_CALL:
	{	    
	    anna_node_call_t *n = (anna_node_call_t *)this;
	    int i;
		
	    anna_alloc_mark_node(n->object);
	    for(i=0; i<n->child_count; i++)
	    {
		anna_alloc_mark_node(n->child[i]);
	    }
	    
	    break;
	}
	
	
	case ANNA_NODE_CLOSURE:
	{
	    anna_node_closure_t *c = (anna_node_closure_t *)this;	    
	    anna_alloc_mark_function(c->payload);
	    break;
	}
	
	case ANNA_NODE_TYPE:
	{
	    anna_node_type_t *c = (anna_node_type_t *)this;
	    anna_type_t *f = c->payload;
	    anna_alloc_mark_type(f);
	    break;
	}

	case ANNA_NODE_ASSIGN:
	{
	    anna_node_assign_t *c = (anna_node_assign_t *)this;
	    anna_alloc_mark_node(c->value);
	    break;
	}


	case ANNA_NODE_MEMBER_GET:
	{
	    anna_node_member_access_t *c = (anna_node_member_access_t *)this;
	    anna_alloc_mark_node(c->object);
	    break;
	}

	case ANNA_NODE_MEMBER_SET:
	{
	    anna_node_member_access_t *g = (anna_node_member_access_t *)this;
	    anna_alloc_mark_node(g->object);
	    anna_alloc_mark_node(g->value);
	    break;
	}
	
	case ANNA_NODE_DECLARE:
	case ANNA_NODE_CONST:
	{
	    anna_node_declare_t *d = (anna_node_declare_t *)this;
	    anna_alloc_mark_node(d->value);
	    anna_alloc_mark_node(d->type);
	    break;
	}
	
	case ANNA_NODE_OR:
	case ANNA_NODE_AND:
	case ANNA_NODE_WHILE:
	{
	    anna_node_cond_t *d = (anna_node_cond_t *)this;
	    anna_alloc_mark_node(d->arg1);
	    anna_alloc_mark_node(d->arg2);
	    break;   
	}

	case ANNA_NODE_IF:
	{
	    anna_node_if_t *d = (anna_node_if_t *)this;
	    anna_alloc_mark_node(d->cond);
	    anna_alloc_mark_node((anna_node_t *)d->block1);
	    anna_alloc_mark_node((anna_node_t *)d->block2);
	    break;
	}	
	
	case ANNA_NODE_DUMMY:
	{
	    anna_node_dummy_t *d = (anna_node_dummy_t *)this;
	    anna_alloc_mark_object(d->payload);
	    break;   
	}
	
	default:
	{
	    anna_error(
		this,
		L"Don't know how to handle node of type %d during garbage collection", this->node_type);	    
	    CRASH;
	    break;
	}
    }
}

void anna_alloc_mark_type(anna_type_t *type)
{
    if( type->flags & ANNA_USED)
	return;
    type->flags |= ANNA_USED;
    size_t i;

    if(type == null_type)
    {
	anna_alloc_mark_entry(type->static_member[0]);
	return;
    }
    
    for(i=0; i<type->static_member_count; i++)
    {
#ifdef ANNA_CHECK_GC
	if(!type->static_member[i])
	{
	    wprintf(
		L"Error, static member at offet %d in type %ls is invalid\n",
		i, type->name);
	    
	    CRASH;
	}
	
	if(type->static_member_blob[i] == 69)
	{
	    wprintf(
		L"Error, static member at offet %d in type %ls has invalid blob status\n",
		i, type->name);
	    
	    CRASH;	    
	}
#endif
	if(anna_type_member_is_blob(type, i))
	{
	    if(anna_type_member_is_alloc(type, i) && type->static_member[i])
		anna_alloc_mark(type->static_member[i]);
	}
	else
	{
	    anna_alloc_mark_entry(type->static_member[i]);
	}
    }
    if(type->stack_macro)
    {
	anna_alloc_mark_stack_template(type->stack_macro);
    }
    if(type->stack)
    {
	anna_alloc_mark_stack_template(type->stack);
    }
    
}

static void anna_alloc_mark_blob(void *mem)
{
    int *mem2 = (int *)mem;
    mem2[-2] |= ANNA_USED;
}

void anna_alloc_mark_entry(anna_entry_t *e)
{
    if(!e)
	return;
    
    if(!anna_is_obj(e))
    {
	if(anna_is_alloc(e))
	{
	    anna_alloc_mark_blob(anna_as_alloc(e));
	    return;
	}
	return;
    }
    anna_object_t *obj = anna_as_obj_fast(e);
    anna_alloc_mark_object(obj);
}

void anna_alloc_mark_object(anna_object_t *obj)
{
    if( obj->flags & ANNA_USED)
	return;
    obj->flags |= ANNA_USED;
    
    if(obj == null_object)
	return;
    
    size_t i;
    if(obj->type == string_type)
    {
	return;
    }

//    if(obj->type->mid_identifier[ANNA_MID_LIST_PAYLOAD])
    if(obj->flags & ANNA_OBJECT_LIST)
    {
	/* This object is a list. Mark all list items */
	size_t sz = anna_list_get_size(obj);
	anna_entry_t **data = anna_list_get_payload(obj);
//	wprintf(L"GC LIST OF TYPE %ls WITH %d ELEMENTS\n\n", obj->type->name, sz);
	
//	KRASHAR EFTERSOM VI FYLLT LISTAN MED NULL_OBJECT, MEN SIZE ÄR MEDLEM, SÅ FÅR NULL_OBJECT SOM STORLEK, OCH SÅ KOMMER GC OCH OJ OJ OJ !!!	
	
	for(i=0; i<sz; i++)
	{
	    anna_alloc_mark_entry(data[i]);
	}	
    }
    else if(obj->type->mid_identifier[ANNA_MID_LIST_PAYLOAD])
    {
	size_t sz = anna_list_get_size(obj);
//	wprintf(L"SKIP GC OF LIST OF TYPE %ls with %d elements!!!\n\n", obj->type->name, sz);
	
    }
    
    if(obj->type->mid_identifier[ANNA_MID_HASH_PAYLOAD])
    {
	anna_hash_mark(obj);
    }    
    
    anna_type_t *t = obj->type;
    for(i=0; i<t->member_count; i++)
    {
	if(anna_object_member_is_blob(t, i))
	{
	    if(anna_object_member_is_alloc(t, i) && obj->member[i])
	    {
		anna_alloc_mark(obj->member[i]);
	    }
	}
	else
	{
	    anna_alloc_mark_entry(obj->member[i]);
	}
    }
    anna_alloc_mark_type(obj->type);
    anna_function_t *f = anna_function_unwrap(obj);
    if(f){
	anna_alloc_mark_function(f);
    }
    anna_type_t *wt = anna_type_unwrap(obj);
    if(wt){
	anna_alloc_mark_type(wt);
    }
}

static void anna_alloc_mark_vmstack(anna_vmstack_t *stack)
{
    assert((stack->flags & ANNA_ALLOC_MASK) == ANNA_VMSTACK);

    if( stack->flags & ANNA_USED)
	return;
    stack->flags |= ANNA_USED;    

    anna_entry_t **obj;
    for(obj = &stack->base[0]; obj < stack->top; obj++)
    {	
	anna_alloc_mark_entry(*obj);
    }
    if(stack->parent)
	anna_alloc_mark_vmstack(stack->parent);
    if(stack->function)
	anna_alloc_mark_function(stack->function);
}

void anna_alloc_mark(void *obj)
{
    switch(*((int *)obj) & ANNA_ALLOC_MASK)
    {
	case ANNA_OBJECT:
	{
	    anna_alloc_mark_object((anna_object_t *)obj);
	    break;
	}
	case ANNA_TYPE:
	{
	    anna_alloc_mark_type((anna_type_t *)obj);
	    break;
	}
	case ANNA_VMSTACK:
	{
	    anna_alloc_mark_vmstack((anna_vmstack_t *)obj);
	    break;
	}
	case ANNA_FUNCTION:
	{
	    anna_alloc_mark_function((anna_function_t *)obj);
	    break;
	}
	case ANNA_NODE:
	{
	    anna_alloc_mark_node((anna_node_t *)obj);
	    break;
	}
	case ANNA_STACK_TEMPLATE:
	{
	    anna_alloc_mark_stack_template((anna_stack_template_t *)obj);
	    break;
	}
	default:
	{
	    CRASH;
	}
    }
}

static void free_val(void *key, void *value)
{
    free(value);
}

static void anna_alloc_free(void *obj)
{
    
    switch(*((int *)obj) & ANNA_ALLOC_MASK)
    {
	case ANNA_OBJECT:
	{
	    if(obj == null_object)
		break;
	    anna_object_t *o = (anna_object_t *)obj;
	    if(anna_alloc_run_finalizers)
	    {
//		wprintf(L"AAA %ls\n", o->type->name);
		
		anna_member_t *del_mem = anna_member_get(o->type, ANNA_MID_DEL);
		if(del_mem && del_mem->is_method)
		{
		    anna_vm_run(
			anna_as_obj_fast(o->type->static_member[del_mem->offset]),
			1,
			&o);
		}
	    }
	    
	    anna_slab_free(obj, o->type->object_size);
	    break;
	}
	case ANNA_TYPE:
	{
	    int i;
	    anna_type_t *o = (anna_type_t *)obj;
	    //wprintf(L"Discarding unused type %ls\n", o->name);
	    
	    if(obj != null_type)
	    {
		for(i=0; i<anna_mid_max_get(); i++)
		{
		    anna_member_t *memb = o->mid_identifier[i];
		    if(!memb)
			continue;
		    free(memb);
		}
	    }
	    
	    free(o->member_blob);
	    if(o->static_member_count)
	    {
		free(o->static_member_blob);
		free(o->static_member);
	    }
	    
	    hash_destroy(&o->name_identifier);
	    free(o->mid_identifier);
	    
	    anna_slab_free(obj, sizeof(anna_type_t));
	    break;
	}
	case ANNA_VMSTACK:
	{
	    anna_vmstack_t *o = (anna_vmstack_t *)obj;
	    if(!o->function){
		free(o);
		return;
	    }
	    anna_slab_free(o, o->function->frame_size);
	    break;
	}
	case ANNA_FUNCTION:
	{
	    anna_function_t *o = (anna_function_t *)obj;
	    free(o->code);
	    free(o->input_type);
	    free(o->input_name);
	    
	    anna_slab_free(obj, sizeof(anna_function_t));
	    break;
	}
	case ANNA_NODE:
	{
	    anna_node_t *o = (anna_node_t *)obj;
	    switch(o->node_type)
	    {
		case ANNA_NODE_IDENTIFIER:
		{
//		    anna_node_identifier_t *n = (anna_node_identifier_t *)o;
		    break;
		}
		case ANNA_NODE_STRING_LITERAL:
		case ANNA_NODE_ASSIGN:
		{
//		    anna_node_assign_t *n = (anna_node_assign_t *)o;
		    break;
		}

		case ANNA_NODE_CONST:
		case ANNA_NODE_DECLARE:
		{
//		    anna_node_declare_t *n = (anna_node_declare_t *)o;
		    break;
		}
		
		case ANNA_NODE_CALL:
		case ANNA_NODE_CONSTRUCT:
		{
		    anna_node_call_t *n = (anna_node_call_t *)o;
		    free(n->child);
		    break;
		}
		case ANNA_NODE_MEMBER_CALL:
		{
		    anna_node_call_t *n = (anna_node_call_t *)o;
		    free(n->child);
		    break;
		}
	    }
	    free(obj);
	    break;
	}
	case ANNA_STACK_TEMPLATE:
	{
	    anna_stack_template_t *o = (anna_stack_template_t *)obj;
	    free(o->member_type);
	    free(o->member_declare_node);
	    free(o->member);
	    free(o->member_flags);
	    al_destroy(&o->import);
	    hash_foreach(&o->member_string_identifier, free_val);
	    hash_destroy(&o->member_string_identifier);
	    anna_slab_free(obj, sizeof(anna_stack_template_t));
	    break;
	}
	case ANNA_BLOB:
	{
//	    wprintf(L"DA BLOB\n");
	    
	    int *blob = (int *)obj;
	    size_t sz = blob[1];
	    anna_slab_free(obj, sz);
	    break;
	}
	default:
	{
	    break;
	}
    }
}

void anna_alloc_gc_block()
{
    anna_alloc_gc_block_counter++;
}

void anna_alloc_gc_unblock()
{
    anna_alloc_gc_block_counter--;
}

void anna_gc(anna_vmstack_t *stack)
{
    if(anna_alloc_gc_block_counter)
	return;

    anna_alloc_gc_block();
    size_t i;

#ifdef ANNA_CHECK_GC_LEAKS
    int s_count[]={0,0,0,0,0,0,0,0,0,0,0,0,0};
    for(i=0; i<al_get_count(&anna_alloc); i++)
    {
	void *obj = al_get_fast(&anna_alloc, i);
	s_count[(*((int *)obj) & ANNA_ALLOC_MASK)]++;
    }
    size_t start_count = al_get_count(&anna_alloc);
#endif

    
//    wprintf(L"\n\nRUNNING GC. We have %d allocated items, %d are objects, %d are stacks\n\n", al_get_count(&anna_alloc), oc, sc);

    anna_vmstack_t *stack_ptr = stack;
    while(stack_ptr)
    {
	anna_alloc_mark_vmstack(stack_ptr);	
	stack_ptr = stack_ptr->caller;
    }
    
//    anna_alloc_mark_stack_template(stack_global);
    int freed = 0;
    static int gc_first = 1;

    if(gc_first)
    {
	gc_first=0;
	
	for(i=0; i<al_get_count(&anna_alloc); i++)
	{
	    void *el = al_get_fast(&anna_alloc, i);
	    int flags = *((int *)el);
	    int alloc = (flags & ANNA_ALLOC_MASK);	
	    if(!(flags & ANNA_USED) && ((alloc == ANNA_OBJECT) || (alloc == ANNA_BLOB) || (alloc == ANNA_VMSTACK)))
	    {
		freed++;
		anna_alloc_free(el);
		al_set_fast(&anna_alloc, i, al_get_fast(&anna_alloc, al_get_count(&anna_alloc)-1));
		al_truncate(&anna_alloc, al_get_count(&anna_alloc)-1);
		i--;
	    }
	}
	
	for(i=0; i<al_get_count(&anna_alloc); i++)
	{
	    void *el = al_get_fast(&anna_alloc, i);
	    if(!(*((int *)el) & ANNA_USED))
	    {
		freed++;
		anna_alloc_free(el);
		al_set_fast(&anna_alloc, i, al_get_fast(&anna_alloc, al_get_count(&anna_alloc)-1));
		al_truncate(&anna_alloc, al_get_count(&anna_alloc)-1);
		i--;
	    }
	    else{
		anna_alloc_unmark(al_get_fast(&anna_alloc, i));
	    }
	}
    }
    else
    {
	for(i=0; i<al_get_count(&anna_alloc); i++)
	{
	    void *el = al_get_fast(&anna_alloc, i);
	    int flags = *((int *)el);
	    if(!(flags & ANNA_USED))
	    {
		freed++;
		anna_alloc_free(el);
		al_set_fast(&anna_alloc, i, al_get_fast(&anna_alloc, al_get_count(&anna_alloc)-1));
		al_truncate(&anna_alloc, al_get_count(&anna_alloc)-1);
		i--;
	    }
	    else
	    {
		anna_alloc_unmark(el);	    
	    }
	}
    }
    

    stack_ptr = stack;
    while(stack_ptr)
    {
	anna_alloc_unmark(stack_ptr);	    
	stack_ptr = stack_ptr->caller;
    }    


#ifdef ANNA_CHECK_GC_LEAKS
    size_t end_count = al_get_count(&anna_alloc);
    int o_count[]={0,0,0,0,0,0,0,0,0,0,0,0,0};
    for(i=0; i<al_get_count(&anna_alloc); i++)
    {
	void *obj = al_get_fast(&anna_alloc, i);
	o_count[(*((int *)obj) & ANNA_ALLOC_MASK)]++;
    }
    wchar_t *name[]=
	{
	    L"type", L"object", L"stack template", L"AST node", L"runtime stack", L"function", L"blob"
	}
    ;

    wprintf(L"Collected %d elements\n", start_count-end_count);
    for(i=0; i<7; i++)
    {
	wprintf(L"Collected %d elements of type %ls, after gc, %d elements remain\n", s_count[i] - o_count[i] , name[i], o_count[i]);
    }
    
#endif

    
//    wprintf(L"GC cycle performed, %d allocations freed, %d remain\n", freed, al_get_count(&anna_alloc));
    anna_alloc_gc_unblock();
//    wprintf(L"After GC. We have %d allocated objects\n", al_get_count(&anna_alloc));
}

#ifdef ANNA_FULL_GC_ON_SHUTDOWN
void anna_gc_destroy(void)
{
    anna_alloc_run_finalizers=0;
    anna_gc();
    al_destroy(&anna_alloc);
}
#endif
