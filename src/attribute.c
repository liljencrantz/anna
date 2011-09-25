#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <wchar.h>
#include <assert.h>
#include <string.h>

#include "anna/common.h"
#include "anna/util.h"
#include "anna/base.h"
#include "anna/attribute.h"
#include "anna/node.h"

static int anna_attribute_has_alias_internal(
    anna_node_call_t *attribute, wchar_t *attr_name, wchar_t *name)
{
    if(!attribute)
	return 0;
    int i;
    
    for(i=0; i<attribute->child_count; i++)
    {
	if(anna_node_is_call_to(attribute->child[i], attr_name))
	{
	    anna_node_call_t *attr = node_cast_call(attribute->child[i]);
	    assert(attr->child_count == 1);
	    return anna_node_is_named(attr->child[0], name);
	}
    }
    return 0;    
}

int anna_attribute_has_alias_reverse(anna_node_call_t *attribute, wchar_t *name)
{
    return anna_attribute_has_alias_internal(attribute, L"aliasReverse", name);
}

int anna_attribute_has_alias(anna_node_call_t *attribute, wchar_t *name)
{
    return anna_attribute_has_alias_internal(attribute, L"alias", name);
}

int anna_attribute_flag(anna_node_call_t *attribute, wchar_t *name)
{
    if(!attribute)
	return 0;
    int i;
    for(i=0; i<attribute->child_count; i++)
    {
	
	if(anna_node_is_named(attribute->child[i], name))
	{
	    return 1;
	}
    }
    return 0;
}

anna_node_t *anna_attribute_call(anna_node_call_t *attribute, wchar_t *name)
{
    array_list_t al = AL_STATIC;
    anna_attribute_call_all(attribute, name, &al);
    anna_node_t *res = 0;
    if(al_get_count(&al))
    {
	res = (anna_node_t *)al_get(&al, 0);
	al_destroy(&al);
    }
    return res;
}


void anna_attribute_call_all(anna_node_call_t *attribute, wchar_t *name, array_list_t *res)
{
    if(!attribute)
	return;
    int i;
    for(i=0; i<attribute->child_count; i++)
    {
	
	if(anna_node_is_call_to(attribute->child[i], name))
	{
	    anna_node_call_t *attr = node_cast_call(attribute->child[i]);
	    int j;
	    for(j=0; j<attr->child_count; j++)
	    {
		al_push(res, attr->child[j]);
	    }
	}
    }
    return;
}

