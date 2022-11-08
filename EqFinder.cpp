/*

For help, run like this:
eqfinder help

*/


#include "stdlib.h"
#include "stdio.h"
#include "string.h"
#include "math.h"
#include "signal.h"
#include "float.h"
#include "setjmp.h"
#include "stdarg.h"
#include "windows.h"


/* If your compiler doesn't take long doubles, then make this datatype
the one with the biggest exponent your compiler can handle (try
'double', then just 'float') */

#define BIGFLOAT double

/* The program doesn't care about the actual meanings of meters, kilograms,
seconds, and amperes. It just considers them dimensions of a vector.
If you ever want to change how many units the program handles, change the NUMUNITS
symbol and the UnitNames array.
*/
#define NUMUNITS 4

/* here are the names of the units */
const char *UnitNames[]=
{
   "Meters",
   "Kilograms",
   "Seconds",
   "Amperes"
};

/* ResultStat is a global set by functions that solve the test equation.
It indicates the status of the current equation.
*/
#define GOOD     0 // Equation was successfully resolved
#define NOTAVAIL 1 // Equation has not yet been resolved, so we don't know what its status is
#define OVER_FLOW 2 // We got an overflow trying to solve the current equation


// This is the maximum number of constants the user can specify
#define MAXCONSTANTS 100


// This structure holds everything about a constant (i.e. Vector).
class Vector
{
public:
	char     Symbol[18];            /* Variable name of this vector, if any    */
	char     Description[100];       /* Full description of this vector, if any */
	float	Magnitude;         /* Vector's magnitude                      */
	int       Unit[NUMUNITS];    /* Holds the powers of the SI base units   */
};


class EqFinder
{
public:
	// File constants are loaded into this array
	Vector AllConstants[MAXCONSTANTS+1];

	// Array of constants. (specifically, array of pointers to the constants
	// Constants from user's file are loaded into the AllConstants array, then
	// the this Constant array is loaded with pointers to the elements of the AllConstants array
	// indicating which constants are to be used in our current search session 
	Vector *Constant[MAXCONSTANTS];


	/* Indicates current status of the result magnitude*/
	__int64 ResultStat;

	// current number of constants we're working with (number of constants in the Constant array)
	__int64 NumConstants;

	__int64 Status;

	__int64 StartEquationNum;
	__int64 StopEquationNum;

	// list of current exponents being tried
	__int64 Exponent[MAXCONSTANTS];

	/* Magnitude result of current equation */
	BIGFLOAT ResultMag;

	/* Range of exponents to try */
	__int64 MinExponent;
	__int64 MaxExponent;

	// number of possible values for a single exponent
	__int64 ExpRange;

	// Criteria for considering the equation a 'hit'
	BIGFLOAT Range;
	__int64 AnyRange;

	/* How often to print the "." and check for kbhit while thinking */
	long PetRate;

	// total possible number of equations
	__int64 NumEquations;

	// Current equation number
	__int64 CurrentEquationNum;

	void Init(double StartFraction=0.,double StopFraction=1.);
	void Run(void);
	void DoCarriesOnExponentPattern(__int64 ExpNum=0);
	void ResetEquation(__int64 StartEqNum);
	void LoadConstants(void);
	void DeleteVector(Vector *V);
	void InsertVector(Vector *PrevV,Vector *NewV);
	__int64  FindEquation(void);
	void DisplayEquation(void);
	__int64  IncExp(int n);
	__int64  NextEquation(void);
	__int64  GoodEquation(void);
	void ShowParameters(void);
};

void Fclose(FILE **FilePtr);
void Print(const char *Format,...);
void Fopen(FILE **FilePtr,char *Filename,char *Mode);


// Safely open a file
void Fopen(FILE **FilePtr,char *FileName,char *Mode)
{
	fopen_s(FilePtr,FileName,Mode);

	if(!*FilePtr)
	{
		Print("ERROR: Couldn't open %s for %s mode",FileName,Mode);
	}
}

// Safely close a file
void Fclose(FILE **FilePtr)
{
	fclose(*FilePtr);
	*FilePtr=NULL;
}

// Math error handlers
#ifdef __TURBOC__
void  SigFPE(int sig, int type, int *reglist);
#else
void  SigFPE(int sig);
#endif

