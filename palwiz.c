// palwiz.c - copyright 2005-2007 Bruce Tomlin

#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
//#include "asmcfg.h"
//#include "asmguts.h"

#define versionName "Simple PAL/GAL compiler"
#define versionNum "1.1"
#define copyright "Copyright 2005-2007 Bruce Tomlin"

// these should already be defined for POSIX compatibility
//typedef	unsigned char	u_char;
//typedef	unsigned short	u_short;
//typedef	unsigned int	u_int;
//typedef	unsigned long	u_long;

// a few useful typedefs
typedef unsigned char  bool;    // i guess bool is a c++ thing
const bool FALSE = 0;
const bool TRUE  = 1;
typedef char Str255[256];       // generic string type

// --------------------------------------------------------------

#define MAXPINS 24+2+1 // max pins + max phantom pins + 1 (so one-based subscripts can be used)
#define MAXTERMS   132 // maximum number of fuse map rows
#define MAXCOLS     44 // maximum number of fuse map columns
#define MAXFUSES  6000 // maximum number of fuses
#define MAXSKIPS    20 // maximum number of skips in fuse map output
#define MAXOPTS      2 // maximum number of option fuses per output pin or max chip-wide option fuses
#define MAXMODES     8 // maximum number of pin mode combinations
#define MAXTESTS  9999 // maximum number of test vectors (too lazy to use a linked list)

int     numCols;            // number of columns in term matrix
int     numRows;            // number of rows in term matrix
int     devPins;            // number of pins on device
int     phantomPins;        // devPins + number of phantom pins
int     devFuses;           // total number of fuses in device

bool    fusemap[MAXFUSES];  // fuse map
char    *pinName[MAXPINS];  // pointer to pin name, null until assigned
char    pinPhase[MAXPINS];  // 1=active high, 0=active low
char    inFuse[MAXPINS];    // fuse map column of input pin, -1 GND, -2 VCC
int     outFuse[MAXPINS];   // first fuse for output pin
int     outRows[MAXPINS];   // number of rows for output pin
int     outOE[MAXPINS];     // 0 = no OE row, 1 = output pin has an OE row, 2 = optional OE row
                            // negative = fixed OE input pin, outOE[0] is used for 16V8R/20V8R
char    nextRow[MAXPINS];   // next unused row for output pin
int     skipLine[MAXSKIPS]; // line skip points in JEDEC fuse map output
int     sigFuse;            // first signature fuse
int     numSigFuses;        // number of signature fuses
int     optFuses[MAXPINS][MAXOPTS]; // option fuses, pin 0 = chip-wide fuses
int     numTests;           // number of test vectors
int     defFuse;            // default fuse state
bool    secFuse;            // TRUE to blow security fuse
int     pinModes[MAXPINS];  // allowable modes for output pins
int     pinMode[MAXPINS];   // current mode for output pins
int     modes[MAXMODES];    // bit-mapped option pin combinations for each mode
bool    outDef[MAXPINS];    // TRUE if output pin defined
bool    oeDef[MAXPINS];     // TRUE if output pin OE defined
int     ptdBase;            // first PTD (product term disable) fuse, one per row
char    *tests[MAXTESTS];   // test vectors
//char    tests[MAXTESTS][40];// test vectors

Str255  chip;

// output pin modes
#define M_XOR    1  // XOR output pin
#define M_REG    2  // registered
#define M_INPUT  4  // dedicated input

// --------------------------------------------------------------

const char		*progname;		// pointer to argv[0]

Str255			line;				// Current line from input file
char            *linePtr;			// pointer into current line
int				linenum;			// line number in main source file
int             curToken;
Str255          word;

//	Command line parameters
Str255			cl_SrcName;			// Source file name

FILE			*source;			// source input file
FILE			*object;			// object output file
//FILE			*listing;			// listing output file

// --------------------------------------------------------------
// error messages


void EatLine(void); // forward declaration


/*
 *	Error
 */

void Error(char *message)
{
	fprintf(stderr, "%s:%d:%s\n",cl_SrcName,linenum,message);

//  exit(1);

    EatLine();
}


void Warning(char *message)
{
	fprintf(stderr, "%s:%d: *** Warning:  %s ***\n",cl_SrcName,linenum,message);
}


// --------------------------------------------------------------
// string utilities


/*
 *	Debleft deblanks the string s from the left side
 */

void Debleft(char *s)
{
	char *p = s;

	while (*p == 9 || *p == ' ')
		p++;

    if (p != s)
        while ((*s++ = *p++));
}


/*
 *	Debright deblanks the string s from the right side
 */

void Debright(char *s)
{
	char *p = s + strlen(s);

	while (p>s && *--p == ' ')
		*p = 0;
}


/*
 *	Deblank removes blanks from both ends of the string s
 */

void Deblank(char *s)
{
	Debleft(s);
	Debright(s);
}


/*
 *	Uprcase converts string s to upper case
 */

void Uprcase(char *s)
{
	char *p = s;

	while ((*p = toupper(*p)))
		p++;
}


int ishex(char c)
{
	c = toupper(c);
	return isdigit(c) || ('A' <= c && c <= 'F');
}


int isalphanum(char c)
{
	c = toupper(c);
	return isdigit(c) || ('A' <= c && c <= 'Z') || c == '_';
}


char *newstring(char *s)
{
    char *p;
    p = malloc(strlen(s)+1);
    if (p)
        strcpy(p,s);
    else
        Error("can't allocate new string");
    return p;
}


u_int EvalBin(char *binStr)
{
	u_int	binVal;
	int		evalErr;
	int		c;

	evalErr = FALSE;
	binVal  = 0;

	while ((c = *binStr++))
	{
		if (c < '0' || c > '1')
			evalErr = TRUE;
		else
			binVal = binVal * 2 + c - '0';
 	}

	if (evalErr)
	{
      binVal = 0;
      Error("Invalid binary number");
	}

   return binVal;
}


u_int EvalOct(char *octStr)
{
	u_int	octVal;
	int		evalErr;
	int		c;

	evalErr = FALSE;
	octVal  = 0;

	while ((c = *octStr++))
	{
		if (c < '0' || c > '7')
			evalErr = TRUE;
		else
			octVal = octVal * 8 + c - '0';
 	}

	if (evalErr)
	{
      octVal = 0;
      Error("Invalid octal number");
	}

   return octVal;
}


u_int EvalDec(char *decStr)
{
	u_int	decVal;
	int		evalErr;
	int		c;

	evalErr = FALSE;
	decVal  = 0;

	while ((c = *decStr++))
	{
		if (!isdigit(c))
			evalErr = TRUE;
		else
			decVal = decVal * 10 + c - '0';
 	}

	if (evalErr)
	{
      decVal = 0;
      Error("Invalid decimal number");
	}

   return decVal;
}


int Hex2Dec(c)
{
	c = toupper(c);
	if (c > '9')
		return c - 'A' + 10;
	return c - '0';
}


u_int EvalHex(char *hexStr)
{
	u_int	hexVal;
	int		evalErr;
	int		c;

	evalErr = FALSE;
	hexVal  = 0;

	while ((c = *hexStr++))
	{
		if (!ishex(c))
			evalErr = TRUE;
		else
			hexVal = hexVal * 16 + Hex2Dec(c);
 	}

	if (evalErr)
	{
      hexVal = 0;
      Error("Invalid hexadecimal number");
	}

	return hexVal;
}


u_int EvalNum(char *word)
{
	int val;

	// handle C-style 0xnnnn hexadecimal constants
	if(word[0] == '0')
	{
		if (toupper(word[1]) == 'X')
		{
			return EvalHex(word+2);
		}
		// return EvalOct(word);	// 0nnn octal constants are in less demand, though
	}

	val = strlen(word) - 1;
	switch(word[val])
	{
		case 'B':
			word[val] = 0;
			val = EvalBin(word);
			break;

		case 'O':
			word[val] = 0;
			val = EvalOct(word);
			break;

		case 'D':
			word[val] = 0;
			val = EvalDec(word);
			break;

		case 'H':
			word[val] = 0;
			val = EvalHex(word);
			break;

		default:
			val = EvalDec(word);
			break;
	}

	return val;
}


