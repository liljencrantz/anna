
/*
    DO NOT MANUALLY EDIT THIS FILE.

    This file has been automaticaly generated by the anna bind
    utility. If you manually edit it, your changes will eventually be
    lost. Not to mention the fact that staring at machine generated
    code rots your brain. If this file is incorrect, either update the
    bind utility or update the binding source, which is located in the
    file:

    bindings/getText.bind

 */

#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <wchar.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>

#include <libintl.h>

#include "anna/anna.h"

// Declare internal variables for all types defined in this module


// Data used to initialize all types defined in this module
const static anna_type_data_t anna_getText_type_data[] = 
{
};

// This is the source code of the various wrapper functions

ANNA_VM_NATIVE(getText_i__get_text, 1)
{
    // Validate parameters
    if(param[0] == null_entry){return null_entry;}

    // Mangle input parameters
    char *native_param_msgid = anna_string_payload_narrow(anna_as_obj(param[0]));

    // Validate parameters
    

    // Call the function
    anna_entry_t *result = (gettext(native_param_msgid)) ? anna_from_obj(anna_string_create_narrow(strlen(gettext(native_param_msgid)), gettext(native_param_msgid))) : null_entry;
    // Perform cleanup
    free(native_param_msgid);

    // Return result
    return result;
}

ANNA_VM_NATIVE(getText_i__d_get_text, 2)
{
    // Validate parameters
    if(param[0] == null_entry){return null_entry;}
    if(param[1] == null_entry){return null_entry;}

    // Mangle input parameters
    char *native_param_domain = anna_string_payload_narrow(anna_as_obj(param[0]));
    char *native_param_msgid = anna_string_payload_narrow(anna_as_obj(param[1]));

    // Validate parameters
    
    

    // Call the function
    anna_entry_t *result = (dgettext(native_param_domain, native_param_msgid)) ? anna_from_obj(anna_string_create_narrow(strlen(dgettext(native_param_domain, native_param_msgid)), dgettext(native_param_domain, native_param_msgid))) : null_entry;
    // Perform cleanup
    free(native_param_domain);
    free(native_param_msgid);

    // Return result
    return result;
}

ANNA_VM_NATIVE(getText_i__d_c_get_text, 3)
{
    // Validate parameters
    if(param[0] == null_entry){return null_entry;}
    if(param[1] == null_entry){return null_entry;}
    if(param[2] == null_entry){return null_entry;}

    // Mangle input parameters
    char *native_param_domain = anna_string_payload_narrow(anna_as_obj(param[0]));
    char *native_param_msgid = anna_string_payload_narrow(anna_as_obj(param[1]));
    int native_param_category = anna_as_int(param[2]);

    // Validate parameters
    
    
    

    // Call the function
    anna_entry_t *result = (dcgettext(native_param_domain, native_param_msgid, native_param_category)) ? anna_from_obj(anna_string_create_narrow(strlen(dcgettext(native_param_domain, native_param_msgid, native_param_category)), dcgettext(native_param_domain, native_param_msgid, native_param_category))) : null_entry;
    // Perform cleanup
    free(native_param_domain);
    free(native_param_msgid);

    // Return result
    return result;
}

ANNA_VM_NATIVE(getText_i__n_get_text, 3)
{
    // Validate parameters
    if(param[0] == null_entry){return null_entry;}
    if(param[1] == null_entry){return null_entry;}
    if(param[2] == null_entry){return null_entry;}

    // Mangle input parameters
    char *native_param_msgid = anna_string_payload_narrow(anna_as_obj(param[0]));
    char *native_param_msgidPlural = anna_string_payload_narrow(anna_as_obj(param[1]));
    int native_param_count = anna_as_int(param[2]);

    // Validate parameters
    
    
    

    // Call the function
    anna_entry_t *result = (ngettext(native_param_msgid, native_param_msgidPlural, native_param_count)) ? anna_from_obj(anna_string_create_narrow(strlen(ngettext(native_param_msgid, native_param_msgidPlural, native_param_count)), ngettext(native_param_msgid, native_param_msgidPlural, native_param_count))) : null_entry;
    // Perform cleanup
    free(native_param_msgid);
    free(native_param_msgidPlural);

    // Return result
    return result;
}