void Usage(void)
{
puts(
"This program takes a list of constants specified by the user\n"
"and finds any valid equations relating those constants.\n"
"\n"
"The units (dimensions) of each constant (meters, kilograms, amperes,\n"
"seconds) are honored when testing equations for validity.\n"
"\n"
"That is, the program will try many possible equations containing the\n"
"constants specified by the user, but display only those equations in\n"
"which the units of the constants all cancel.\n"
"\n"
"The program tries equations of the form:\n"
"\n"
"K1^E1  *  K2^E2  *  K3^E3 = unitless magnitude?\n"
"\n"
"Where\n"
"Kn    is a constant\n"
"En    is an integral exponent.\n"
"\n"
"For example, if the user gives the program the permittivity (p) and\n"
"permeability (u) of free space, and the speed of light (c), and\n"
"the user specifies that exponents should be tried from -2 to 2,\n"
"then the program will test the following equations for validity:\n"
"\n"
"p^-2  *  u^-2  *  c^-2    =   unitless magnitude?\n"
"p^-2  *  u^-2  *  c^-1    =   unitless magnitude?\n"
"p^-2  *  u^-2  *  c^0     =   unitless magnitude?\n"
"p^-2  *  u^-2  *  c^1     =   unitless magnitude?\n"
"p^-2  *  u^-2  *  c^2     =   unitless magnitude?\n"
"p^-2  *  u^-1  *  c^-2    =   unitless magnitude?\n"
"p^-2  *  u^-1  *  c^-1    =   unitless magnitude?\n"
"--etc--\n"
"\n"
"\n"
"Eventually the program will discover Maxwells solution for the speed of light:\n"
"\n"
"\n"
"p^-1  *  u^-1  *  c^2    ==   1.000 (no units)\n"
"\n"
"\n"
".. and display it for the user.\n"
"\n"
"The user can also specify the target magnitude range (how close the resulting magnitude \n"
"is to unity), so that equations resulting in a magnitude outside this range are not\n"
"displayed.\n"
"\n"
"\n"
"Kelly Loum, 1989\n"
"\n"
);

}


//FILE *OutFile;


// Program entry point
void main(int ArgC,char **ArgV)
{
	if(ArgC>1)
	{
		Usage();
		exit(1);
	}

	printf("For help screen, run like this:\neqfinder help\n\n");

	// open the output file that we print 'hits' to
//	Fopen(&OutFile,"eq.txt","w");

	// setup math error handler
	signal(SIGFPE,SigFPE);

//	Print("\n\nEquation Finder\n(screen output also goes to eq.txt)\n");

	EqFinder *e[16];

	for(int n=0;n<16;n++)
	{
		e[n]=new EqFinder();
		e[n]->Init(n/16.,n/16.+1/16.);
	}

	int NumDone=0;
	while(NumDone < 16)
	{
		NumDone=0;
		for(int n=0;n<16;n++)
		{
			if(e[n]->Status)
				NumDone++;
		}
		Sleep(50);
	}

	printf("\n\nFinished. Press Enter to quit.\n");

	getchar();
}


DWORD WINAPI EqFinderThreadProc(void *Context)
{
	EqFinder *This=(EqFinder *)Context;
	This->Run();
	return 0;
}


void EqFinder::Init(double StartFraction,double StopFraction)
{
	Status=0;
	Constant[0]=NULL;
	NumConstants=0;
	ResultMag=-1.;
	ResultStat=NOTAVAIL;

	MinExponent=-6;
	MaxExponent=6;

	// Criteria for considering the equation a 'hit'
	Range=100.;
	AnyRange=0;

	/* How often to print the "." and check for kbhit while thinking */
	PetRate=2000000;

	// number of equations to try
	NumEquations=0;

	// Load the constants.txt file, and also point elements of the Constant array
	// to each of them.
	LoadConstants();

	ExpRange=MaxExponent*2+1;

	StartEquationNum=(__int64)(NumEquations*StartFraction);
	StopEquationNum=(__int64)(NumEquations*StopFraction);

	// Start with first equation
	ResetEquation(StartEquationNum);

	unsigned long ID;
	CreateThread(NULL,0,EqFinderThreadProc,this,0,&ID);
}