// --------------------------------------------------------------
// token handling

// returns 0 for end-of-line, -1 for alpha-numeric, else char value for non-alphanumeric
// converts the word to uppercase, too
int GetWord(char *word)
{
	u_char	c;

	word[0] = 0;

	// skip initial whitespace
	c = *linePtr;
	while (c == 12 || c == '\t' || c == ' ')
		c = *++linePtr;

	// skip comments
	if (c == '/' && linePtr[1] == '/')
		while (c)
			c = *++linePtr;

	// test for end of line
	if (c)
	{
		// test for alphanumeric token
		if (isalphanum(c))
		{
			while (isalphanum(c) || c == '$')
			{
				*word++ = toupper(c);
				c = *++linePtr;
			}
			*word = 0;
			return -1;
		}
		else
		{
			word[0] = c;
			word[1] = 0;
			linePtr++;
			return c;
		}
	}

	return 0;
}


// --------------------------------------------------------------
// text I/O


int ReadLine(FILE *file, char *line, int max)
{
	int c = 0;
	int len = 0;

    linePtr = line;
	{
		linenum++;

		while (max > 1)
		{
			c = fgetc(file);
			*line = 0;
			switch(c)
			{
				case EOF:
					if (len == 0) return 0;
				case '\n':
					return 1;
				case '\r':
					break;
				default:
					*line++ = c;
					max--;
					len++;
					break;
			}
		}
		while (c != EOF && c != '\n')
			c = fgetc(file);
	}
	return 1;
}


int GetSym(void)
{
    int i;

    curToken = GetWord(word);
    if (curToken == 0)
    {
        i = ReadLine(source, line, sizeof(line));
        while(i)
        {
            curToken = GetWord(word);
            if (curToken) break;

            i = ReadLine(source, line, sizeof(line));
        }
    }

    return curToken;
}


int GetNum(void)
{
    if (GetSym() == -1)
    {
        return EvalNum(word);
    }
    Error("Number expected");
    return 0;
}


void Expect(char *expected)
{
	Str255 s;
	GetSym();
	if (strcmp(word,expected) != 0)
	{
		sprintf(s,"\"%s\" expected",expected);
		Error(s);
	}
}


// returns TRUE if 's' == 'word'
bool kwd(char *word, char *s)
{
    return (strcmp(word,s) == 0);
}


void EatLine(void)
{
    int     token;

    if (curToken != ';')
    {
        token = 1;
        while (token != ';' && token != 0)
            token = GetSym();
    }
}


// --------------------------------------------------------------
// PAL specific routines


int FindPin(char *name)
{
    int i;

    for (i=0; i<=phantomPins; i++)
    {
        if (pinName[i] && strcmp(pinName[i],word) == 0)
            return i;
    }
    return 0;
}


// --------------------------------------------------------------


