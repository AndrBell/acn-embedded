/* Copyright (c) 2017 Arrow Electronics, Inc.
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Apache License 2.0
 * which accompanies this distribution, and is available at
 * http://apache.org/licenses/LICENSE-2.0
 * Contributors: Arrow Electronics, Inc.
 */

#include <qcom_common.h>
#include <qcom_misc.h>
#include <sys/mem.h>

char *strndup (const char *__string, size_t __n) {
  char *p = malloc(__n + 1);
  strncpy(p, __string, __n);
  p[__n] = 0x0;
  return p;
}

char *strcat(char *dest, const char *src) {
    char* tail = dest;
    while(*tail) {
        tail++;
    }
    strcpy(tail, src);
    return dest;
}

#if 0
char *strncpy(char *dst, const char *src, size_t n) {
  char *temp = dst;
  while (n-- && (*dst++ = *src++))
    ;
  return temp;
}
#endif

char *strncat(char *dest, const char *src, size_t n) {
    char *ret = dest;
    while (*dest++)
      ;
    dest--;
    while (n-- && (*dest++ = *src++))
      ;
    return ret;
}

double strtod (const char* str, char** endptr) {
    double value;
    SSP_PARAMETER_NOT_USED(endptr);
    int i = sscanf( str, "%lf", &value );
    if ( i != 1 ) return 0;
    return value;
}

static void swap(void *x, void *y, size_t l) {
   char *a = x, *b = y, c;
   while(l--) {
      c = *a;
      *a++ = *b;
      *b++ = c;
   }
}

static void sort(char *array, size_t size, __compar_fn_t cmp, int begin, int end) {
  if (end > begin) {
    void *pivot = array + begin;
    int l = begin + size;
    int r = end;
    while(l < r) {
      if (cmp(array+l,pivot) <= 0) {
        l += size;
      } else if ( cmp(array+r, pivot) > 0 )  {
        r -= size;
      } else if ( l < r ) {
        swap(array+l, array+r, size);
      }
    }
    l -= size;
    swap(array+begin, array+l, size);
    sort(array, size, cmp, begin, l);
    sort(array, size, cmp, r, end);
  }
}

void qsort(void *__base, size_t __nmemb, size_t __size, __compar_fn_t __compar) {
  sort((char*)__base, __size, __compar, 0, (__nmemb-1) * __size);
}

int vsnprintf(char *s, size_t n, const char *format, va_list ap) {
    return qcom_vsnprintf(s,  n, format, ap);
}