void EqFinder::Run(void)
{
	Print("Thread %8.8X started\n",GetCurrentThreadId());

	// Search equations and print any hits
	while(FindEquation())
	{
//		Print("\n%%%%%%%%%% HIT:\n");
		DisplayEquation();
		NextEquation();
	}
//	Fclose(&OutFile);

	Status=1;

}


// Display the constants & criteria we're using
// Typically done at the beginning of a new search session
void EqFinder::ShowParameters(void)
{
	__int64 n;

	Print("MaxExponent=%d\n",MaxExponent);
	Print("MagnitudeDeviation=%f\n",Range);
	Print("\n");
	Print("SYM MAGNITUDE     M  Kg S  A DESCRIPTION:\n");

	for(n=0;n<NumConstants;n++)
	{
		Print("%3.3s %1.5e % d % d % d % d %s\n",
		Constant[n]->Symbol,
		Constant[n]->Magnitude,
		Constant[n]->Unit[0],
		Constant[n]->Unit[1],
		Constant[n]->Unit[2],
		Constant[n]->Unit[3],
		Constant[n]->Description
		);
	}
}

// Iterate all test equations and return true when we've found a hit. return zero when done.
__int64 EqFinder::FindEquation(void)
{
	long Pet;

	Pet=PetRate;

	do
	{

		if(--Pet < 1)
		{
			double Percent=100.*(double)(CurrentEquationNum-StartEquationNum)/(double)(StopEquationNum-StartEquationNum);
			//printf("%2.7f%% complete                 \r",Percent);

			//DisplayEquation();
/*
			long n;
			printf(" ");
			for(n=0;n<NumConstants;n++)
			{
				Print("%3d ",Exponent[n]);
			}
			printf("      \r");
*/
			Pet=PetRate;
		}

		if(GoodEquation())
			return 1;
/*
I think this may have hidden some good equations--don't know why -KGL
		// The following block is a speed up. it can be removed if you like, from here...
		ResultStat=NOTAVAIL;
		if(Exponent[0] < MaxExponent)
		{
			Exponent[0]++;
			continue;
		}
		// ...to here.
*/
	}
	while(NextEquation());
	return 0;
}

// Solve the current equation and return whether it is a hit
__int64 EqFinder::GoodEquation(void)
{
	__int64 n,u;
	__int64 Unit[4]={0,0,0,0};
	__int64 ThisExponent;
	__int64 UnitPower;

	for(u=0;u<4;u++)
	{
		for(n=0;n<NumConstants;n++)
		{
			if((UnitPower=Constant[n]->Unit[u]) && (ThisExponent=Exponent[n]))
				Unit[u]+=UnitPower * ThisExponent;
		}

		// abort if any of the powers is non-zero
		if(Unit[u])
			break;
	}

	if(u==4)  // if all four powers were zero
	{
		BIGFLOAT ResultLog=0;

		ResultStat=GOOD;

		for(n=0;n<NumConstants;n++)
		{
			ResultLog+=log(Constant[n]->Magnitude)*Exponent[n];
		}

		ResultMag=exp(ResultLog);

		if(AnyRange || (ResultStat==GOOD && (ResultMag > (1/Range)) && (ResultMag < Range)))
			return 1;
	}

	return 0;
}

// Change current equation to the next equation to try
__int64 EqFinder::NextEquation(void)
{
	CurrentEquationNum++;
	ResultStat=NOTAVAIL;
	return IncExp(0);
}

// Increment the right exponent with carry to left exponents
__int64 EqFinder::IncExp(int n)
{
	if(Exponent[n] >= MaxExponent)
	{
		Exponent[n]=MinExponent;

		return IncExp(n+1);
	}
	Exponent[n]++;

	CurrentEquationNum++;

	if(CurrentEquationNum >= StopEquationNum)
		return 0;

	return 1;
}


// Show user the current equation
void EqFinder::DisplayEquation(void)
{
	__int64 n=0;

	char s[2000]="\n";

	for(n=0;n<NumConstants;n++)
	{
		sprintf(s+strlen(s),"%3d ",Exponent[n]);
	}

	sprintf(s+strlen(s),"\n");

	for(n=0;n<NumConstants;n++)
	{
		sprintf(s+strlen(s),"%3.3s ",Constant[n]->Symbol);
	}

	if(ResultStat != NOTAVAIL)
	{
		if(ResultStat == OVER_FLOW)
			sprintf(s+strlen(s)," = (OVER_FLOW)\n");
		else
			sprintf(s+strlen(s)," = %g\n",ResultMag);
	}
	Print("%s",s);
}