void DoChip(void)
{
    int     i;
    int     token;
    Str255  s;

    token = GetSym();
    if (token == -1)
    {
        strcpy(chip,word);

        if (kwd(word,"22V10"))
        {
            // 22V10, global reset on phantom pin 25, preset on pin 26
            numCols     =   44;
            numRows     =  132;
            devFuses    = 5892;
            devPins     =   24;
            phantomPins =   26;

            inFuse[ 1] =  0;
            inFuse[ 2] =  4;
            inFuse[ 3] =  8;
            inFuse[ 4] = 12;
            inFuse[ 5] = 16;
            inFuse[ 6] = 20;
            inFuse[ 7] = 24;
            inFuse[ 8] = 28;
            inFuse[ 9] = 32;
            inFuse[10] = 36;
            inFuse[11] = 40;
            inFuse[12] = -1; // GND
            inFuse[13] = 42;
            inFuse[14] = 38;
            inFuse[15] = 34;
            inFuse[16] = 30;
            inFuse[17] = 26;
            inFuse[18] = 22;
            inFuse[19] = 18;
            inFuse[20] = 14;
            inFuse[21] = 10;
            inFuse[22] =  6;
            inFuse[23] =  2;
            inFuse[24] = -2; // VCC

            skipLine[ 0] =   44;
            skipLine[ 1] =  440;
            skipLine[ 2] =  924;
            skipLine[ 3] = 1496;
            skipLine[ 4] = 2156;
            skipLine[ 5] = 2904;
            skipLine[ 6] = 3652;
            skipLine[ 7] = 4312;
            skipLine[ 8] = 4884;
            skipLine[ 9] = 5368;
            skipLine[10] = 5764;
            skipLine[11] = 5808;
            skipLine[12] = 5828;

            outOE  [ 0] =    1;
            outFuse[23] =   44;
            outRows[23] =    9;
            outOE  [23] =    1;
            outFuse[22] =  440;
            outRows[22] =   11;
            outOE  [22] =    1;
            outFuse[21] =  924;
            outRows[21] =   13;
            outOE  [21] =    1;
            outFuse[20] = 1496;
            outRows[20] =   15;
            outOE  [20] =    1;
            outFuse[19] = 2156;
            outRows[19] =   17;
            outOE  [19] =    1;
            outFuse[18] = 2904;
            outRows[18] =   17;
            outOE  [18] =    1;
            outFuse[17] = 3652;
            outRows[17] =   15;
            outOE  [17] =    1;
            outFuse[16] = 4312;
            outRows[16] =   13;
            outOE  [16] =    1;
            outFuse[15] = 4884;
            outRows[15] =   11;
            outOE  [15] =    1;
            outFuse[14] = 5368;
            outRows[14] =    9;
            outOE  [14] =    1;

            outFuse[25] =    0; // RESET
            outRows[25] =    1;
            outOE  [25] =    0;
//            pinName[25] = newstring("G_RESET");
            outFuse[26] = 5764; // PRESET
            outRows[26] =    1;
            outOE  [26] =    0;
//            pinName[26] = newstring("G_PRESET");

            ptdBase = -1; // no product term disable fuses

            for (i=14; i<=23; i++)
                pinModes[i] = M_XOR + M_REG;
            modes[0] =  3; // 0: combH - S0=1 S1=1
            modes[1] =  2; // 1: combL - S0=0 S1=1
            modes[2] =  1; // 2: regH  - S0=1 S1=0
            modes[3] =  0; // 3: regL  - S0=0 S1=0
            modes[4] = -1; // 4: input - not allowed (use OE instead)

/*
int     pinModes[MAXPINS];  // allowable modes for output pins
int     pinMode[MAXPINS];   // current mode for output pins
int     modes[MAXMODES];    // bit-mapped option pin combinations for each mode

Str255  chip;

// output pin modes
#define M_XOR    1  // XOR output pin
#define M_REG    2  // registered
#define M_INPUT  4  // dedicated input
*/
            sigFuse     = 5828;
            numSigFuses =   64;

            optFuses[23][0] = 5808; // S0
            optFuses[23][1] = 5809; // S1
            optFuses[22][0] = 5810;
            optFuses[22][1] = 5811;
            optFuses[21][0] = 5812;
            optFuses[21][1] = 5813;
            optFuses[20][0] = 5814;
            optFuses[20][1] = 5815;
            optFuses[19][0] = 5816;
            optFuses[19][1] = 5817;
            optFuses[18][0] = 5818;
            optFuses[18][1] = 5819;
            optFuses[17][0] = 5820;
            optFuses[17][1] = 5821;
            optFuses[16][0] = 5822;
            optFuses[16][1] = 5823;
            optFuses[15][0] = 5824;
            optFuses[15][1] = 5825;
            optFuses[14][0] = 5826;
            optFuses[14][1] = 5827;

/*
modes:
  S0=0 S1=0 - reg, active low
  S0=1 S1=0 - reg, active high
  S0=0 S1=1 - combo, active low
  S0=1 S1=1 - combo, active high
*/
        }
        else if (kwd(word,"20V8S"))
        {
            // 20V8, simple mode - pins 18 and 19 are output-only
            numCols     =   40;
            numRows     =   64;
            devFuses    = 2706;
            devPins     =   24;
            phantomPins =   24; // no phantom pins

            // SYN = 1, AC0 = 0
            fusemap[2704] = TRUE;
            fusemap[2705] = FALSE;

            inFuse[ 1] =  2;
            inFuse[ 2] =  0;
            inFuse[ 3] =  4;
            inFuse[ 4] =  8;
            inFuse[ 5] = 12;
            inFuse[ 6] = 16;
            inFuse[ 7] = 20;
            inFuse[ 8] = 24;
            inFuse[ 9] = 28;
            inFuse[10] = 32;
            inFuse[11] = 36;
            inFuse[12] = -1; // GND
            inFuse[13] = 38;
            inFuse[14] = 34;
            inFuse[15] = 30;
            inFuse[16] = 26;
            inFuse[17] = 22;
            inFuse[18] = -3; // no feedback
            inFuse[19] = -4; // no feedback
            inFuse[20] = 18;
            inFuse[21] = 14;
            inFuse[22] = 10;
            inFuse[23] =  6;
            inFuse[24] = -2; // VCC

            skipLine[ 0] =  320;
            skipLine[ 1] =  640;
            skipLine[ 2] =  960;
            skipLine[ 3] = 1280;
            skipLine[ 4] = 1600;
            skipLine[ 5] = 1920;
            skipLine[ 6] = 2240;
            skipLine[ 7] = 2560;
            skipLine[ 8] = 2568;
            skipLine[ 9] = 2632;
            skipLine[10] = 2640;
            skipLine[11] = 2704;
            skipLine[12] = 2705;

            outOE  [ 0] =    0;
            outFuse[22] =    0;
            outRows[22] =    8;
            outOE  [22] =    0;
            outFuse[21] =  320;
            outRows[21] =    8;
            outOE  [21] =    0;
            outFuse[20] =  640;
            outRows[20] =    8;
            outOE  [20] =    0;
            outFuse[19] =  960;
            outRows[19] =    8;
            outOE  [19] =    0;
            outFuse[18] = 1280;
            outRows[18] =    8;
            outOE  [18] =    0;
            outFuse[17] = 1600;
            outRows[17] =    8;
            outOE  [17] =    0;
            outFuse[16] = 1920;
            outRows[16] =    8;
            outOE  [16] =    0;
            outFuse[15] = 2240;
            outRows[15] =    8;
            outOE  [15] =    0;

            ptdBase = 2640; // product term disable fuses, one per row

            for (i=14; i<=23; i++)
                pinModes[i] = M_XOR;
            modes[0] =  1; // 0: combH - XOR=1 AC1=0
            modes[1] =  0; // 1: combL - XOR=0 AC1=0
            modes[2] = -1; // 2: regH  - not allowed
            modes[3] = -1; // 3: regL  - not allowed
            modes[4] =  3; // 4: input - XOR=X AC1=1

            sigFuse     = 2568;
            numSigFuses =   64;

            optFuses[22][0] = 2560; // XOR
            optFuses[22][1] = 2632; // AC1
            optFuses[21][0] = 2561;
            optFuses[21][1] = 2633;
            optFuses[20][0] = 2562;
            optFuses[20][1] = 2634;
            optFuses[19][0] = 2563;
            optFuses[19][1] = 2635;
            optFuses[18][0] = 2564;
            optFuses[18][1] = 2636;
            optFuses[17][0] = 2565;
            optFuses[17][1] = 2637;
            optFuses[16][0] = 2566;
            optFuses[16][1] = 2638;
            optFuses[15][0] = 2567;
            optFuses[15][1] = 2639;
/*
    SYN=1 AC0=0 - simple mode
    XOR=0 AC1=0 - feedback, active low
    XOR=1 AC1=0 - feedback, active high
    XOR=X AC1=1 - dedicated input pin
*/
        }
        else if (kwd(word,"20V8C"))
        {
            // 20V8, complex mode - pins 15 and 22 are output-only
            numCols     =   40;
            numRows     =   64;
            devFuses    = 2706;
            devPins     =   24;
            phantomPins =   24; // no phantom pins

            // SYN = 1, AC0 = 1
            fusemap[2704] = TRUE;
            fusemap[2705] = TRUE;

            inFuse[ 1] =  2;
            inFuse[ 2] =  0;
            inFuse[ 3] =  4;
            inFuse[ 4] =  8;
            inFuse[ 5] = 12;
            inFuse[ 6] = 16;
            inFuse[ 7] = 20;
            inFuse[ 8] = 24;
            inFuse[ 9] = 28;
            inFuse[10] = 32;
            inFuse[11] = 36;
            inFuse[12] = -1; // GND
            inFuse[13] = 38;
            inFuse[14] = 34;
            inFuse[15] = -3; // no feedback
            inFuse[16] = 30;
            inFuse[17] = 26;
            inFuse[18] = 22;
            inFuse[19] = 18;
            inFuse[20] = 14;
            inFuse[21] = 10;
            inFuse[22] = -4; // no feedback
            inFuse[23] =  6;
            inFuse[24] = -2; // VCC

            skipLine[ 0] =  320;
            skipLine[ 1] =  640;
            skipLine[ 2] =  960;
            skipLine[ 3] = 1280;
            skipLine[ 4] = 1600;
            skipLine[ 5] = 1920;
            skipLine[ 6] = 2240;
            skipLine[ 7] = 2560;
            skipLine[ 8] = 2568;
            skipLine[ 9] = 2632;
            skipLine[10] = 2640;
            skipLine[11] = 2704;
            skipLine[12] = 2705;

            outOE  [ 0] =    1;
            outFuse[22] =    0;
            outRows[22] =    8;
            outOE  [22] =    1;
            outFuse[21] =  320;
            outRows[21] =    8;
            outOE  [21] =    1;
            outFuse[20] =  640;
            outRows[20] =    8;
            outOE  [20] =    1;
            outFuse[19] =  960;
            outRows[19] =    8;
            outOE  [19] =    1;
            outFuse[18] = 1280;
            outRows[18] =    8;
            outOE  [18] =    1;
            outFuse[17] = 1600;
            outRows[17] =    8;
            outOE  [17] =    1;
            outFuse[16] = 1920;
            outRows[16] =    8;
            outOE  [16] =    1;
            outFuse[15] = 2240;
            outRows[15] =    8;
            outOE  [15] =    1;

            ptdBase = 2640; // product term disable fuses, one per row

            for (i=14; i<=23; i++)
                pinModes[i] = M_XOR;
            modes[0] =  3; // 0: combH - XOR=1 AC1=1
            modes[1] =  2; // 1: combL - XOR=0 AC1=1
            modes[2] = -1; // 2: regH  - not allowed
            modes[3] = -1; // 3: regL  - not allowed
            modes[4] = -1; // 4: input - not allowed

            sigFuse     = 2568;
            numSigFuses =   64;

            optFuses[22][0] = 2560; // XOR
            optFuses[22][1] = 2632; // AC1
            optFuses[21][0] = 2561;
            optFuses[21][1] = 2633;
            optFuses[20][0] = 2562;
            optFuses[20][1] = 2634;
            optFuses[19][0] = 2563;
            optFuses[19][1] = 2635;
            optFuses[18][0] = 2564;
            optFuses[18][1] = 2636;
            optFuses[17][0] = 2565;
            optFuses[17][1] = 2637;
            optFuses[16][0] = 2566;
            optFuses[16][1] = 2638;
            optFuses[15][0] = 2567;
            optFuses[15][1] = 2639;
/*
    SYN=1 AC0=1 - complex mode
    XOR=0 AC1=1 - combo, active low
    XOR=1 AC1=1 - combo, active high
    pin 16-21 have feedback, pins 15 and 22 have no feedback
*/
        }
        else if (kwd(word,"20V8R"))
        {
            // 20V8, registered mode
            numCols     =   40;
            numRows     =   64;
            devFuses    = 2706;
            devPins     =   24;
            phantomPins =   24; // no phantom pins

            // SYN = 0, AC0 = 1
            fusemap[2704] = FALSE;
            fusemap[2705] = TRUE;

            inFuse[ 1] = -3; // CLK
            inFuse[ 2] =  0;
            inFuse[ 3] =  4;
            inFuse[ 4] =  8;
            inFuse[ 5] = 12;
            inFuse[ 6] = 16;
            inFuse[ 7] = 20;
            inFuse[ 8] = 24;
            inFuse[ 9] = 28;
            inFuse[10] = 32;
            inFuse[11] = 36;
            inFuse[12] = -1; // GND
            inFuse[13] = -4; // OE
            inFuse[14] = 38;
            inFuse[15] = 34;
            inFuse[16] = 30;
            inFuse[17] = 26;
            inFuse[18] = 22;
            inFuse[19] = 18;
            inFuse[20] = 14;
            inFuse[21] = 10;
            inFuse[22] =  6;
            inFuse[23] =  2;
            inFuse[24] = -2; // VCC

            skipLine[ 0] =  320;
            skipLine[ 1] =  640;
            skipLine[ 2] =  960;
            skipLine[ 3] = 1280;
            skipLine[ 4] = 1600;
            skipLine[ 5] = 1920;
            skipLine[ 6] = 2240;
            skipLine[ 7] = 2560;
            skipLine[ 8] = 2568;
            skipLine[ 9] = 2632;
            skipLine[10] = 2640;
            skipLine[11] = 2704;
            skipLine[12] = 2705;

            outOE  [ 0] =    2;
            outFuse[22] =    0;
            outRows[22] =    8;
            outOE  [22] =    2;
            outFuse[21] =  320;
            outRows[21] =    8;
            outOE  [21] =    2;
            outFuse[20] =  640;
            outRows[20] =    8;
            outOE  [20] =    2;
            outFuse[19] =  960;
            outRows[19] =    8;
            outOE  [19] =    2;
            outFuse[18] = 1280;
            outRows[18] =    8;
            outOE  [18] =    2;
            outFuse[17] = 1600;
            outRows[17] =    8;
            outOE  [17] =    2;
            outFuse[16] = 1920;
            outRows[16] =    8;
            outOE  [16] =    2;
            outFuse[15] = 2240;
            outRows[15] =    8;
            outOE  [15] =    2;

            ptdBase = 2640; // product term disable fuses, one per row

            for (i=14; i<=23; i++)
                pinModes[i] = M_XOR + M_REG;
            modes[0] =  3; // 0: combH - XOR=1 AC1=1
            modes[1] =  2; // 1: combL - XOR=0 AC1=1
            modes[2] =  1; // 2: regH  - XOR=1 AC1=0
            modes[3] =  0; // 3: regL  - XOR=0 AC1=0
            modes[4] = -1; // 4: not allowed (use OE instead)

            sigFuse     = 2568;
            numSigFuses =   64;

            optFuses[22][0] = 2560; // XOR
            optFuses[22][1] = 2632; // AC1
            optFuses[21][0] = 2561;
            optFuses[21][1] = 2633;
            optFuses[20][0] = 2562;
            optFuses[20][1] = 2634;
            optFuses[19][0] = 2563;
            optFuses[19][1] = 2635;
            optFuses[18][0] = 2564;
            optFuses[18][1] = 2636;
            optFuses[17][0] = 2565;
            optFuses[17][1] = 2637;
            optFuses[16][0] = 2566;
            optFuses[16][1] = 2638;
            optFuses[15][0] = 2567;
            optFuses[15][1] = 2639;

/*
    SYN=0 AC0=1 - registered mode
    XOR=0 AC1=0 - reg, active low
    XOR=1 AC1=0 - reg, active high
    XOR=0 AC1=1 - combo, active low
    XOR=1 AC1=1 - combo, active high
    pin 1 is global clock
    pin 13 is global !OE

note: output pin feedback is always active low in registered mode
*/
        }
        else if (kwd(word,"16V8S"))
        {
            numCols     =   32;
            numRows     =   64;
            devFuses    = 2194;
            devPins     =   20;
            phantomPins =   20; // no phantom pins

            // SYN = 1, AC0 = 0
            fusemap[2192] = TRUE;
            fusemap[2193] = FALSE;

            inFuse[ 1] =  2;
            inFuse[ 2] =  0;
            inFuse[ 3] =  4;
            inFuse[ 4] =  8;
            inFuse[ 5] = 12;
            inFuse[ 6] = 16;
            inFuse[ 7] = 20;
            inFuse[ 8] = 24;
            inFuse[ 9] = 28;
            inFuse[10] = -1; // GND
            inFuse[11] = 30;
            inFuse[12] = 26;
            inFuse[13] = 22;
            inFuse[14] = 18;
            inFuse[15] = -3; // no feedback
            inFuse[16] = -4; // no feedback
            inFuse[17] = 14;
            inFuse[18] = 10;
            inFuse[19] =  6;
            inFuse[20] = -2; // VCC

            skipLine[ 0] =  256;
            skipLine[ 1] =  512;
            skipLine[ 2] =  768;
            skipLine[ 3] = 1024;
            skipLine[ 4] = 1280;
            skipLine[ 5] = 1536;
            skipLine[ 6] = 1792;
            skipLine[ 7] = 2048;
            skipLine[ 8] = 2056;
            skipLine[ 9] = 2120;
            skipLine[10] = 2128;
            skipLine[11] = 2192;
            skipLine[12] = 2193;

            outOE  [ 0] =    0;
            outFuse[19] =    0;
            outRows[19] =    8;
            outOE  [19] =    0;
            outFuse[18] =  256;
            outRows[18] =    8;
            outOE  [18] =    0;
            outFuse[17] =  512;
            outRows[17] =    8;
            outOE  [17] =    0;
            outFuse[16] =  768;
            outRows[16] =    8;
            outOE  [16] =    0;
            outFuse[15] = 1024;
            outRows[15] =    8;
            outOE  [15] =    0;
            outFuse[14] = 1280;
            outRows[14] =    8;
            outOE  [14] =    0;
            outFuse[13] = 1536;
            outRows[13] =    8;
            outOE  [13] =    0;
            outFuse[12] = 1792;
            outRows[12] =    8;
            outOE  [12] =    0;

            ptdBase = 2128; // product term disable fuses, one per row

            for (i=12; i<=19; i++)
                pinModes[i] = M_XOR;
            modes[0] =  1; // 0: combH - XOR=1 AC1=0
            modes[1] =  0; // 1: combL - XOR=0 AC1=0
            modes[2] = -1; // 2: regH  - not allowed
            modes[3] = -1; // 3: regL  - not allowed
            modes[4] =  3; // 4: input - XOR=X AC1=1

            sigFuse     = 2568;
            numSigFuses =   64;

            optFuses[19][0] = 2048; // XOR
            optFuses[19][1] = 2120; // AC1
            optFuses[18][0] = 2049;
            optFuses[18][1] = 2121;
            optFuses[17][0] = 2050;
            optFuses[17][1] = 2122;
            optFuses[16][0] = 2051;
            optFuses[16][1] = 2123;
            optFuses[15][0] = 2052;
            optFuses[15][1] = 2124;
            optFuses[14][0] = 2053;
            optFuses[14][1] = 2125;
            optFuses[13][0] = 2054;
            optFuses[13][1] = 2126;
            optFuses[12][0] = 2055;
            optFuses[12][1] = 2127;
        }
        else if (kwd(word,"16V8C"))
        {
            numCols     =   32;
            numRows     =   64;
            devFuses    = 2194;
            devPins     =   20;
            phantomPins =   20; // no phantom pins

            // SYN = 1, AC0 = 1
            fusemap[2192] = TRUE;
            fusemap[2193] = TRUE;

            inFuse[ 1] =  2;
            inFuse[ 2] =  0;
            inFuse[ 3] =  4;
            inFuse[ 4] =  8;
            inFuse[ 5] = 12;
            inFuse[ 6] = 16;
            inFuse[ 7] = 20;
            inFuse[ 8] = 24;
            inFuse[ 9] = 28;
            inFuse[10] = -1; // GND
            inFuse[11] = 30;
            inFuse[12] = -3; // no feedback
            inFuse[13] = 26;
            inFuse[14] = 22;
            inFuse[15] = 18;
            inFuse[16] = 14;
            inFuse[17] = 10;
            inFuse[18] =  6;
            inFuse[19] = -4; // no feedback
            inFuse[20] = -2; // VCC

            skipLine[ 0] =  256;
            skipLine[ 1] =  512;
            skipLine[ 2] =  768;
            skipLine[ 3] = 1024;
            skipLine[ 4] = 1280;
            skipLine[ 5] = 1536;
            skipLine[ 6] = 1792;
            skipLine[ 7] = 2048;
            skipLine[ 8] = 2056;
            skipLine[ 9] = 2120;
            skipLine[10] = 2128;
            skipLine[11] = 2192;
            skipLine[12] = 2193;

            outOE  [ 0] =    1;
            outFuse[19] =    0;
            outRows[19] =    8;
            outOE  [19] =    1;
            outFuse[18] =  256;
            outRows[18] =    8;
            outOE  [18] =    1;
            outFuse[17] =  512;
            outRows[17] =    8;
            outOE  [17] =    1;
            outFuse[16] =  768;
            outRows[16] =    8;
            outOE  [16] =    1;
            outFuse[15] = 1024;
            outRows[15] =    8;
            outOE  [15] =    1;
            outFuse[14] = 1280;
            outRows[14] =    8;
            outOE  [14] =    1;
            outFuse[13] = 1536;
            outRows[13] =    8;
            outOE  [13] =    1;
            outFuse[12] = 1792;
            outRows[12] =    8;
            outOE  [12] =    1;

            ptdBase = 2128; // product term disable fuses, one per row

            for (i=12; i<=19; i++)
                pinModes[i] = M_XOR;
            modes[0] =  3; // 0: combH - XOR=1 AC1=1
            modes[1] =  2; // 1: combL - XOR=0 AC1=1
            modes[2] = -1; // 2: regH  - not allowed
            modes[3] = -1; // 3: regL  - not allowed
            modes[4] = -1; // 4: input - not allowed

            sigFuse     = 2568;
            numSigFuses =   64;

            optFuses[19][0] = 2048; // XOR
            optFuses[19][1] = 2120; // AC1
            optFuses[18][0] = 2049;
            optFuses[18][1] = 2121;
            optFuses[17][0] = 2050;
            optFuses[17][1] = 2122;
            optFuses[16][0] = 2051;
            optFuses[16][1] = 2123;
            optFuses[15][0] = 2052;
            optFuses[15][1] = 2124;
            optFuses[14][0] = 2053;
            optFuses[14][1] = 2125;
            optFuses[13][0] = 2054;
            optFuses[13][1] = 2126;
            optFuses[12][0] = 2055;
            optFuses[12][1] = 2127;
        }
        else if (kwd(word,"16V8R"))
        {
            numCols     =   32;
            numRows     =   64;
            devFuses    = 2194;
            devPins     =   20;
            phantomPins =   20; // no phantom pins

            // SYN = 0, AC0 = 1
            fusemap[2192] = FALSE;
            fusemap[2193] = TRUE;

            inFuse[ 1] = -3; // CLK
            inFuse[ 2] =  0;
            inFuse[ 3] =  4;
            inFuse[ 4] =  8;
            inFuse[ 5] = 12;
            inFuse[ 6] = 16;
            inFuse[ 7] = 20;
            inFuse[ 8] = 24;
            inFuse[ 9] = 28;
            inFuse[10] = -1; // GND
            inFuse[11] = -4; // OE
            inFuse[12] = 30;
            inFuse[13] = 26;
            inFuse[14] = 22;
            inFuse[15] = 18;
            inFuse[16] = 14;
            inFuse[17] = 10;
            inFuse[18] =  6;
            inFuse[19] =  2;
            inFuse[20] = -2; // VCC

            skipLine[ 0] =  256;
            skipLine[ 1] =  512;
            skipLine[ 2] =  768;
            skipLine[ 3] = 1024;
            skipLine[ 4] = 1280;
            skipLine[ 5] = 1536;
            skipLine[ 6] = 1792;
            skipLine[ 7] = 2048;
            skipLine[ 8] = 2056;
            skipLine[ 9] = 2120;
            skipLine[10] = 2128;
            skipLine[11] = 2192;
            skipLine[12] = 2193;

            outOE  [ 0] =    2;
            outFuse[19] =    0;
            outRows[19] =    8;
            outOE  [19] =    2;
            outFuse[18] =  256;
            outRows[18] =    8;
            outOE  [18] =    2;
            outFuse[17] =  512;
            outRows[17] =    8;
            outOE  [17] =    2;
            outFuse[16] =  768;
            outRows[16] =    8;
            outOE  [16] =    2;
            outFuse[15] = 1024;
            outRows[15] =    8;
            outOE  [15] =    2;
            outFuse[14] = 1280;
            outRows[14] =    8;
            outOE  [14] =    2;
            outFuse[13] = 1536;
            outRows[13] =    8;
            outOE  [13] =    2;
            outFuse[12] = 1792;
            outRows[12] =    8;
            outOE  [12] =    2;

            ptdBase = 2128; // product term disable fuses, one per row

            for (i=12; i<=19; i++)
                pinModes[i] = M_XOR + M_REG;
            modes[0] =  3; // 0: combH - XOR=1 AC1=1
            modes[1] =  2; // 1: combL - XOR=0 AC1=1
            modes[2] =  1; // 2: regH  - XOR=1 AC1=0
            modes[3] =  0; // 3: regL  - XOR=0 AC1=0
            modes[4] = -1; // 4: not allowed (use OE instead)

            sigFuse     = 2568;
            numSigFuses =   64;

            optFuses[19][0] = 2048; // XOR
            optFuses[19][1] = 2120; // AC1
            optFuses[18][0] = 2049;
            optFuses[18][1] = 2121;
            optFuses[17][0] = 2050;
            optFuses[17][1] = 2122;
            optFuses[16][0] = 2051;
            optFuses[16][1] = 2123;
            optFuses[15][0] = 2052;
            optFuses[15][1] = 2124;
            optFuses[14][0] = 2053;
            optFuses[14][1] = 2125;
            optFuses[13][0] = 2054;
            optFuses[13][1] = 2126;
            optFuses[12][0] = 2055;
            optFuses[12][1] = 2127;
/*
CL1[19..12] = 2048-2055
signature fuses 2056-2119
CL0[19..12] = 2120-2127
PTD terms = 2128-2191

note: output pin feedback is always active low in registered mode
*/
        }
        else
        {
            sprintf(s,"Unknown chip type '%s'",word);
            Error(s);
        }
    }

    if (ptdBase>=0)
        for (i=ptdBase; i<ptdBase+numRows; i++)
            fusemap[i] = TRUE;
}


