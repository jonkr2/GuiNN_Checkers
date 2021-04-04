//
// This code can be used in a program to load & uncompress a file from an archive created
// by Jeffrey the Compression Butler
// I don't think this is general purpose code, because I don't think I got around to making
// it work for files compressed in multiple segments.
//
// The decompression algorithm code is heavily based on the code from Mark Nelson's Data Compression Page
// Still most of the code was written from scratch, except the arithmetic decompression code, to which full credit goes to the orignal authors
// 

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// ------------------------------------------------------
// Uncompress files from a single archive file
// ------------------------------------------------------
#define SEG_SIZE 1200000
char outfilename [255];

int number, origTotal, compTotal;

// Functions definitions 
int unBWT (unsigned char table[], unsigned char outTable[], int length, int &outsize);
int unMTF (unsigned char table[], unsigned char outTable[], int length, int &outsize);
int unARI (unsigned char table[], unsigned char outTable[], int length, int &outsize);
int unRLE (unsigned char table[], unsigned char outTable[], int length, int &outsize);

struct Header  {
    char compType, percent;
    short nameLen; // num characters in stored filename
    int dataLen;	// length of compressed file data. Max = SEG_SIZE
};

unsigned char *table = nullptr;
unsigned char *table2 = nullptr;
unsigned int *indices = nullptr;

// ---------------------
// This function is called to place the uncompressed output into *OutputTable
// filename == The archive 
// compFile == the name of the file in the archive to decompress 
// ---------------------
int uncompressFileFromArchive (char *filename, char *compFile, unsigned char *OutputTable)
{
    Header header1;
    char filename2[512], over = 1, compType;
    int size = 0, size2 = 0, files = 0, totalBytes = 0, retVal = 0;

    FILE* inFile = fopen( filename, "rb" );
    if (inFile == NULL) {
	    return -1;
	}

    fread( &header1, sizeof (header1), 1, inFile);
    if (header1.nameLen > 512 || header1.compType == 0 || header1.compType > 4 || header1.dataLen > SEG_SIZE+1 ) {
        return -1;
    }

    table = (unsigned char*)malloc( SEG_SIZE + 100000 );
    table2 = (unsigned char*)malloc( SEG_SIZE + 100000 );
    indices = (unsigned int*)malloc( (SEG_SIZE + 100000)*4 );

    while (!feof (inFile) )
	{
	    number++;
	    // Read Chunk
	    if (header1.nameLen !=0) fread( filename2, header1.nameLen, 1, inFile);							
	    fread( table, header1.dataLen, 1, inFile);
	    size = header1.dataLen;
	    compType = header1.compType;
	    fread( &header1, sizeof (header1), 1, inFile);
	
	    // uncompress File
	    if (size!=0 && strcmp(_strlwr( _strdup( compFile ) ), _strlwr( _strdup( filename2)) ) == 0)
		{
		    unARI(table, table2, size, size2);
		    unRLE(table2, table, size2, size);
		    unMTF(table, table2, size, size2);
		    unBWT(table2, table, size2, size);
		    unRLE(table, table2, size, size2);

		    memcpy(OutputTable, table2, size2);
		    retVal = 1;
		    break;
		}
	}

    fclose(inFile);
    free(table);
    free(table2);
    free(indices);
	
    return retVal;
}

// ---------------------

int inline readInt( unsigned char table[], int index)
{
    int number;
    number =  (table[ index++ ] << 24);
    number += (table[ index++ ] << 16);
    number += (table[ index++ ] << 8);
    number += (table[ index++ ]);
    return number;
}

// ----------------============================================------------------------
// Undo the Burrow-Wheeler transform
// ------------------------------------------------------------------------------------
int unBWT( unsigned char table[], unsigned char outTable[], int length, int &outsize)
{
    unsigned int Count[ 257 ];
    unsigned int RunningTotal[ 257 ];
    int i = 0, j, index;
    int first, last;
    int sum = 0;
    int buflen;
    outsize = 0;

    buflen = readInt( table, i );
    first = readInt( table, buflen + 5 );
    last = readInt( table, buflen + 9 );

    for ( i = 0 ; i < 257 ; i++ )
        Count[ i ] = 0;

    for ( i = 0; i <=  buflen; i++ )
        if ( i == last )
            Count[ 256 ]++;
        else
            Count[ table[ i + 4] ]++;
      
    for ( i = 0 ; i < 257 ; i++ ) 
	{
        RunningTotal[ i ] = sum;
        sum += Count[ i ];
		Count[ i ] = 0;
    }
 
    for ( i = 0 ; i <= buflen ; i++ ) 
	{   
        if ( i == last )
            index = 256;
        else
            index = table[ i + 4];

        indices[ Count[ index ] + RunningTotal[ index ] ] = i;
        Count[ index ]++;
    }
  
	i = first;
    for ( j = 0 ; j < buflen; j++ ) 
	{
	    outTable[ outsize++ ] = table[ i + 4 ];
        i = indices[ i ];
	}

    return 0;
}

