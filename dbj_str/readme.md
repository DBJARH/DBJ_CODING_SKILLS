
> **Caveat Emptor**: We are enjoying the metapresence of [DBJ Taxonomies](https://method.dbj.org/taxonomy_core.html). Thus we know where are we in the information space with these endeavor. Top category: **Implementation**. Capability: **Development**. In other word: We know what is this all about. And we can explain it to the reader.
>

# dbj_str

> Core principles for this repo are defined once, in
> [CLAUDE.md](../CLAUDE.md#core-principles) — see that file, not here.

---

## Technical Definitions

**String Type:**

```c
#define dbj_str( sz ) struct { unsigned char data[sz]; }
```

### Basic Structure Definition

```c
#include <stddef.h>

#define dbj_str( sz ) struct { unsigned char data[sz]; }
typedef dbj_str(256) str256;

// Factory method for initialization
str256 create_str256(const char* input) {
    str256 s = {0};
    size_t i = 0;
    while (input[i] != '\0' && i < 255) {
        s.data[i] = (unsigned char)input[i];
        i++;
    }
    s.data[i] = '\0';
    return s;
}
```

## C23 Static Array Parameters

Using `static` in array parameters enforces non-null constraints and minimum size at the interface level, acting as a compiler-verified contract.

```c
#include <string.h>

str256 create_str256_from_buffer(const unsigned char src[static 256]) {
    str256 s;
    // Safe due to static 256 constraint
    memcpy(s.data, src, 256);
    return s;
}
```

## Generic Macro for Data Types with CRUD

To maintain your principles while reducing boilerplate, a macro generates the data structures and functional CRUD operations.

```c
#include <string.h>

#define DEFINE_DBJ_STR_TYPE(name, sz) \
    typedef struct { unsigned char data[sz]; } name; \
    \
    /* Create: Factory method */ \
    static inline name name##_create(const unsigned char src[static sz]) { \
        name s; \
        memcpy(s.data, src, sz); \
        return s; \
    } \
    \
    /* Read: Direct access */ \
    static inline void name##_read(const name* s, unsigned char dest[static sz]) { \
        memcpy(dest, s->data, sz); \
    } \
    \
    /* Update: Functional modification */ \
    static inline name name##_update(name s, const unsigned char src[static sz]) { \
        memcpy(s.data, src, sz); \
        return s; \
    } \
    \
    /* Delete: Reset to default */ \
    static inline name name##_delete(name s) { \
        memset(s.data, 0, sz); \
        return s; \
    }

// Usage Example
DEFINE_DBJ_STR_TYPE(str256, 256)

int main(void) {
    unsigned char init_data[256] = "Initial Data";
    str256 my_str = str256_create(init_data);

    unsigned char new_data[256] = "Updated Data";
    my_str = str256_update(my_str, new_data);

    my_str = str256_delete(my_str);

    return 0;
}
```