void DoPin(void)
{
    int i;
    int pin;
    int token;
    Str255 s;

    if (chip[0] == 0)
    {
        Error("chip type not declared!");
        exit(1);
    }

    pin = GetNum();
    if (pin < 1 || pin > phantomPins)
    {
        sprintf(s,"invalid pin number %d",pin);
        Error(s);
        return;
    }

    if (pinName[pin])
    {
        sprintf(s,"pin %d already named '%s'",pin,pinName[pin]);
        Error(s);
        return;
    }

    pinPhase[pin] = 1;  // default to active-high
    token = GetSym();
    if (token == '!' || token == '/')
    {
        pinPhase[pin] = 0;
        token = GetSym();
    }
    if (token != -1 || !isalpha(word[0]))
    {
        Error("pin name expected");
    }
    else
    {
        if ((i = FindPin(word)) != 0)
        {
                sprintf(s,"pin %d name '%s' already used for pin %d",pin,pinName[i],i);
                Error(s);
                return;
        }
        else pinName[pin] = newstring(word);
    }
//printf("DoPin: pin = %2d, phase = %d, name = '%s'\n",pin,pinPhase[pin],pinName[pin]);
}


void FuseEqn(int row, int lastRow)
{
    int i;
    bool inputPhase;
    int col;
    int token;
    int input;
    Str255 s;
    bool newrow;

//printf("fuseeqn: row=%d, lastRow=%d\n",row,lastRow);

    token = GetSym();
    if (kwd(word,"1"))
    {
        // disconnect all input terms because a zero-input AND is always = 1
        for (i=row; i<row+numCols; i++)
            fusemap[i] = TRUE;
        return;
    }
    else if (kwd(word,"0"))
    {
        // connect all input terms because A & !A always = 0
        for (i=row; i<row+numCols; i++)
            fusemap[i] = FALSE;
        return;
    }

    newrow = TRUE;
    while (token == '!' || token == '/' || token == -1)
    {
        // initialize new row to all fuses set
        if (newrow)
        {
            if (row >= lastRow)
            {
                Error("too many terms for output pin");
                return;
            }
            for (i=row; i<row+numCols; i++)
                fusemap[i] = TRUE;
            newrow = FALSE;
        }

        inputPhase = TRUE;
        if (token == '!' || token == '/')
        {
            inputPhase = FALSE;
            token = GetSym();
        }
        input = FindPin(word);
        if (input == 0)
        {
            sprintf(s,"invalid pin '%s'",word);
            Error(s);
            return;
        }
        else
        {
            col = inFuse[input];
            if (col < 0)
            {
                sprintf(s,"pin '%s' is not an input pin",word);
                Error(s);
                return;
            }

            if (inputPhase != pinPhase[input])
                col = col + 1;

            if (!fusemap[row+col])
            {
                sprintf(s,"input '%s' has already been used",word);
                Warning(s);
            }
            fusemap[row+col] = FALSE;

            // look for operator or end of statement
            token = GetSym();
            if (token == '|' || token == '+')
            {
                row = row + numCols;
                newrow = TRUE;
            }
            if (token == '&' || token == '*' ||
                token == '|' || token == '+')
            {
                // get next symbol
                token = GetSym();
            }
        }
    }
}

