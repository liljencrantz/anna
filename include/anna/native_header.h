#include <string.h>
#include "anna/anna.h"
#include "anna/vm.h"

static inline anna_activation_frame_t *anna_frame_get_static(anna_context_t *context, size_t sz)
{
    anna_activation_frame_t *res = (anna_activation_frame_t *)
	context->static_frame_ptr;
    context->static_frame_ptr += sz; 
    res->flags = ANNA_ACTIVATION_FRAME | ANNA_ACTIVATION_FRAME_STATIC;
    return res;
}

static inline void anna_frame_push(anna_context_t *context) 
{
    anna_object_t *wfun = context->function_object;
    size_t stack_offset = wfun->type->mid_identifier[ANNA_MID_FUNCTION_WRAPPER_STACK]->offset;
    anna_activation_frame_t *static_frame = *(anna_activation_frame_t **)&wfun->member[stack_offset];
    anna_function_t *fun = anna_function_unwrap_fast(wfun);
    anna_activation_frame_t *res = anna_frame_get_static(context, fun->frame_size);
    
    res->static_frame = static_frame;
    res->dynamic_frame = context->frame;
    res->function = fun;
    res->code = fun->code;
    context->top -= (fun->input_count+1);
    res->return_stack_top = context->top;
    /* Copy over input parameter values */
    memcpy(&res->slot[0], context->top+1,
	   sizeof(anna_object_t *)*fun->input_count);
    /* Set initial value of all variables to null */
    int i;
    for(i=fun->input_count; i<fun->variable_count;i++)
	res->slot[i] = null_entry;
    context->frame = res;
}

static inline void anna_frame_return(anna_context_t *context, anna_activation_frame_t *frame)
{
    if(frame->flags & ANNA_ACTIVATION_FRAME_STATIC)
    {
	context->static_frame_ptr -= frame->function->frame_size;
//	assert(stack == anna_context_static_ptr);
    }
}

static inline void anna_context_frame_return(anna_context_t *context)
{
    context->top = context->frame->return_stack_top;
    anna_frame_return(context, context->frame);
    context->frame = context->frame->dynamic_frame;
}

static inline void anna_context_frame_return_static(anna_context_t *context)
{
    anna_frame_return(context, context->frame);
    context->frame = context->frame->static_frame;
}

