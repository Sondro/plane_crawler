/*
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
              SINGLE-HEADER C/++ UTILITY DEFINITIONS
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

    DESCRIPTION

        This is a single-header library that defines a set of
        useful typedefs/macros for the programmer. This is
        effectively just stuff that I have used to make
        programming easier/more fun, but it can be quite
        convenient so I decided to package it together for
        others. It isn't necessarily cumbersome for one to do
        themselves, but it's nice to have it in a single file
        just to throw into a project.

        It is dependent on the CRT.

    UTILITIES

      * fixed-size type typedefs
            Some aliases are provided for fixed-length integer
            and floating-point types that are shorter and
            easier to type. Here's a list of them:

            i8  for int8_t
            i16 for int16_t
            i32 for int32_t
            i64 for int64_t
            u8  for uint8_t
            u16 for uint16_t
            u32 for uint32_t
            u64 for uint64_t
            r32 for float
            r64 for double
            bl  for int8_t (used for booleans)

      * malloc/alloca helpers
            I found it particularly annoying to type:

                int *arr = (int *)malloc(100*sizeof(int));

            time after time. Typing the type that you want to
            allocate three times is a bit cumbersome, so this
            file contains some macros that are helpful here.
            They're also more readable.

            These macros are heap_alloc and stack_alloc. They
            both only accept two parameters, the type that is
            being allocated, and the number of objects of that
            type to allocate.

            Here's an example:

                int *heap_arr = heap_alloc(int, 100000);
                int *stack_arr = stack_alloc(int, 100);

                free(heap_arr);
                // don't free stack_arr, it's on the stack

      * byte/kilobyte/megabyte/etc. conversion helpers
            A few macros are provided for converting from
            kilobytes/megabytes/etc. into bytes. For example,
            kilobytes(1) will return 1024 bytes, and so on.

      * foreach loops
            foreach loops are a quicker method of looping from
            0 to some upper-bound. They're usually useful for
            looping through the indices of an array. The
            foreach macros take two arguments: the identifier
            to be used to define the iterator, and the upper-limit
            of the loop (during the last iteration of the loop,
            the iterator will be the upper-limit minus 1).

            Here's an example:

                int arr[1000];
                foreach(i, 1000) {
                    arr[i] = i;
                }

                // is equivalent to:

                int arr[1000];
                for(unsigned int i = 0; i < 1000; ++i) {
                    arr[i] = i;
                }

            foreach uses a 64-bit unsigned integer by default.
            There are other foreach macros as well, however,
            that can allow looping with iterators of different
            lengths. For example, foreach8 loops with an
            unsigned byte, foreach16 loops with a 16-bit
            unsigned integer, etc.

      * forrng loops
            forrng ("for range") loops are similar to foreach
            loops, except they are iterated with a signed integer
            and allow iterating through negative values. They
            take 3 parameters instead of 2: the identifier to
            be used to define the loop's iterator, the starting
            value, and the upper limit of looping.

            Here's an example:

                printf("I'll print out every number between ");
                printf("-500 and 500 (inclusive)!");

                forrng(i, -500, 501) {
                    printf("%i\n", i);
                }

            forrng uses a 64-bit signed integer by default.
            There are other forrng macros as well, however,
            similar to foreach, that allow iterators with
            different lengths. For example, forrng8 loops with
            a signed byte, forrng16 loops with a signed 16-bit
            integer, etc.

      * min/max macros
            This file contains two macros, min and max, that
            are just shorthands for using the ternary operator
            to determine the min/max of two values. They take
            two arguments (the two values to find the min/max of).

            Here's an example:

                int a = 5;
                int b = 2;

                printf("%i\n", max(a, b)); // will print 5 (a)
                printf("%i\n", min(a, b)); // will print 2 (b)

      * even/odd checking
            A shorthand for checking even-ness/oddness of a number
            is provided. It just &'s with 1 (to determine if
            the number's last bit is 1, indicating whether it's
            odd or even). It takes one parameter (the number to
            check the evenness/oddness of).

      * helpful definitions for angle math
            A definition for PI is included, as well as deg2rad
            and rad2deg macros. These are fairly straightfoward:
            deg2rad just takes a value interpreted as degrees and
            converts it to radians. rad2deg does the opposite.

    LICENSE INFORMATION IS AT THE END OF THE FILE
*/

#ifndef _RF_UTILS_INCLUDED
#define _RF_UTILS_INCLUDED

#include <stdint.h>

#define heap_alloc(t, n)    ((t *)malloc((n)*sizeof(t)))
#define stack_alloc(t, n)   ((t *)alloca((n)*sizeof(t)))

#define bytes(n)            (n)
#define kilobytes(n)        (n*1024)
#define megabytes(n)        (kilobytes(n)*1024)
#define gigabytes(n)        (megabytes(n)*1024)

#define foreach8(i, lim)    for(u8 i = 0; i < (u8)(lim); ++i)
#define foreach16(i, lim)   for(u16 i = 0; i < (u16)(lim); ++i)
#define foreach32(i, lim)   for(u32 i = 0; i < (u32)(lim); ++i)
#define foreach64(i, lim)   for(u64 i = 0; i < (u64)(lim); ++i)
#define foreach(i, lim)     for(u64 i = 0; i < (u64)(lim); ++i)

#define forrng8(i, l, h)    for(i8 i = (i8)(l); (i) < (i8)(h); ++i)
#define forrng16(i, l, h)   for(i16 i = (i16)(l); (i) < (i16)(h); ++i)
#define forrng32(i, l, h)   for(i32 i = (i32)(l); (i) < (i32)(h); ++i)
#define forrng64(i, l, h)   for(i64 i = (i64)(l); (i) < (i64)(h); ++i)
#define forrng(i, l, h)     for(i64 i = (i64)(l); (i) < (i64)(h); ++i)

#define max(a, b)           ((a) > (b) ? (a) : (b))
#define min(a, b)           ((a) < (b) ? (a) : (b))

#define is_even(a)          (!((a) & 0x01))

#define swap(a, b)          if(&(a) != &(b)) { (a) ^= (b); (b) ^= (a); (a) ^= (b); }

#define PI                  (3.1415926535897)
#define deg2rad(a)          (a*(PI/180.0))
#define rad2deg(a)          (a*(180.0/PI))

#define global              static

typedef int8_t   i8;
typedef int16_t  i16;
typedef int32_t  i32;
typedef int64_t  i64;
typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
typedef float    r32;
typedef double   r64;

#endif

/*
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

MIT License

Copyright (c) 2017 Ryan Fleury

Permission is hereby granted, free of charge, to any
person obtaining a copy of this software and associated
documentation files (the "Software"), to deal in the
Software without restriction, including without
limitation the rights to use, copy, modify, merge,
publish, distribute, sublicense, and/or sell copies of
the Software, and to permit persons to whom the Software
is furnished to do so, subject to the following
conditions: The above copyright notice and this permission
notice shall be included in all copies or substantial
portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF
ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED
TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*/