void DoEqn(int pin, bool phase)
{
    int     i;
    int     row;
    int     lastRow;
    int     token;
    bool    mainterm;
    Str255  s;

    if (chip[0] == 0)
    {
        Error("chip type not declared!");
        exit(1);
    }

    mainterm = TRUE;

    // make sure it's an output pin
    row = outFuse[pin];
    lastRow = row + numCols * outRows[pin];
    if (row < 0)
    {
        Error("left side of equation must be an output pin");
        return;
    }

//printf("DoEqn: pin = %d, phase = %d\n",pin,phase);

    token = GetSym();
    if (token == '.')
    {
        mainterm = FALSE;
        token = GetSym();
        if (kwd(word,"OE"))
        {
            if (outOE[pin] <= 0)
            {
                Error("pin does not have OE term");
                return;
            }
            if (oeDef[pin])
            {
                Error("pin OE term already defined");
                return;
            }

            // check if optional OE term (16V8R / 20V8R)
            if (outOE[pin] == 2)
            {
                if (pinMode[pin] & M_REG)
                {
                    sprintf(s,"Can't use .OE term on registered output pin %d (%s)", pin,pinName[pin]);
                    Error(s);
                    return;
                }

                outOE[pin] = 1;
                // block move other terms down in case they were already defined
                // note: really should check if last row wasn't blank because it'll be lost
                for (i=lastRow-1; i>=row+numCols; i--)
                    fusemap[i] = fusemap[i-numCols];
                // clear out OE row
                for (i=row; i<row+numCols; i++)
                    fusemap[i] = 0;
            }

            lastRow = row + numCols;
            oeDef[pin] = TRUE;
            token = GetSym();
        }
        else
        {
            Error("unknown control pin");
            return;
        }
    }
    else
    {
        // skip first row if output enable
        if (outOE[pin] == 1) row = row + numCols;
        lastRow = outFuse[pin] + numCols * outRows[pin];

        if (outDef[pin])
        {
            Error("pin already defined");
            return;
        }
        outDef[pin] = TRUE;

        if (phase != pinPhase[pin])
        {
            if (pinModes[pin] & M_XOR)
            {
                pinMode[pin] |=  M_XOR;
                pinMode[pin] &= ~M_INPUT;
//printf("pin %d mode is now %d\n",pin,pinMode[pin]);
            }
            else
            {
                Error("pin does not support output XOR");
                return;
            }
        }
    }

    // look for assignment operator
    switch(token)
    {
        case ':':
            token = GetSym();
            if (token != '=')
            {
                Error("invalid operator in equation");
                return;
            }
            if (!mainterm || !(pinModes[pin] & M_REG))
            {
                Error("can't make registered assignment");
                return;
            }
            if (outOE[0] == 2 && oeDef[pin])
            {
                // 16V8R / 20V8R can't use OE terms on registered outputs
                sprintf(s,"Can't use .OE term on registered output pin %d (%s)", pin,pinName[pin]);
                Error(s);
                return;
            }

            // operator = registered assignment
            pinMode[pin] |=  M_REG;
            pinMode[pin] &= ~M_INPUT;
//printf("pin %d mode is now %d\n",pin,pinMode[pin]);
            break;

        case '=':
            // operator = assignment
            pinMode[pin] &= ~M_INPUT;
//printf("pin %d mode is now %d\n",pin,pinMode[pin]);
            break;

        default:
            Error("invalid operator in equation");
            return;
    }

    FuseEqn(row,lastRow);
}


