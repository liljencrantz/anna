
struct duck_string_element;

struct duck_string
{
  size_t element_count;
  size_t element_capacity;
  struct duck_string_element **element;
  size_t *element_offset;
  size_t *element_length;
}
  ;

void duck_string_append(duck_string_t *dest, duck_string_t *src, size_t offset, size_t length);
void duck_string_init(duck_string_t *string);
void duck_string_init_from_string(duck_string_t *string, wchar_t *payload, size_t size);
void duck_string_substring(duck_string_t *dest, duck_string_t *src, size_t offset, size_t length);
void duck_string_append(duck_string_t *dest, duck_string_t *src, size_t offset, size_t length);
void duck_string_set_char(duck_string_t *dest, size_t offset, wchar_t ch);
wchar_t duck_string_get_char(duck_string_t *dest, size_t offset);
size_t duck_string_get_length(duck_string_t *dest);
void duck_string_print(duck_string_t *dest);






