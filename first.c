#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

int misses;
int hits;
int reads;
int writes;
int rh;
int rm;
int wh;
int wm;

/* CACHE SIMULATOR
Parameters: <cache size><associativity><block size><trace file>
    Cache & block sizes given in bytes
FIFO
Write-through cache -- write immediately to memory
Print: reads/writes per cache block, cache hits & misses
    Memory reads: %d\n
    Memory writes: %d\n
    Cache hits: %d\n
    Cache misses: %d\n
Memory size is 2pow48 -- 48 bit addresses (zero-extend if shorter)
Bits in the tag, cache address, and byte address are determined by cache & block size
    Offset bits = log2(block size)
    Index bits = log2(cache size / (associativity * block size))
    Tag bits: memory address length - index bits - offset bits
    Number of rows = cache size / (block size * associativity)
If write-miss: read block from memory, then write to memory
Read-hit: H+
Read-miss: R+, M+
Write-hit: W+, H+
Write-miss: R+, W+, M+

Strategy
2D array
    First dimension is the sets, second is the blocks containing the memory addresses
Pointer to track FIFO order
*/

int log_2(int x)
{
    int result = 0;
    while(x>>=1) result++;
    return result;
}


long int section(long int add, int pwr, int shiftBits)
{
    long int x = 1;
    //printf("\nsection\nx, pwr: %ld, %d\n", x, pwr); //
    x <<= pwr;
    //x += 1;
    //printf("pwr shift: %ld\n", x);
    x -= 1;
    x <<= shiftBits;
    //printf("bit shift: %ld\n", x);
    //printf("add&x: %ld\n", add&x);
    long int p = add&x;
    p >>= shiftBits;
    //printf("returning %ld\n\n", p);
    return p;
    /*long int mask = pow(2,pwr) - 1;
    mask<<=shiftBits;
    return add&mask;*/
}

int main(int argc, char* argv[argc + 1])
{
    reads = 0;
    writes = 0;
    hits = 0;
    misses = 0;
    // read input
    int cSize = atoi(argv[1]);
    char* assoc = argv[2];
    int bSize = atoi(argv[3]);
    FILE* fp = fopen(argv[4], "r");
    //printf("read-in complete\n");
    //printf("cSize: %d\tassoc: %s\tbSize: %d\n", cSize, assoc, bSize);

    //check if all valid format
    if(cSize <= 0 || bSize <= 0 || !fp)
    {
        //printf("error\n");
        return 0;
    }
    //printf("no error\n");

    int ass = 0;
    if(strcmp(assoc, "assoc") == 0)
    {
        ass = cSize/bSize;
        //printf("ass: %d\t", ass);
    } else if (strcmp(assoc, "direct") == 0)
    {
        ass = 1;
        //printf("ass: %d\t", ass);
    } else {
        ass = atoi(assoc + strlen(argv[2]) - 1);
        //printf("ass: %d\t", ass);
    }

    // calculate tag | index | offset bit lengths
    int set = cSize / (bSize * ass);
    int offBits = log_2(bSize);
    int indBits = log_2(set);
    int tagBits = 48 - offBits - indBits;
    //printf(" %d | %d | %d\t", tagBits, indBits, offBits);
    //printf("set: %d\n", set);

    // create the cache
    //printf("creating cache\n");
    long int cache[set][ass];
    for(int i = 0; i < set; i++)
    {
        for(int j = 0; j < ass; j++)
        {
            cache[i][j] = -1;
        }
    }

    // keep track of which index to evict next (%ass)
    int evict[set];
    for(int i = 0; i < set; i++)
    {
        evict[i] = 0;
    }
    //printf("\nevict array created\n");

    char op;
    long unsigned int addy;
    int count = 0;
    while(fscanf(fp, "%c %lx\n", &op, &addy) != EOF)
    {
        count++;
        //printf("iteration %d", count);
        long int index = section(addy, indBits, offBits);
        long int tag = section(addy, tagBits, (offBits + indBits));
        long int* current = &cache[index][0];
        //printf("evict index: %d\n", evict[index]);

        if(op == 'R')
        {
            //printf("read\n");
            for(int j = 0; j < ass; j++)
            {
                //printf("read %d\t", j);
                current = &cache[index][j];
                if(*current >= 0 && tag == *current)
                {
                    //printf("hit\ttag: %ld index: %ld\n", tag, index);
                    rh++;
                    hits++;
                    break;
                }
                if( j >= ass - 1)
                {
                    //printf("miss\ttag: %ld index: %ld\n", tag, index);
                    misses++;
                    rm++;
                    reads++;
                    cache[index][evict[index]] = tag;
                    //printf("index: %ld evict: %d\n", index, evict[index]);
                    evict[index] = (evict[index] + 1)%ass;
                }
            }
        }
        else if(op == 'W')
        {
            for(int j = 0; j < ass; j++)
            {
                //printf("write %d\t", j);
                current = &cache[index][j];
                if(*current >= 0 && tag == *current)
                {
            
                    //printf("hit\ttag: %ld index: %ld\n", tag, index);
                    wh++;
                    hits++;
                    writes++;
                    break;
                }
                if(j >= ass - 1)
                {
                    //printf("index: %ld evict: %d\n", index, evict[index]);
                    //printf("miss\t tag: %ld index:%ld\n", tag, index);
                    wm++;
                    misses++;
                    reads++;
                    writes++;
                    cache[index][evict[index]] = tag;
                    evict[index] = (evict[index] + 1)%ass;

                }
            }
        }
    }
    fclose(fp);
    //printf("rh: %d rm: %d wh: %d wm:%d\n", rh, rm, wh, wm);
    printf("memread:%d\nmemwrite:%d\ncachehit:%d\ncachemiss:%d\n", reads, writes, hits, misses);
    return EXIT_SUCCESS;
}