// Before dumping the fusemap, check for registered feedback terms.
// Registered feedback terms on all of 16V8/20V8/22V10 are inverted, so
// look for any input terms that use them. If found, just swap the bits.

void DoFeedbackTerms(void)
{
    int pin;
    int row;
    int fuse;

    for (pin = 1; pin <= devPins; pin++)
    {
        if (pinMode[pin] & M_REG)
        {
            for (row=0; row<numRows; row++)
            {
                fuse = inFuse[pin] + row * numCols;
                if (fusemap[fuse] ^ fusemap[fuse+1])
                {
                    fusemap[fuse  ] = !fusemap[fuse  ];
                    fusemap[fuse+1] = !fusemap[fuse+1];
                }
            }
        }
    }
}


bool AddTest(char *test)
{
    if (numTests >= MAXTESTS)
    {
        Error("too many test vectors");
        return TRUE;
    }
    else
    {
        tests[numTests] = newstring(test);
        if (tests[numTests] == NULL)
        {
            Error("can't allocate string for test vector");
            return TRUE;
        }
//        strcpy(tests[numTests],test);
        numTests++;
    }
    return FALSE;
}


int FindInput(int col)
{
    int i;

    // this should probably be an automatically generated array

    for (i=1; i<=devPins; i++)
    {
        if (inFuse[i] == col) return i;
    }
    return 0;
}


bool IsFeedbackPin(int pin)
{
    int fuse;

    if (outFuse[pin] < 0) return 0;         // not a feedback pin if no row fuse for pin
    if (pinMode[pin] & M_REG) return 1;     // always a feedback pin if registered
    if (pinMode[pin] & M_INPUT) return 0;   // never a feedback term if input-only

    // if any pair of input fuses is 01 or 10, return 1
    for (fuse = outFuse[pin]; fuse < outFuse[pin] + numCols*outRows[pin]; fuse = fuse + 2)
    {
        if (fusemap[fuse] ^ fusemap[fuse+1])
        {
          printf("input term at fuse %d\n",fuse);
            return 1;
        }
    }

    // otherwise it's probably not a feedback term
    return 0;
}


