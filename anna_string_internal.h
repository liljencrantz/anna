
struct anna_string_element;

struct anna_string
{
  size_t element_count;
  size_t element_capacity;
  struct anna_string_element **element;
  size_t *element_offset;
  size_t *element_length;
}
  ;

typedef struct anna_string anna_string_t;


/**
   Init the string to an empty value
 */
void anna_string_init(anna_string_t *string);
/**
   Init the string to contain the specified C string
 */
void anna_string_init_from_ptr(anna_string_t *string, wchar_t *payload, size_t size);
/*
  Destroy the specified string, free:ing all memory used by it.
 */
void anna_string_destroy(anna_string_t *string);

/**
   Append the specified substring to the end of the specified string
 */
void anna_string_append(anna_string_t *dest, anna_string_t *src, size_t offset, size_t length);

/**
   Copies the specified substring into the specified string
 */
void anna_string_substring(anna_string_t *dest, anna_string_t *src, size_t offset, size_t length);

/**
   Sets the character at the specified offset of the string
 */
void anna_string_set_char(anna_string_t *dest, size_t offset, wchar_t ch);
/**
   Returns the character at the specified offset of the string
 */
wchar_t anna_string_get_char(anna_string_t *dest, size_t offset);

/**
   Replace the pecified substring in source with the specified substring of dest
 */
void anna_string_replace(anna_string_t *dest, anna_string_t *src, size_t dest_offset, size_t dest_length, size_t src_offset, size_t src_length);
/**
   Return the length of the string
 */
size_t anna_string_get_length(anna_string_t *dest);

void anna_string_truncate(anna_string_t *dest, size_t length);







