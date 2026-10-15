#include "common.h"
#include "lib/common.h"

void putchar(char c);

void *memset(void *buf, char val, size_t size) {
    // Turns the generic `void *` into a byte pointer
    // Write 1 byte at a time
    uint8_t *p = (uint8_t *) buf;

    // Run exactly size time
    // Check the size and decrement it
    while (size--)
        // Writes `val` to the current byte, then moves p to the next byte.
            *p++ = val;

    // Return the buffer
    return buf;
}

void *memcpy(void *dest, const void *src, size_t size) {
    uint8_t *p = (uint8_t *) dest;
    const uint8_t *s = (const uint8_t *) src;

    while (size--)
        *p++ = *s++;

    return dest;
}

size_t strlen(const char *s) {
    const char *start = s;  // initial initial starting address

    while (*s)
        // dereference, go over the string till '\0' (*s = false)
        s++;

    return s - start;       // address diff is the length
}

char *strcpy(char *dest, const char *src) {
    char *d = dest;

    while (*src)
        *d++ = *src++;
    *d = '\0';

    return dest;
}

int strcmp(const char *s1, const char *s2) {
    /*
     * Condition    Result
     *  s1 == s2	0
     *  s1 > s2	    Positive value
     *  s1 < s2	    Negative value
     */

    while (*s1 && *s2) {
        if (*s1 != *s2)
            break;

        s1++;
        s2++;
    }

    // The casting to unsigned char * when comparing is done to conform to the POSIX specification.
    return *(unsigned char *)s1 - *(unsigned char *)s2;
}

int strncmp(const char *s1, const char *s2, size_t n) {
    // Lexicographically compares two strings as in strcmp with difference
    // that is compare only up to a specified number of characters (n).
    while (n > 0 && *s1 && *s2) {
        if (*s1 != *s2)
            break;

        n--;
        s1++;
        s2++;
    }

    // Strings are identical up to `n` characters
    if (n == 0) return 0;

    return *(unsigned char *)s1 - *(unsigned char *)s2;
}

void printf(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);

    while (*fmt) {
        // Ordinary character; print it
        if (*fmt != '%') {
            // Read the character, print it and move forward
            putchar(*fmt++);
            continue;
        }

        // skip the '%'
        fmt++;
        switch (*fmt) {
            //'%' at the very end of the string
            case '\0': {
                putchar('%');
                goto end;
            }
            // "%%" prints a literal '%'
            case '%': {
                putchar('%');
                break;
            }
            // strings
            case 's': {
                // access to the next parameter from the va_list.
                // va_arg does not know the type of the arg, it is
                // explicitly passed e.g. `char ...` `int ...`
                const char *c = va_arg(args, const char *);
                // a null pointer passed to %s prints (null)
                if (c == 0) c = "(null)";
                // read, print and move forward over the characters
                while (*c) putchar(*c++);
                break;
            }
            case 'd': {
                int value = va_arg(args, int);

                /*
                 * If the value is not converted to `unsigned` before performing
                 * `value = -value` it would lead to signed integer overflow for
                 * INT_MIN e.g. +128 -> signed integer overflow
                 *
                 * Example: 8 bits (same for 16, 32, 64)
                 * There are 128 negative values but only 127 positive ones.
                 * The number -128 has no positive partner. The value +128
                 * simply doesn't exist in a signed 8-bit integer.
                 *
                 * Example (shown with 8-bit numbers for readability; the code
                 * uses 32-bit unsigned, where the same idea applies with 2^32)
                 *  value = -128
                 *  val = -128 + 256 = 128  (conversion)
                 *  -val = 256 - 128 = 128  (negation)
                 *  prints: "-" then "128"   → "-128"
                 */
                unsigned val = value;
                unsigned divisor = 1;  // convert to unsigned

                if (value < 0) {
                    putchar('-');
                    val = -val;  // negate the unsigned value
                }

                /*
                 * Find the place value of the first digit
                 * Example: magnitude = 2048
                 *  divisor = 1      2048 / 1    = 2048  > 9 → grow
                 *  divisor = 10     2048 / 10   = 204   > 9 → grow
                 *  divisor = 100    2048 / 100  = 20    > 9 → grow
                 *  divisor = 1000   2048 / 1000 = 2   ≤ 9 → stop
                 */
                while (val / divisor > 9) divisor *= 10;

                /*
                 * Peel off one digit at a time. Divide to get the current digit,
                 * use % to remove it, and shrink the divisor:
                 */
                while (divisor > 0) {
                    putchar('0' + val / divisor);
                    val %= divisor;
                    divisor /= 10;
                }

                break;
            }
            case 'x': {
                unsigned value = va_arg(args, unsigned);

                /*
                 * Reading the number four bits at a time
                 * Example: 0x1234abcd
                 *  value = 0x1234abcd
                 *  bits:   0001 0010 0011 0100 1010 1011 1100 1101
                 *  nibble:  i=7  i=6  i=5  i=4  i=3  i=2  i=1  i=0
                 *  hex:      1    2    3    4    a    b    c    d
                 *
                 * For each i from 7 down to 0:
                 *  - value >> (i * 4) slides the wanted nibble down to the lowest 4 bits.
                 *  - & 0xf (binary 1111) clears everything except those 4 bits.
                 */
                for (int i = 7; i >= 0; i--) {
                    const unsigned nibble = value >> (i * 4) & 0xf;
                    putchar("0123456789abcdef"[nibble]);
                }

                break;

            }
            // default case - unknown specifier: print it as-is
            default: {
                putchar('%');
                putchar(*fmt);
                break;
            }
        }

        // Move forward
        fmt++;
    }

    end:
        va_end(args);
}