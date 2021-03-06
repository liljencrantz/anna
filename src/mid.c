//ROOT: src/type.c

static hash_table_t anna_mid_identifier;
static array_list_t anna_mid_identifier_reverse;
static mid_t mid_pos = ANNA_MID_FIRST_UNRESERVED;

static void anna_mid_put(wchar_t *name, mid_t mid);

static void anna_mid_free(void *key, void *val)
{
    free(key);
    free(val);
}

void anna_mid_destroy(void)
{
    al_destroy(&anna_mid_identifier_reverse);
    hash_foreach(&anna_mid_identifier, anna_mid_free);
    hash_destroy(&anna_mid_identifier);
}

void anna_mid_init()
{
    al_init(&anna_mid_identifier_reverse);
    hash_init(
	&anna_mid_identifier,
	&hash_wcs_func, 
	&hash_wcs_cmp);
    wchar_t *mid_name[] = 
	{
	    L"!intPayload",
	    L"!stringPayload",
	    L"!stringPayloadSize",
	    L"!charPayload",
	    L"!listPayload",
	    L"!listSize",
	    L"!listCapacity",
	    L"!listSpecialization",
	    L"!functionPayload",
	    L"!functionStack",

	    L"!functionTypePayload",
	    L"!typeWrapperPayload",
	    L"__init__",
	    L"!nodePayload",
	    L"!memberPayload",
	    L"!memberTypePayload",
	    L"!stackPayload",
	    L"!stackTypePayload",
	    L"step",
	    L"!floatPayload",

	    L"!rangeFrom",
	    L"!rangeTo",
	    L"!rangeStep",
	    L"!rangeOpen",
	    L"!complexPayload",
	    L"hashCode",
	    L"__cmp__",
	    L"!hashPayload",
	    L"!hashSpecialization1",
	    L"!hashSpecialization2",

	    L"toString",
	    L"!pairSpecialization1",
	    L"!pairSpecialization2",
	    L"!pairFirst",
	    L"!pairSecond",
	    L"!continuationStack",
	    L"!continuationCodePos",
	    L"!this",
	    L"!method",
	    L"__add__",

	    L"__sub__",	    
	    L"__mul__",
	    L"__div__",
	    L"__increaseAssign__",
	    L"__decreaseAssign__",
	    L"__nextAssign__",
	    L"__prevAssign__",
	    L"bitand",
	    L"bitor",
	    L"bitxor",

	    L"__add__Float__",
	    L"__sub__Float__",
	    L"__mul__Float__",	    
	    L"__div__Float__",
	    L"expFloat__",
	    L"__increaseAssign__Float__",
	    L"__decreaseAssign__Float__",
	    L"!bufferPayload",
	    L"!bufferSize",
	    L"!bufferCapacity",

	    L"!cStructPayload",
	    L"!continuationCallCount",
	    L"!continuationStackCount",
	    L"!continuationActivationFrame",
	    L"name",
	    L"outputType",
	    L"inputType",
	    L"inputName",
	    L"defaultValue",
	    L"attribute",

	    L"filename",
	    L"continuation",
	    L"!channelRead",
	    L"!channelWrite",
	    L"!channelSync",
	    L"Iterator",
	    L"iterator",
	    L"collection",
	    L"key",
	    L"value",

	    L"empty?",
	    L"offset",
	    L"versionId",
	    L"count",
	    L"valid?",
	    L"__appendAssign__",
	    L"get",
	    L"getRange",
	    L"set",
	    L"setRange",

	    L"__join__",
	    L"reverse",
	    L"push",
	    L"pop",
	    L"clear",
	    L"__in__",
	    L"first",
	    L"last",
	    L"freeze",
	    L"thaw",

	    L"__get__",
	    L"__set__",
	    L"!comparator",
	    L"__neg__",
	    L"abs",
	    L"sign",
	};
    
    int i;
    assert(
	(sizeof(mid_name)/sizeof(wchar_t *)) == ANNA_MID_FIRST_UNRESERVED);    
    
    for(i=0; i<(sizeof(mid_name)/sizeof(wchar_t *)); i++)
    {
	anna_mid_put(anna_intern_static(mid_name[i]), i);
    }    
}

static void anna_mid_put(wchar_t *name, mid_t mid)
{
    size_t *offset_ptr = hash_get(&anna_mid_identifier, name);
    if(offset_ptr)
    {
	anna_message(L"Tried to reassign mid!\n");
	exit(1);
    }
    name = anna_intern(name);
    offset_ptr = malloc(sizeof(size_t));
    *offset_ptr = mid;
    hash_put(&anna_mid_identifier, name, offset_ptr);   
    al_set(&anna_mid_identifier_reverse, mid, name);
}

size_t anna_mid_get(wchar_t *name)
{
    size_t *offset_ptr = hash_get(&anna_mid_identifier, name);
    if(!offset_ptr)
    {      

	size_t gg = mid_pos++;
	anna_mid_put(name, gg);
	return gg;
    }
    return *offset_ptr;
}

wchar_t *anna_mid_get_reverse(mid_t mid)
{
    return (wchar_t *)al_get(&anna_mid_identifier_reverse, mid);
}