void TestPins(void)
{
    Str255 s;
    char test[MAXPINS+1];
    int i,j,pin;
    int combo;
    int result;
    int fuse;
    int row,col;
    int token;
    int numInp,numOut;
    int pinType[MAXPINS+1];
    int pinVal[MAXPINS+1];
    int phase[MAXPINS+1];

    enum {nullPin, inpPin, outPin};

//printf("--- TestPins ---\n");

    for (i=0; i < devPins; i++)
    {
        test[i]    = 'N';
        pinType[i] = nullPin;
        phase[i]   = TRUE;
    }
    test[i] = 0;
    numInp = 0;
    numOut = 0;

    // first get the names of the output pins
    token = curToken;
    if (token == ';')
    {
        Error("no test pins specified");
        return;
    }
    while (token != 0 && token != ';')
    {
        pin = FindPin(word);
        if (pin<1 || outFuse[pin]<0 || (pinMode[pin] & M_REG) || (pinMode[pin] & M_INPUT))
        {
            sprintf(s,"pin '%s' must be a non-registered output pin",word);
            Error(s);
            return;
        }
        else if (pinType[pin] == outPin)
        {
            Error("pin specified twice");
            return;
        }
        {
            pinType[pin] = outPin;
            if (pinMode[pin] & M_XOR) phase[pin] = FALSE;
                                 else phase[pin] = TRUE;
            numOut++;

            token = GetSym();
        }
//printf("Test output pin %d (%s), mode=%d, pinPhase=%d, phase=%d\n",pin,pinName[pin],pinMode[pin],pinPhase[pin],phase[pin]);
    }

    // for every output pin, find its inputs
    for (pin=1; pin<=devPins; pin++)
        if (pinType[pin] == outPin)
        {
//printf("Pin %d (%s) is an output, first row=%d, rows=%d\n",pin,pinName[pin],(outOE[pin]==1) ? 1:0,outRows[pin]);
            for (row = (outOE[pin]==1) ? 1:0; row < outRows[pin]; row++)
                for (col=0; col<numCols; col = col + 2)
                {
                    fuse = outFuse[pin] + row*numCols + col;
                    if (fusemap[fuse] ^ fusemap[fuse+1])
                    {
                        i = FindInput(col);
                        if (i < 1 || i > devPins)
                        {
                            // input column not found
                            sprintf(s,"Output pin %d (%s), input from column %d is not a valid pin",
                                        pin,pinName[pin],col);
                            Error(s);
                            return;
                        }
                        if (IsFeedbackPin(i))
                        {
                            // feedback pins can be a real mess, so I'm not handling them now
                            sprintf(s,"Output pin %d (%s), input from pin %d (%s) is a feedback term",
                                        pin,pinName[pin],i,pinName[i]);
                            Error(s);
                            return;
                        }
                        if (pinType[i] == nullPin)
                        {
//printf("  input from pin %d (%s)\n",i,pinName[i]);
                            pinType[i] = inpPin;
//                            phase[i]   = pinPhase[i];
////                            if (fusemap[fuse+1] == 0)
////                                phase[i] = FALSE;
                            numInp++;
                        }
                    }
                }
        }
/*
    // create template from input and output pins
    for (pin=1; pin<=devPins; pin++)
    {
        switch(pinType[pin])
        {
            default:
            case nullPin: test[pin-1] = 'N'; break;
            case inpPin:  test[pin-1] = '0'; break;
            case outPin:  test[pin-1] = 'L'; break;
        }
    }
    printf("%d input terms, %d output terms, template = '%s'\n",numInp,numOut,test);
*/
    // iterate all combinations of inputs
    for (combo=0; combo < (1<<numInp); combo++)
    {
        j = 1;
        for (pin=devPins; pin>0; pin--)
        {
            switch(pinType[pin])
            {
                default:
                case nullPin:
                    test[pin-1] = 'N';
                    break;
                case inpPin:
                    pinVal[pin] = (combo & j) != 0;
                    if (combo & j) test[pin-1] = '1';
                              else test[pin-1] = '0';
                    j = j << 1;
                    break;
                case outPin:
                    test[pin-1] = '?';
                    break;
            }
        }

        // simulate output pins
        for (pin=devPins; pin>0; pin--)
            if (pinType[pin] == outPin)
            {
                result = 1;
                for (row = (outOE[pin]==1) ? 1:0; row < outRows[pin]; row++)
                {
                    result = 1;
                    for (col=0; col<numCols; col = col + 2)
                    {
                        fuse = outFuse[pin] + row*numCols + col;
//printf("%.4d: pin %d, row %d, col %d, fuse=%d: %d %d %d\n",combo,pin,row,col,fuse,fusemap[fuse],fusemap[fuse+1],
//fusemap[fuse] + fusemap[fuse+1]*2);
                        switch(fusemap[fuse] + fusemap[fuse+1]*2)
                        {
                            case 0: // both terms used
//printf("  both terms used, result = 0\n");
                                result = 0;
                                break;
                            case 1: // inverted term used
                                i = FindInput(col);
//printf("  input pin %d (%s), !value=%d\n",i,pinName[i],!pinVal[i]);
                                if (i >= 1 && i <=devPins)
                                {
                                    result = !pinVal[i];
//                                    if (phase[i]) result = !result;
                                }
                                break;
                            case 2: // non-inverted term used
                                i = FindInput(col);
//printf("  input pin %d (%s), value=%d\n",i,pinName[i],pinVal[i]);
                                if (i >= 1 && i <=devPins)
                                {
                                    result = pinVal[i];
//                                    if (phase[i]) result = !result;
                                }
                                break;
                            default:
                            case 3: // neither input used
//printf("  neither term used, continuing\n");
                                break;
                        }
//printf("  result = %d\n",result);
                        if (result == 0) break;
                    }
                    if (result == 1) break;
                }
//printf("%.4d: pin %d (%s) phase=%d result=%d\n",combo,pin,pinName[pin],phase[pin],result);
                if (!phase[pin]) result = !result;
                if (result == 0) test[pin-1] = 'L';
                            else test[pin-1] = 'H';
            }

        //printf("%.4d: %s\n",combo,test);
        if (AddTest(test)) return;
    }
}


void DoTest(void)
{
    char test[MAXPINS+1];
    int i;
    bool phase;
    char vals[4];

    int token;

    test[0] = 0;

    token = GetSym();
    if (token == '[')
    {
        vals[0] = '0';
        vals[1] = '1';
        vals[2] = 'C';
        vals[3] = 'K';

        for (i=0; i < devPins; i++)
            test[i] = 'N';
        test[i] = 0;

        token = GetSym();
        while (token != ']' && token != ';' && token != 0)
        {
            phase = TRUE;
            if (token == '!' || token == '/')
            {
                phase = FALSE;
                token = GetSym();
            }
            if (token == -1)
            {
                i = FindPin(word);
                if (i > 0)
                {
                    if (!pinPhase[i])
                        phase = !phase;

                    token = GetSym();
                    if (token == '=')
                    {
                        token = GetSym();
                        if (word[0] != 0 && word[1] == 0 &&
                            ( (vals[0]=='0' && word[0] != 'H'
                                            && word[0] != 'L') ||
                              (vals[0]!='0' && word[0] != '0'
                                            && word[0] != '1'
                                            && word[0] != 'C'
                                            && word[0] != 'K') ))
                        {
                            if (!phase)
                            {
                                switch(word[0])
                                {
                                case '0': word[0]='1'; break;
                                case '1': word[0]='0'; break;
                                case 'L': word[0]='H'; break;
                                case 'H': word[0]='L'; break;
                                case 'C': word[0]='K'; break;
                                case 'K': word[0]='C'; break;
                                }
                            }
                            test[i-1] = word[0];
                        }
                        else Error("invalid pin test value");
                        token = GetSym();
                    }
                    else test[i-1] = vals[phase];
                }
                else Error("unknown pin name");
            }
            else Error("pin name expected");

            token = curToken;
            if (token == '-')
            {
                token = GetSym();
                if (token != '>')
                    Error("'->' expected");
                else if (vals[0] != '0')
                    Error("'->' already specified");
                else
                {
                    token = GetSym();
                    vals[0] = 'L';
                    vals[1] = 'H';
                    vals[2] = ' ';
                    vals[3] = ' ';
                }
            }
        }

        AddTest(test);
    }
    else if (FindPin(word) > 0)
    {
        TestPins();
    }
    else
    {
        // raw JEDEC test vectors
        while (token != ';')
        {
            strcat(test,word);
            token = GetSym();
        }
        AddTest(test);
    }
}