// ------------------------------------------------------
// RLE Decode
// ----------------------------
int unRLE( unsigned char table[], unsigned char outTable[], int size, int &outindex )
{
	int index = 0, count;
    int last = 0, last2 = 1;
    unsigned char c;
	outindex = 0;

    while ( index < size )  
	{	
		c = table[ index++ ];
		outTable[ outindex++ ] = c;

        if ( c == last && c == last2 && index < size) 
		{
            count = table[index++];

			while (count == 255) 
			{
				for ( int i = 0; i < 255; i++)
					outTable[ outindex++ ] = c;

				count = table[index++];
			}

			for ( int i = 0; i < count; i++)
				outTable[ outindex++ ] = c;
		}

		last2 = last;
		last = c;
    }
    return 1;
}

// ---------------------------
// Decode Move to Front
// ---------------------------
int unMTF( unsigned char table[], unsigned char outTable[], int length, int &outsize )
{
    unsigned char order[ 256 ];
	unsigned char c;
    register int i;
	int index = 0;
	outsize = 0;

    for ( i = 0; i < 256; i++ )
        order[ i ] = (unsigned char)i;

    while ( index < length )  
	{
		i = table[ index++ ];
		c = order[ i ];
		outTable[ outsize++ ] = c;
      
        for ( i ; i > 0 ; i-- )
            order[ i ] = order[ i - 1 ];
        order[ 0 ] = c;
	}
    return 1;
}


// ---------------------------------
//  Arithmetic Decoding
//
//  (This code is not written by me.)
// --------------------------------

#define No_of_chars 256                 /* Number of character symbols      */
#define EOF_symbol (No_of_chars+1)      /* Index of EOF symbol              */

#define No_of_symbols (No_of_chars+1)   /* Total number of symbols          */

/* TRANSLATION TABLES BETWEEN CHARACTERS AND SYMBOL INDEXES. */

int char_to_index[No_of_chars];         /* To index from character          */
unsigned char index_to_char[No_of_symbols+1]; /* To character from index    */

/* ADAPTIVE SOURCE MODEL */

int freq[No_of_symbols+1];      /* Symbol frequencies                       */
int cum_freq[No_of_symbols+1];  /* Cumulative symbol frequencies            */

#define Code_value_bits 16              /* Number of bits in a code value   */
typedef long code_value;                /* Type of an arithmetic code value */

#define Top_value (((long)1<<Code_value_bits)-1)      /* Largest code value */


/* HALF AND QUARTER POINTS IN THE CODE VALUE RANGE. */

#define First_qtr (Top_value/4+1)       /* Point after first quarter        */
#define Half	  (2*First_qtr)         /* Point after first half           */
#define Third_qtr (3*First_qtr)         /* Point after third quarter        */


/* CUMULATIVE FREQUENCY TABLE. */

#define Max_frequency 16383             /* Maximum allowed frequency count  */

extern int cum_freq[No_of_symbols+1];   /* Cumulative symbol frequencies    */

/* BIT OUTPUT ROUTINES. */

/* THE BIT BUFFER. */

int buffer;                     /* Bits buffered for output                 */
int bits_to_go;                 /* Number of bits free in buffer            */


unsigned char *outBuffer;
unsigned char *inBuffer;
int bufferSize;
/* INITIALIZE FOR BIT OUTPUT. */

void start_outputing_bits( void )
{   buffer = 0;                                 /* Buffer is empty to start */
    bits_to_go= 8;                              /* with.                    */
}

/* OUTPUT A BIT. */

inline void output_bit( int bit )
{   buffer >>= 1; if (bit) buffer |= 0x80;      
    bits_to_go -= 1;
    if (bits_to_go==0) 
		{outBuffer [ bufferSize ++ ] = unsigned char (buffer);
         bits_to_go = 8;
		}
}

/* FLUSH OUT THE LAST BITS. */

void done_outputing_bits( void )
{ 
	outBuffer [ bufferSize ++ ] = unsigned char (buffer >> bits_to_go);
}

/* CURRENT STATE OF THE ENCODING. */

code_value low, high;           /* Ends of the current code region          */
long bits_to_follow;            /* Number of opposite bits to output after  */
                                /* the next bit.                            */

/* THE ADAPTIVE SOURCE MODEL */

/* INITIALIZE THE MODEL. */

void start_model( void )
{   int i;
    for (i = 0; i<No_of_chars; i++) {           /* Set up tables that       */
        char_to_index[i] = i+1;                 /* translate between symbol */
        index_to_char[i+1] = (unsigned char) i; /* indexes and characters.  */
    }
    for (i = 0; i<=No_of_symbols; i++) {        /* Set up initial frequency */
        freq[i] = 1;                            /* counts to be one for all */
        cum_freq[i] = No_of_symbols-i;          /* symbols.                 */
    }
    freq[0] = 0;                                /* Freq[0] must not be the  */
}                                               /* same as freq[1].         */


