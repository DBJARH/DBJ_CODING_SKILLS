/*

DBJ String Type Definition
```c
#define dbjstr( sz ) struct { unsigned char data[sz]; }
typedef dbjstr(256) str256;

```

Generic CRUD Template

This implementation adheres to the principle of generating standard operations through macros,
ensuring type safety via `static` array constraints and maintaining a functional (non-OOP) data flow.

*/
#include <string.h>

#define DEFINE_DBJSTR_TYPE(name, sz)                                             \
    typedef struct                                                               \
    {                                                                            \
        unsigned char data[sz];                                                  \
    } name;                                                                      \
                                                                                 \
    /* Create: Factory method with static constraint */                          \
    static inline name name##_create(const unsigned char src[static sz])         \
    {                                                                            \
        name s;                                                                  \
        memcpy(s.data, src, sz);                                                 \
        return s;                                                                \
    }                                                                            \
                                                                                 \
    /* Read: Direct access, data returned by value */                            \
    static inline void name##_read(const name *s, unsigned char dest[static sz]) \
    {                                                                            \
        memcpy(dest, s->data, sz);                                               \
    }                                                                            \
                                                                                 \
    /* Update: Functional modification, returning new data */                    \
    static inline name name##_update(name s, const unsigned char src[static sz]) \
    {                                                                            \
        memcpy(s.data, src, sz);                                                 \
        return s;                                                                \
    }                                                                            \
                                                                                 \
    /* Delete: Clear the data (Reset to default) */                              \
    static inline name name##_delete(name s)                                     \
    {                                                                            \
        memset(s.data, 0, sz);                                                   \
        return s;                                                                \
    }

/*
// Usage Example
DEFINE_DBJSTR_TYPE(str256, 256)

int main(void) {
unsigned char init_data[256] = "Initial Data";
str256 my_str = str256_create(init_data);

// Update operation
unsigned char new_data[256] = "Updated Data";
my_str = str256_update(my_str, new_data);

// Delete operation
my_str = str256_delete(my_str);

return 0;
}

*/