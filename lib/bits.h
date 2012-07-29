#ifndef BITHACKS_H
#define BITHACKS_H

/******************************************************************************
 * Bitstring macros 
 * ````````````````
 * Macros for working with "bitstrings", that is, arrays of bytes where
 * the functional unit of logic is the individual bits. Useful for things
 * like radix trees, Bloom filters, bitboards for computer chess, or any
 * other succinct data structure or process.
 *
 * AUTHORS
 * Jason Linehan, with lots of help
 *
 * HISTORY
 * These macros were the product of a lot of experimentation and study.
 * Although there are few innovative approaches to these macros or their
 * equivalents, I consider these to be "my implementation" in that they
 * incorporate features, naming conventions, and styles from many
 * different sets of macros. Numerous things have also been altered,
 * expanded upon, or eliminated, so that they reflect my particular ideas
 * of form and function. 
 *
 * The famous header file xtrapbits.h, Copyright 1987, 1988, 1989, 1990, 
 * 1994 by Digital Equipment Corporation, was a good starting point.
 * It was used by the X11 developers to define platform-independent bit
 * array manipulation.
 *
 * Notable contributors included:
 *
 *      Dick Annicchiarico
 *      Robert Chesler
 *      Dan Coutu
 *      Gene Durso
 *      Marc Evans
 *      Alan Jamison
 *      Mark Henry
 *      Ken Miller
 *
 * Also, numerous examples floating around comp.lang.c, and many other
 * places, I'm sure.
 *
 * LICENSE
 * Public domain.
 *
 ******************************************************************************/

#include <limits.h>
#include <stdint.h>
#include <stdbool.h>

#define SEGSIZE CHAR_BIT // Number of bits in a word of the bitstring

#define BITMASK(b)      (1 << ((b) % SEGSIZE))
#define BITSEG(b)       ((b) / SEGSIZE)
#define BITSET(a,b)     ((a)[BITSEG(b)]  |=  BITMASK(b))
#define BITCLR(a,b)     ((a)[BITSEG(b)]  &= ~BITMASK(b))
#define BITTOG(a,b)     ((a)[BITSEG(b)]  ^=  BITMASK(b))
#define BITVAL(a,b)     ((a)[BITSEG(b)]  &   BITMASK(b))
#define BITFIT(nb)      (((nb) + (SEGSIZE - 1)) / SEGSIZE)
#define BIT_IS_SET(a,b) ((BITVAL((a),(b))) ? 1 : 0)



/******************************************************************************
 * Binary constants
 * ````````````````
 * Very cool macros that will expand binary literals into the appropriate
 * compile time constants.
 *
 * AUTHORS
 * Tom Torfs (basically everything)
 * Jason Linehan (added B64)
 *
 * LICENSE
 * donated to the public domain*
 *
 * USAGE
 *
 *      B8(01010101) -------------------------------> 85
 *      B16(10101010,01010101) ---------------------> 43605
 *      B32(10000000,11111111,10101010,01010101) ---> 2164238933
 *
 ******************************************************************************/

 
/* Helper macros (not intended for use by the caller)
````````````````````````````````````````````````````````````````````````````` */

/*
 * Convert a numeric literal into a hexadecimal constant 
 * (avoids problems with leading zeroes). 8-bit constants 
 * (max value 0x11111111) always fits in unsigned long.
 */
#define HEX__(X) 0x##X##LU

/*
 * Convert a sequence of eight bit literals into a valid
 * compile-time constant literal.
 * 
 * That's a bitwise AND predicating a ternary operation, 
 * in case you're scratching your head like I was.
 */
#define B8__(X)                   \
        (((X&0x0000000FLU)?1:0)   \
       + ((X&0x000000F0LU)?2:0)   \
       + ((X&0x00000F00LU)?4:0)   \
       + ((X&0x0000F000LU)?8:0)   \
       + ((X&0x000F0000LU)?16:0)  \
       + ((X&0x00F00000LU)?32:0)  \
       + ((X&0x0F000000LU)?64:0)  \
       + ((X&0xF0000000LU)?128:0))


/* User-callable macros 
````````````````````````````````````````````````````````````````````````````` */

/* for up to 8-bit binary constants */
#define B8(d) ((unsigned char)B8__(HEX__(d)))

/* for upto 16-bit binary constants, big endian (MSB first) */
#define B16(msb, lsb) (((unsigned short)B8(msb)<<8) + B8(lsb))

/* for upto 32-bit binary constants, big endian (MSB first) */
#define B32(msb, b01, b02, lsb)        \
        (((unsigned long)B8(msb)<<24)  \
       + ((unsigned long)B8(b01)<<16)  \
       + ((unsigned long)B8(b02)<<8)   \
       + B8(lsb))