ANNA_VM_NATIVE(getText_i__d_n_get_text, 4)
{
    // Validate parameters
    if(param[0] == null_entry){return null_entry;}
    if(param[1] == null_entry){return null_entry;}
    if(param[2] == null_entry){return null_entry;}
    if(param[3] == null_entry){return null_entry;}

    // Mangle input parameters
    char *native_param_domain = anna_string_payload_narrow(anna_as_obj(param[0]));
    char *native_param_msgid = anna_string_payload_narrow(anna_as_obj(param[1]));
    char *native_param_msgidPlural = anna_string_payload_narrow(anna_as_obj(param[2]));
    int native_param_count = anna_as_int(param[3]);

    // Validate parameters
    
    
    
    

    // Call the function
    anna_entry_t *result = (dngettext(native_param_domain, native_param_msgid, native_param_msgidPlural, native_param_count)) ? anna_from_obj(anna_string_create_narrow(strlen(dngettext(native_param_domain, native_param_msgid, native_param_msgidPlural, native_param_count)), dngettext(native_param_domain, native_param_msgid, native_param_msgidPlural, native_param_count))) : null_entry;
    // Perform cleanup
    free(native_param_domain);
    free(native_param_msgid);
    free(native_param_msgidPlural);

    // Return result
    return result;
}

ANNA_VM_NATIVE(getText_i__d_c_n_get_text, 5)
{
    // Validate parameters
    if(param[0] == null_entry){return null_entry;}
    if(param[1] == null_entry){return null_entry;}
    if(param[2] == null_entry){return null_entry;}
    if(param[3] == null_entry){return null_entry;}
    if(param[4] == null_entry){return null_entry;}

    // Mangle input parameters
    char *native_param_domain = anna_string_payload_narrow(anna_as_obj(param[0]));
    char *native_param_msgid = anna_string_payload_narrow(anna_as_obj(param[1]));
    char *native_param_msgidPlural = anna_string_payload_narrow(anna_as_obj(param[2]));
    int native_param_count = anna_as_int(param[3]);
    int native_param_category = anna_as_int(param[4]);

    // Validate parameters
    
    
    
    
    

    // Call the function
    anna_entry_t *result = (dcngettext(native_param_domain, native_param_msgid, native_param_msgidPlural, native_param_count, native_param_category)) ? anna_from_obj(anna_string_create_narrow(strlen(dcngettext(native_param_domain, native_param_msgid, native_param_msgidPlural, native_param_count, native_param_category)), dcngettext(native_param_domain, native_param_msgid, native_param_msgidPlural, native_param_count, native_param_category))) : null_entry;
    // Perform cleanup
    free(native_param_domain);
    free(native_param_msgid);
    free(native_param_msgidPlural);

    // Return result
    return result;
}

ANNA_VM_NATIVE(getText_i__text_domain, 1)
{
    // Validate parameters
    if(param[0] == null_entry){return null_entry;}

    // Mangle input parameters
    char *native_param_domain = anna_string_payload_narrow(anna_as_obj(param[0]));

    // Validate parameters
    

    // Call the function
    anna_entry_t *result = (textdomain(native_param_domain)) ? anna_from_obj(anna_string_create_narrow(strlen(textdomain(native_param_domain)), textdomain(native_param_domain))) : null_entry;
    // Perform cleanup
    free(native_param_domain);

    // Return result
    return result;
}

ANNA_VM_NATIVE(getText_i__bind_text_domain, 2)
{
    // Validate parameters
    if(param[0] == null_entry){return null_entry;}
    if(param[1] == null_entry){return null_entry;}

    // Mangle input parameters
    char *native_param_domain = anna_string_payload_narrow(anna_as_obj(param[0]));
    char *native_param_dir = anna_string_payload_narrow(anna_as_obj(param[1]));

    // Validate parameters
    
    

    // Call the function
    anna_entry_t *result = (bindtextdomain(native_param_domain, native_param_dir)) ? anna_from_obj(anna_string_create_narrow(strlen(bindtextdomain(native_param_domain, native_param_dir)), bindtextdomain(native_param_domain, native_param_dir))) : null_entry;
    // Perform cleanup
    free(native_param_domain);
    free(native_param_dir);

    // Return result
    return result;
}


// This function is called to create all types defined in this module