bool Statement(void)
{
    int         token;
    int         pin;
    Str255      s;

    token = GetSym();
    if (token == 0) return 0;
    if (token != ';')
    {
             if (kwd(word,"CHIP")) DoChip();
        else if (kwd(word,"PIN" ))  DoPin();
        else if (kwd(word,"TEST")) DoTest();
        else if (token == '!' || token == '/')
        {
            token = GetSym();
            if (token != -1 || (pin = FindPin(word)) == 0)
            {
                Error("invalid pin name");
            }
            else
            {
                DoEqn(pin,0);
            }
        }
        else if ((pin = FindPin(word)) != 0)
        {
            DoEqn(pin,1);
        }
        else
        {
            sprintf(s,"Invalid keyword '%s'",word);
            Error(s);
        }
    }
    if (curToken != ';')
    {
        GetSym();
        if (curToken != ';')
            Error("';' expected");
    }
    return 1;
}


void DoModes(void)
{
    int i;
    int pin;
    int mode;
    int fuse;

    for (pin=1; pin<=devPins; pin++)
    {
        if (outFuse[pin] >= 0)
        {
            mode = modes[pinMode[pin]];
            if (mode < 0)
            {
                if (pinMode[pin] == M_INPUT)
                    fprintf(stderr,"pin %d (%s) invalid output mode (use OE to disable output)\n",pin,pinName[pin]);
                else
                    fprintf(stderr,"pin %d (%s) invalid output mode\n",pin,pinName[pin]);
            }
            else
            {
//fprintf(stderr,"pin %d output mode = %d:%d - ",pin,pinMode[pin],mode);
                for (i=0; i<MAXOPTS; i++)
                {
                    fuse = optFuses[pin][i];
                    if (fuse > 0)
                    { if (mode & (1 << i)) fusemap[fuse] = 1;
                                      else fusemap[fuse] = 0;
//fprintf(stderr," fuse %d = %d",fuse,fusemap[fuse]);
                    }
                }
//fprintf(stderr,"\n");
            }
        }
    }
}


void DoFile(void)
{

//	fseek(source, 0, SEEK_SET);	// rewind source file

	linenum = 0;
    linePtr = line;

    while (Statement())
    {
    }
}


void DumpFuseMap(void)
{
    int     addr,i,col;
    int     chksum;

    col = 0;
    addr = 0;
    chksum = 0;
    fprintf(object,"\nL%.4d\n",addr);

    while (addr < devFuses)
    {
        if (fusemap[addr])
            chksum = chksum + (1 << (addr & 7));

        for (i=0; i<MAXSKIPS && skipLine[i] && addr != skipLine[i]; i++) /* loop */;
        if (addr != 0 && addr == skipLine[i])
        {
            fprintf(object,"*\n\nL%.4d\n",addr);
            col = 0;
        }
        else if (addr < numRows * numCols && col == numCols)
        {
            printf("\n");
            col = 0;
        }
        if (fusemap[addr]) printf("1");
                      else printf("0");
        addr++;
        col++;
    }

    fprintf(object,"*\n");
    fprintf(object,"\nC%.4X*\n\n",chksum);   // print fusemap checksum
}


void DumpTests()
{
    int i;

    if (numTests)
    {
        fprintf(object,"N test vectors *\n\n");

        for (i=0; i<numTests; i++)
            fprintf(object,"V%.4d %s*\n",i+1,tests[i]);

        fprintf(object,"\n");
    }
}


void DumpInfo(void)
{
    fprintf(object,"%c",2);         // print STX character
    // print comments at top
    fprintf(object,"Chip type = %s\n",chip);
    fprintf(object,"Source file: %s\n",cl_SrcName);
    fprintf(object,"*\n");

    fprintf(object,"QP%2d*    N %d pins * \n",devPins,devPins);
    fprintf(object,"QF%.4d*  N %d fuses *\n",devFuses,devFuses);
    fprintf(object,"QV%.4d*  N %d test vectors *\n",numTests,numTests);
    fprintf(object,"F%d*      N default fuse state *\n",defFuse);
    if (secFuse) fprintf(object,"G1*      N blow the security fuse *\n");
            else fprintf(object,"G0*      N don't blow the security fuse *\n");

    DumpFuseMap();

    DumpTests();

    fprintf(object,"%c%.4X\n",3,0);    // print ETX character and bogus text checksum
}


void Initialize(void)
{
    int i,j;

    chip[0] = 0;

    numCols     = 0;
    numRows     = 0;
    devPins     = 0;
    phantomPins = 0;
    devFuses    = 0;
    sigFuse     = 0;
    numSigFuses = 0;
    numTests    = 0;
    defFuse     = 0;
    secFuse     = FALSE;

    for (i=0; i<MAXFUSES; i++)
        fusemap[i] = FALSE;

    for (i=0; i<MAXPINS; i++)
    {
        pinName [i] =  NULL;
        pinPhase[i] =     1;
        inFuse  [i] =    -1;
        outFuse [i] =    -1;
        outRows [i] =     0;
        outOE   [i] =     0;
        nextRow [i] =     0;
        pinModes[i] =     0;
        pinMode [i] =     0;
        outDef  [i] = FALSE;
        oeDef   [i] = FALSE;
        pinMode [i] = M_INPUT;

        for (j=0; j<MAXOPTS; j++)
        {
            optFuses[i][j] = -1;
        }
    }

    for (i=0; i<MAXSKIPS; i++)
    {
        skipLine[i] = 0;
    }

    memset(tests,0,sizeof(tests));
}


// --------------------------------------------------------------
// initialization and parameters


void usage(void)
{
    fprintf(stderr,"%s version %s\n",versionName,versionNum);
	fprintf(stderr,"\n");
	fprintf(stderr,"Usage:\n");
	fprintf(stderr,"    %s srcfile\n",progname);
}


void getopts(int argc, char * const argv[])
{
	int ch;
//	int val;
//	Str255 labl,word;
//	int neg;

	while ((ch = getopt(argc, argv, "?")) != -1)
	{
		switch (ch)
		{
			case '?':
			default:
				usage();
		}
	}
	argc -= optind;
	argv += optind;

	// now argc is the number of remaining arguments
	// and argv[0] is the first remaining argument

	if (argc != 1)
		usage();

	strncpy(cl_SrcName, argv[0], 255);

	// note: this won't work if there's a single-char filename in the current directory!
	if (cl_SrcName[0] == '?' && cl_SrcName[1] == 0)
		usage();
}


int main(int argc, char * const argv[])
{
//	int i;

	// initialize and get parms

	progname  = argv[0];

	cl_SrcName [0] = 0;		source  = NULL;

	getopts(argc, argv);

	// open files

	source = fopen(cl_SrcName, "r");
	if (source == NULL)
	{
		fprintf(stderr,"Unable to open source input file '%s'!\n",cl_SrcName);
		exit(1);
	}

    object = stdout;

    Initialize();
    DoFile();
    DoModes();
    DoFeedbackTerms();
    DumpInfo();

	if (source)
		fclose(source);
	if (object && object != stdout)
		fclose(object);

//	return (errCount != 0);
    return 0;
}