/* for upto 64-bit binary constants, big endian (MSB first) */
#define B64(msb, b01, b02, b03, b04, b05, b06, lsb)        \
        (((unsigned long)B8(msb)<<56)  \
        (((unsigned long)B8(b01)<<48)  \
        (((unsigned long)B8(b02)<<40)  \
        (((unsigned long)B8(b03)<<32)  \
        (((unsigned long)B8(b04)<<24)  \
       + ((unsigned long)B8(b05)<<16)  \
       + ((unsigned long)B8(b06)<<8)   \
       + B8(lsb))



/******************************************************************************
 * Miscellaneous dark magic 
 * ````````````````````````
 * Lots of fun and useful stuff.
 ******************************************************************************/

#define IS_EVEN(x)      (((x) & 1) == 0) ? 1 : 0
#define IS_ODD(x)       (!IS_EVEN((x)))  ? 1 : 0


/* 
 * haszero (4 operations)
 * ``````````````````````
 * (text adapted from Sean Anderson's Bit Twiddling Hacks page)
 *
 * NOTES
 * The fastest method known -- uses hasless(w,1), which is defined above.
 * Requires no subsquent verification. 
 *
 * The subexpression (v - 0x01010101UL), evaluates to a high bit set in any 
 * byte whenever the corresponding byte in v is zero or greater than 0x80. 
 * The sub-expression ~v & 0x80808080UL evaluates to high bits set in bytes 
 * where the byte of v doesn't have its high bit set (so the byte was less 
 * than 0x80). Finally, by ANDing these two sub-expressions the result is 
 * the high bits set where the bytes in v were zero, since the high bits 
 * set due to a value greater than 0x80 in the first sub-expression are 
 * masked off by the second.
 *
 * AUTHOR
 * Juha Järvi suggested hasless(w,1) on April 6, 2005, which he found on 
 * Paul Hsieh's Assembly Lab; previously it was written in a newsgroup 
 * post on April 27, 1987 by Alan Mycroft. 
 * 
 */
#define haszero(v) (((v) - 0x01010101UL) & ~(v) & 0x80808080UL)


/*
 * hasvalue
 * ````````
 * (text adapted from Sean Anderson's Bit Twiddling Hacks page)
 *
 * To determine if any byte in a word has a specific value, we can XOR 
 * the "haystack" word with a word that has been filled with the byte 
 * values ("needles") we are looking for. Because XORing a value with 
 * itself results in a zero byte, and nonzero otherwise, we can pass 
 * the result to the haszero macro defined above, and get our answer.
 *
 * AUTHOR
 * Stephen M Bennet suggested this on December 13, 2009 after reading 
 * the entry for haszero. 
 */
#define hasvalue(x,n) (haszero((x) ^ (~0UL/255 * (n))))


/* 
 * hasless
 * ```````
 * (text adapted from Sean Anderson's Bit Twiddling Hacks page)
 *
 * Determine if a word has a byte less than n
 *
 * NOTES
 * Test if a word x contains an unsigned byte with value < n. 
 * Specifically, for n=1, it can be used to find a 0-byte by examining 
 * one long at a time, or any byte by XORing x with a mask first. 
 * Uses 4 arithmetic/logical operations when n is constant.
 *
 * CAVEAT
 * Requires x>=0; 0<=n<=128
 * 
 * AUTHOR
 * Juha Järvi suggested hasless(w,1) on April 6, 2005, which he found on 
 * Paul Hsieh's Assembly Lab; previously it was written in a newsgroup 
 * post on April 27, 1987 by Alan Mycroft. 
 */
#define hasless(x,n) (((x)-~0UL/255*(n))&~(x)&~0UL/255*128)


/*
 * hasmore
 * ```````
 * (text adapted from Sean Anderson's Bit Twiddling Hacks page)
 * 
 * Determine if a word has a byte greater than n
 *
 * Test if a word x contains an unsigned byte with value > n. 
 * Uses 3 arithmetic/logical operations when n is constant.
 *
 * CAVEAT
 * Requires x>=0; 0<=n<=127
 * 
 * AUTHOR
 * Juha Järvi submitted hasmore to Bit Twiddling Hacks on April 6, 2005.
 */
#define hasmore(x,n) (((x)+~0UL/255*(127-(n))|(x))&~0UL/255*128)


/*
 * likelyhasbetween
 * ````````````````
 * (text adapted from Sean Anderson's Bit Twiddling Hacks page)
 *
 * Determine if a word has a byte between m and n
 *
 * When m < n, this technique tests if a word x contains an unsigned 
 * byte value such that m<value<n. It uses 7 arithmetic/logical operations 
 * when n and m are constant.
 *
 * This technique would be suitable for a fast pretest. A variation that 
 * takes one more operation (8 total for constant m and n) but provides 
 * the exact answer is also given.
 *
 * CAVEAT
 * Bytes that equal n can be reported by likelyhasbetween as false 
 * positives, so this should be checked by character if a certain 
 * result is needed.
 *
 * Requires x>=0; 0<=m<=127; 0<=n<=128
 *
 * AUTHORS
 * Juha Järvi suggested likelyhasbetween on April 6, 2005. 
 * Sean Anderson created hasbetween on April 10, 2005. 
 */
#define likelyhasbetween(x,m,n) \
        ((((x)-~0UL/255*(n))&~(x)&((x)&~0UL/255*127)+~0UL/255*(127-(m)))&~0UL/255*128)

#define hasbetween(x,m,n) \
        ((~0UL/255*(127+(n))-((x)&~0UL/255*127)&~(x)&((x)&~0UL/255*127)+~0UL/255*(127-(m)))&~0UL/255*128)



/*
 * ones32
 * ``````
 * Compute the number of set bits (ones) in a 32-bit integer w
 * @w: unsigned 32-bit integer value
 * Returns: unsigned integer representing the number of '1' bits in w.
 *
 * NOTES
 * The population count of a binary integer value x is the number of one 
 * bits in the value. Although many machines have single instructions for 
 * this, the single instructions are usually microcoded loops that test a 
 * bit per cycle; a log-time algorithm coded in C is often faster. The 
 * following code uses a variable-precision SWAR algorithm to perform a 
 * tree reduction adding the bits in a 32-bit value:
 *
 * It is worthwhile noting that this SWAR population count algorithm can 
 * be improved upon for the case of counting the population of multi-word 
 * bit sets. How? The last few steps in the reduction are using only a 
 * portion of the SWAR width to produce their results; thus, it would be 
 * possible to combine these steps across multiple words being reduced.
 *
 * CAVEAT
 * The AMD Athlon optimization guidelines suggest a very similar algorithm 
 * that replaces the last three lines with return((x * 0x01010101) >> 24);. 
 * For the Athlon (which has a very fast integer multiply), I would have 
 * expected AMD's code to be faster... but it is actually 6% slower according 
 * to my benchmarks using a 1.2GHz Athlon (a Thunderbird). Why? Well, it so 
 * happens that GCC doesn't use a multiply instruction - it writes out the 
 * equivalent shift and add sequence! 
 * 
 * AUTHOR
 * Henry Gordon Dietz, The Aggregate Magic Algorithms
 * University of Kentucky
 * Aggregate.org online technical report (http://aggregate.org/MAGIC/)
 */
static inline unsigned int ones32(register uint32_t x)
{
        /* 32-bit recursive reduction using SWAR...
	 * but first step is mapping 2-bit values
	 * into sum of 2 1-bit values in sneaky way
	 */
        x -= ((x >> 1) & 0x55555555);
        x  = (((x >> 2) & 0x33333333) + (x & 0x33333333));
        x  = (((x >> 4) + x) & 0x0f0f0f0f);
        x += (x >> 8);
        x += (x >> 16);
        return(x & 0x0000003f);
}




/* 
 * lzc 
 * ```
 * Return the number of leading zeroes in a 32-bit word w
 * @w: unsigned 32-bit integer value
 * Returns: unsigned integer representing the number of leading 0 bits in w
 *
 * NOTES
 * Some machines have had single instructions that count the number of 
 * leading zero bits in an integer; such an operation can be an artifact 
 * of having floating point normalization hardware around. Clearly, floor 
 * of base 2 log of x is (WORDBITS-lzc(x)). In any case, this operation 
 * has found its way into quite a few algorithms, so it is useful to have 
 * an efficient implementation:  
 *
 * AUTHOR
 * Henry Gordon Dietz, The Aggregate Magic Algorithms
 * University of Kentucky
 * Aggregate.org online technical report (http://aggregate.org/MAGIC/)
 */
static inline unsigned int lzc(uint32_t w)
{
        #define LZC_WORDBITS 32

        w |= (w >> 1);
        w |= (w >> 2);
        w |= (w >> 4);
        w |= (w >> 8);
        w |= (w >> 16);

        return (LZC_WORDBITS - ones32(w));
}




/*
 * ffz
 * ```
 * Determine the offset of the first 0 bit in 32-bit word 'w'
 *
 * @w: unsigned 32-bit integer value
 * Returns: unsigned integer representing the offset of the first 0 bit in 'w'
 *
 * NOTES
 *
 * The algorithm goes like this:
 *
 *   1. Invert the number
 *   2. Compute the two's complement of the inverted number
 *   3. AND the results of (1) and (2)
 *   4. Find the position by computing the binary logarithm of (3)
 *      e.g.
 *      For the number 10110111:
 *              1. 01001000 `------- first zero
 *              2. 10111000
 *              3. 01001000 AND 10111000 = 00001000
 *              4. log2(00001000) = 3
 *                           `------- clever girl
 */
static inline unsigned int ffz(uint32_t w)
{
        #define FFZ_WORDBITS 32
        unsigned pos = 0;

        __asm__("bsfl %1,%0\n\t"
                "jne 1f\n\t"
                "movl $32, %0\n"
                "1:"
                : "=r" (pos)
                : "r" (~(w)));

        return (pos > FFZ_WORDBITS-1) ? FFZ_WORDBITS : (unsigned short)pos;
}


#endif