void anna_getText_create(anna_stack_template_t *stack);
void anna_getText_create(anna_stack_template_t *stack)
{
    anna_type_data_create(anna_getText_type_data, stack);        
}

// This function is called to load all functions and other declarations into the module

void anna_getText_load(anna_stack_template_t *stack);
void anna_getText_load(anna_stack_template_t *stack)
{
    anna_type_t *stack_type = anna_stack_wrap(stack)->type;
    anna_module_data_t modules[] =
        {
        };
    anna_module_data_create(modules, stack);

    wchar_t *this_argn[] = {L"this"};


    anna_type_t *getText_i__get_text_argv[] = {string_type};
    wchar_t *getText_i__get_text_argn[] = {L"msgid"};
    anna_module_function(stack, L"getText", 0, &getText_i__get_text, string_type, 1, getText_i__get_text_argv, getText_i__get_text_argn, 0, L"Translate message");

    anna_type_t *getText_i__d_get_text_argv[] = {string_type, string_type};
    wchar_t *getText_i__d_get_text_argn[] = {L"domain", L"msgid"};
    anna_module_function(stack, L"dGetText", 0, &getText_i__d_get_text, string_type, 2, getText_i__d_get_text_argv, getText_i__d_get_text_argn, 0, L"Translate message");

    anna_type_t *getText_i__d_c_get_text_argv[] = {string_type, string_type, int_type};
    wchar_t *getText_i__d_c_get_text_argn[] = {L"domain", L"msgid", L"category"};
    anna_module_function(stack, L"dCGetText", 0, &getText_i__d_c_get_text, string_type, 3, getText_i__d_c_get_text_argv, getText_i__d_c_get_text_argn, 0, L"Translate message");

    anna_type_t *getText_i__n_get_text_argv[] = {string_type, string_type, int_type};
    wchar_t *getText_i__n_get_text_argn[] = {L"msgid", L"msgidPlural", L"count"};
    anna_module_function(stack, L"nGetText", 0, &getText_i__n_get_text, string_type, 3, getText_i__n_get_text_argv, getText_i__n_get_text_argn, 0, L"Translate message");

    anna_type_t *getText_i__d_n_get_text_argv[] = {string_type, string_type, string_type, int_type};
    wchar_t *getText_i__d_n_get_text_argn[] = {L"domain", L"msgid", L"msgidPlural", L"count"};
    anna_module_function(stack, L"dNGetText", 0, &getText_i__d_n_get_text, string_type, 4, getText_i__d_n_get_text_argv, getText_i__d_n_get_text_argn, 0, L"Translate message");

    anna_type_t *getText_i__d_c_n_get_text_argv[] = {string_type, string_type, string_type, int_type, int_type};
    wchar_t *getText_i__d_c_n_get_text_argn[] = {L"domain", L"msgid", L"msgidPlural", L"count", L"category"};
    anna_module_function(stack, L"dCNGetText", 0, &getText_i__d_c_n_get_text, string_type, 5, getText_i__d_c_n_get_text_argv, getText_i__d_c_n_get_text_argn, 0, L"Translate message");

    anna_type_t *getText_i__text_domain_argv[] = {string_type};
    wchar_t *getText_i__text_domain_argn[] = {L"domain"};
    anna_module_function(stack, L"textDomain", 0, &getText_i__text_domain, string_type, 1, getText_i__text_domain_argv, getText_i__text_domain_argn, 0, L"Set domain for future getText calls");

    anna_type_t *getText_i__bind_text_domain_argv[] = {string_type, string_type};
    wchar_t *getText_i__bind_text_domain_argn[] = {L"domain", L"dir"};
    anna_module_function(stack, L"bindTextDomain", 0, &getText_i__bind_text_domain, string_type, 2, getText_i__bind_text_domain_argv, getText_i__bind_text_domain_argn, 0, L"Set directory containing message catalogs");
    anna_stack_document(stack, L"The Anna getText module is a low level wrapper around the C gettext library. For a higher level solution, see the <a href='i18n.html'>i18n module</a>. For full documentation on this library, please see e.g. <a href='www.gnu.org/software/gettext/manual/gettext.html'>the GNU gettext documentation</a>.");

     anna_type_data_register(anna_getText_type_data, stack);
}
