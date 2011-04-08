#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <wchar.h>
#include <assert.h>
#include <string.h>

#include "common.h"
#include "util.h"
#include "anna.h"
#include "anna_attribute.h"
#include "anna_node.h"

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

wchar_t *anna_attribute_identifier(anna_node_call_t *attribute, wchar_t *name)
{
    anna_node_t *res = anna_attribute_node(attribute, name);
    if(res->node_type == ANNA_NODE_IDENTIFIER)
    {
	anna_node_identifier_t *id = (anna_node_identifier_t *)res;
	return id->name;
    }
    return 0;
}

anna_node_t *anna_attribute_node(anna_node_call_t *attribute, wchar_t *name)
{
    array_list_t al = AL_STATIC;
    anna_attribute_node_all(attribute, name, &al);
    anna_node_t *res = 0;
    if(al_get_count(&al))
    {
	res = (anna_node_t *)al_get(&al, 0);
	al_destroy(&al);
    }
    return res;
}


void anna_attribute_node_all(anna_node_call_t *attribute, wchar_t *name, array_list_t *res)
{
    if(!attribute)
	return;
    int i;
    for(i=0; i<attribute->child_count; i++)
    {
	
	if(anna_node_is_call_to(attribute->child[i], name))
	{
	    anna_node_call_t *attr = node_cast_call(attribute->child[i]);
	    assert(attr->child_count == 1);
	    al_push(res, attr->child[0]);
	}
    }
    return;
}