/* UPDATE THE MODEL TO ACCOUNT FOR A NEW SYMBOL. */

void update_model( int symbol )
{   int i;					                    /* New index for symbol     */
    if (cum_freq[0]==Max_frequency) {           /* See if frequency counts  */
        int cum;                                /* are at their maximum.    */
        cum = 0;
        for (i = No_of_symbols; i>=0; i--) {    /* If so, halve all the     */
            freq[i] = (freq[i]+1)/2;            /* counts (keeping them     */
            cum_freq[i] = cum;                  /* non-zero).               */
            cum += freq[i];
        }
    }
    for (i = symbol; freq[i]==freq[i-1]; i--) ; /* Find symbol's new index. */
    if (i<symbol) {
        int ch_i, ch_symbol;
        ch_i = index_to_char[i];                /* Update the translation   */
        ch_symbol = index_to_char[symbol];      /* tables if the symbol has */
        index_to_char[i] = (unsigned char) ch_symbol;           /* moved.   */
        index_to_char[symbol] = (unsigned char) ch_i;
        char_to_index[ch_i] = symbol;
        char_to_index[ch_symbol] = i;
    }
    freq[i] += 1;                               /* Increment the frequency  */
    while (i>0) {                               /* count for the symbol and */
        i -= 1;                                 /* update the cumulative    */
        cum_freq[i] += 1;                       /* frequencies.             */
    }
}

/* ============================== */
/* ARITHMETIC DECODING ALGORITHM. */
/* ============================== */
/* CURRENT STATE OF THE DECODING. */

static code_value value;        /* Currently-seen code value                */
int bufferTotal;

/* INITIALIZE BIT INPUT. */

void start_inputing_bits( void )
{   bits_to_go = 0;                             /* Buffer starts out with   */
}


/* INPUT A BIT. */

inline int input_bit( void )
{   int t;
    if (bits_to_go==0) 
		{                        /* Read the next byte if no */
        buffer = inBuffer [bufferSize++];       /* bits are left in buffer. */
        bits_to_go = 8;
		}
    t = buffer&1;                               /* Return the next bit from */
    buffer >>= 1;                               /* the bottom of the byte.  */
    bits_to_go -= 1;
    return t;
}

/* START DECODING A STREAM OF SYMBOLS. */

void start_decoding( void )
{   int i;
    value = 0;                                  /* Input bits to fill the   */
    for (i = 1; i<=Code_value_bits; i++) {      /* code value.              */
        value = 2*value+input_bit();
    }
    low = 0;                                    /* Full code range.         */
    high = Top_value;
}


/* DECODE THE NEXT SYMBOL. */

inline int decode_symbol( int cum_freq[] )
{   long range;                 /* Size of current code region              */
    int cum;                    /* Cumulative frequency calculated          */
    int symbol;                 /* Symbol decoded                           */
    range = (long)(high-low)+1;
    cum = (int)                                 /* Find cum freq for value. */
      ((((long)(value-low)+1)*cum_freq[0]-1)/range);
    for (symbol = 1; cum_freq[symbol]>cum; symbol++) ; /* Then find symbol. */
    high = low +                                /* Narrow the code region   */
      (range*cum_freq[symbol-1])/cum_freq[0]-1; /* to that allotted to this */
    low = low +                                 /* symbol.                  */
      (range*cum_freq[symbol])/cum_freq[0];
    for (;;) {                                  /* Loop to get rid of bits. */
        if (high<Half) {
            /* nothing */                       /* Expand low half.         */
        }
        else if (low>=Half) {                   /* Expand high half.        */
            value -= Half;
            low -= Half;                        /* Subtract offset to top.  */
            high -= Half;
        }
        else if (low>=First_qtr                 /* Expand middle half.      */
              && high<Third_qtr) {
            value -= First_qtr;
            low -= First_qtr;                   /* Subtract offset to middle*/
            high -= First_qtr;
        }
        else break;                             /* Otherwise exit loop.     */
        low = 2*low;
        high = 2*high+1;                        /* Scale up code range.     */
        value = 2*value+input_bit();            /* Move in next input bit.  */
    }
    return symbol;
}

// Main unARI
int unARI (unsigned char table[], unsigned char table2[], int size, int &outsize)
{
 int ticker = 0, index = 0, ch, symbol;
	bufferSize = 0; outsize = 0;
	inBuffer = table;
	bufferTotal = size;

    start_model();                               /* Set up other modules.    */
    start_inputing_bits();
    start_decoding();
    for (;;) 
		{symbol = decode_symbol(cum_freq);       /* Decode next symbol.      */
         if (symbol == EOF_symbol) break;         /* Exit loop if EOF symbol. */
         ch = index_to_char[symbol];             /* Translate to a character.*/
         table2 [ outsize++ ] = ch;              /* Write that character.    */
         update_model(symbol);                   /* Update the model.        */
    }
	outsize --;
    return 1;
}