void EqFinder::DoCarriesOnExponentPattern(__int64 ExpNum)
{
	__int64 ThisExpVal=Exponent[ExpNum]+MaxExponent;
	__int64 IntegerQuotient=ThisExpVal / ExpRange;
	__int64 Remainder=ThisExpVal % ExpRange;

	Exponent[ExpNum]=Remainder-MaxExponent;

	ExpNum++;
	if(ExpNum < NumConstants)
	{
		Exponent[ExpNum]+=IntegerQuotient;
		if(Exponent[ExpNum] > MaxExponent)
			DoCarriesOnExponentPattern(ExpNum);
	}
}



// Set equation to first equation to try
void EqFinder::ResetEquation(__int64 StartEqNum)
{
	__int64 n=0;

	Exponent[0]=0;

	for (n=1;n<NumConstants;n++)
	{
		Exponent[n]=MinExponent;
	}

	Exponent[0]+=StartEqNum;

	DoCarriesOnExponentPattern();

	CurrentEquationNum=StartEqNum;
}

// Load the constants.txt file, and also point elements of the Constant array
// to each of them.
void EqFinder::LoadConstants(void)
{
	FILE *InFile=NULL;
	__int64 n;
	char Keyword[500];

	fopen_s(&InFile,"eqfinder.txt","r");

	if(!InFile)
	{
		printf("ERROR: could not open eqfinder.txt.\n");
		exit(1);
	}
	else
	{
		for(n=0;n<MAXCONSTANTS;n++)
		{
			strcpy_s(AllConstants[n].Symbol,16,"END");

			if(0>=fscanf_s(InFile,"%s",Keyword,490))
				break;

			if(strpbrk(Keyword,"; \t\n") || Keyword[0]==0)
			{
				fgets(Keyword,500,InFile);
				n--;
				continue;
			}

			if(!_strnicmp(Keyword,"MaxExponent",11))
			{
				MaxExponent=strtoul(Keyword+12,NULL,10);
				MinExponent=-MaxExponent;

				fgets(Keyword,500,InFile);
				
				n--;
				continue;
			}

			if(!_strnicmp(Keyword,"MagnitudeDeviation",18))
			{
				Range=atof(Keyword+19);

				fgets(Keyword,500,InFile);
				
				n--;
				continue;
			}

			if
			(
				0>=fscanf_s(
				InFile,
				"%e %d %d %d %d %[^\n]\n",
				&AllConstants[n].Magnitude,
				&AllConstants[n].Unit[0],
				&AllConstants[n].Unit[1],
				&AllConstants[n].Unit[2],
				&AllConstants[n].Unit[3],
				AllConstants[n].Description,98)
			)
				break;

			strncpy_s(AllConstants[n].Symbol,16,Keyword,3);
		}

		fclose(InFile);
	}

	NumConstants=0;

	while(_stricmp("END",AllConstants[NumConstants].Symbol))
	{
		Constant[NumConstants]=&AllConstants[NumConstants];
		NumConstants++;
	}

	NumEquations=(unsigned __int64)(pow((double)(2*MaxExponent+1),double(NumConstants-1))*(MaxExponent+1));
}

// Output text
void Print(const char *Format, ...)
{
	va_list Arg;

	va_start(Arg,Format);

	vprintf(Format,Arg);

/*
	if(OutFile)
	{
		vfprintf(OutFile,Format,Arg);
	}
	fflush(OutFile);
*/
}


// Error handler

#ifdef __TURBOC__
int matherr()
#else
int matherr(struct _exception*t)
#endif
{
//	ResultStat=OVER_FLOW;
	return 1;
}


#ifdef __TURBOC__
void  SigFPE(int sig, int type, int *reglist)
#else
void  SigFPE(int sig)
#endif
{
//	ResultStat=OVER_FLOW;
	signal(SIGFPE,SigFPE);
}











