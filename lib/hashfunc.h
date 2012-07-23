/******************************************************************************
 * djb2_hash
 * `````````
 * HISTORY
 * This algorithm (k=33) was first reported by Dan Bernstein many years 
 * ago in comp.lang.c. Another version of this algorithm (now favored by 
 * bernstein) uses XOR: 
 *
 *      hash(i) = hash(i - 1) * 33 ^ str[i]; 
 *
 * The magic of number 33 (why it works better than many other constants, 
 * prime or not) has never been adequately explained.
 *
 ******************************************************************************/
static inline unsigned djb2_hash(const char *str)
{
        unsigned hash; 
        int c;

        hash = 5381;

        while ((c = (unsigned char)*str++))
                hash = ((hash << 5) + hash) + c; /* hash * 33 + c */

        return (unsigned) hash;
}

/******************************************************************************
 * sdbm_hash
 * `````````
 * HISTORY
 * This algorithm was created for sdbm (a public-domain reimplementation 
 * of ndbm) database library. It was found to do well in scrambling bits, 
 * causing better distribution of the keys and fewer splits. it also 
 * happens to be a good general hashing function with good distribution. 
 *
 * The actual function is 
 *
 *      hash(i) = hash(i - 1) * 65599 + str[i]; 
 *
 * What is included below is the faster version used in gawk. [there is 
 * even a faster, duff-device version] the magic constant 65599 was picked 
 * out of thin air while experimenting with different constants, and turns 
 * out to be a prime. this is one of the algorithms used in berkeley db 
 * (see sleepycat) and elsewhere.
 *
 ******************************************************************************/
static inline unsigned sdbm_hash(const char *str)
{
        unsigned hash;
        int c;
       
        hash = 0;

        while ((c = (unsigned char)*str++))
                hash = c + (hash << 6) + (hash << 16) - hash;

        return (unsigned) hash;
}

