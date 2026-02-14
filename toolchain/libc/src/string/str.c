/*
 * linx-libc: String functions
 */

#include <linxisa_libc.h>

size_t strlen(const char *s) {
    const char *p = s;
    while (*p) p++;
    return (size_t)(p - s);
}

char *strcpy(char *dest, const char *src) {
    char *d = dest;
    while ((*d++ = *src++));
    return dest;
}

char *strncpy(char *dest, const char *src, size_t n) {
    char *d = dest;
    size_t i = 0;
    while (i < n && src[i]) {
        d[i] = src[i];
        i++;
    }
    while (i < n) {
        d[i++] = '\0';
    }
    return dest;
}

int strcmp(const char *s1, const char *s2) {
    while (*s1 && *s2 && *s1 == *s2) {
        s1++;
        s2++;
    }
    return (int)(unsigned char)*s1 - (int)(unsigned char)*s2;
}

int strncmp(const char *s1, const char *s2, size_t n) {
    while (n-- && *s1 && *s2 && *s1 == *s2) {
        s1++;
        s2++;
    }
    if (n == (size_t)-1) return 0;
    return (int)(unsigned char)*s1 - (int)(unsigned char)*s2;
}

char *strchr(const char *s, int c) {
    while (*s) {
        if (*s == (char)c) {
            return (char *)s;
        }
        s++;
    }
    return NULL;
}

char *strrchr(const char *s, int c) {
    char *last = NULL;
    while (*s) {
        if (*s == (char)c) {
            last = (char *)s;
        }
        s++;
    }
    return last;
}

char *strstr(const char *haystack, const char *needle) {
    if (!*needle) return (char *)haystack;
    
    const char *h = haystack;
    while (*h) {
        const char *n = needle;
        const char *p = h;
        
        while (*n && *p && *n == *p) {
            n++;
            p++;
        }
        
        if (!*n) return (char *)h;
        h++;
    }
    
    return NULL;
}
