/***********************************************************************************
 * File: replaceAlgos.c
 * Author: Justin Hardy
 * Procedures:
 * main				- Program which performs Monte Carlo Simulations of various
 *						virtual memory replacement algorithm, and computes
 *						their average page faults on various set sizes.
 * LRU				- Performs the Least Recently Used replacement algorithm on
 * 						a given data set.
 * FIFO				- Performs the First-In-First-Out replacement algorithm on
 *						a given data set.
 * Clock			- Performs the Clock replacement algorithm on a given data
 *						set.
 * normal			- Generates a random number off of a normal distribution
 *						with a specified mean and standard deviation.
 * arrayContains	- Determines if the given array contains a given value.
 * getIndex			- Determines the index at which a given array contains a
 *						given value, or if an index does not exist for it.
 ***********************************************************************************/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/utsname.h>
#include <errno.h>
#include <math.h>
#include <time.h>
#include <limits.h>

// Simulation constants
#define TRACES		1000		// The number of traces to be performed
#define SET_SIZE_LOWER	4		// The lower bound of the set sizes to test
#define SET_SIZE_UPPER	20		// The upper bound of the set sizes to test

// Program functions - see below main for implementation and details!
// 	I'd like to note that I do it this way out of personal preference;
// 	I like main to be the first full function you see in the program.
// 	This isn't neccessary, since they're all default return type, but
// 	I'll include it since it's  generally good programming practice.
int LRU(int,int[]);					// Performs LRU Algorithm
int FIFO(int,int[]);				// Performs FIFO Algorithm
int Clock(int,int[]);				// Performs Clock Algorithm
int normal(int,int);				// Generates random number under normal distribution
int arrayContains(int[],int,int);	// Gets if an element is contained in an array
int getIndex(int[],int,int);		// Gets the index of an element in an array

/***********************************************************************************
 * int main( int argc, char* argv[] )
 * Author: Justin Hardy
 * Date: 19 November 2021
 * Description: Performs a Monte Carlo Simulation of virtual memory replacement
 *					algorithms by iteratively generating 1000 (by default)
 *					page number traces, acting as experiments, separated into
 *					10 regions. The algorithms are testing on varying working
 *					set sizes from 4 to 20 (by default) and computes the
 *					average of those 1000 experiements on those set sizes.
 *
 * Parameters:
 * 	argc	I/P	int			The number of arguments on the command line
 * 	argv	I/P	char *[]	The arguments on the command line
 * 	main	O/P	int			Status code (see in-line comments)
 ***********************************************************************************/
int main( int argc, char* argv[] ) {
	// Declare program variables
	int i, j, wss;
	// Declare program arrays
	int data[TRACES], LRUResults[SET_SIZE_UPPER+1], FIFOResults[SET_SIZE_UPPER+1], ClockResults[SET_SIZE_UPPER+1];
	
	// Fill arrays with empty data
	for( i = 0; i <= SET_SIZE_UPPER; i++ ) {
		LRUResults[i] = 0;		// LRU
		FIFOResults[i] = 0;		// FIFO
		ClockResults[i] = 0;	// Clock
	}

	// Set seed for random generator to current time
	srand(time(NULL));

	// Run experiments
	for( i = 0; i < TRACES; i++) {
		// Print progress message
		printf("Running traces... (%d/%d)%s", i+1, TRACES, ((i+1)%5 == 0 ? "\n" : "\t"));
		
		// Generate data
		for( j = 0; j < TRACES; j++ ) {
			// Generate a random set of numbers (mean 10, sd 2)
			data[j] = ( 10 * ((int)(j/100)) ) + normal(10, 2);
		}

		// Run monte carlo simulation
		for( wss = SET_SIZE_LOWER; wss <= SET_SIZE_UPPER; wss++ ) {
			// Accumulate # of page faults for each algorithm base on current wss and trace
			LRUResults[wss] += LRU(wss, data);		// LRU
			FIFOResults[wss] += FIFO(wss, data);	// FIFO
			ClockResults[wss] += Clock(wss, data);	// Clock
		}
	}

	// Get the average of the results
	for( wss = SET_SIZE_LOWER; wss <= SET_SIZE_UPPER; wss++ ) {
		LRUResults[wss] /= TRACES;		// LRU
		FIFOResults[wss] /= TRACES;		// FIFO
		ClockResults[wss] /= TRACES;	// Clock
	}
	
	// Get current time
	time_t r;
	time(&r);
	struct tm* time = localtime(&r);

	// Generate file name
	char fileName[32];
	strftime(fileName, sizeof(fileName), "Pgm3_%m-%d-%Y_%H:%M:%S.csv", time);	// This will generate a file name based off of
																				// the frame of time at the time of execution

	// Create file
	FILE  *file = fopen(fileName, "w");

	// Check if file was successfully opened
	if( file == NULL ) {
		// Print error message and exit program with error code
		printf("ERROR: Failed to create file %s\n", fileName);
		return -1;
	}

	// Output results header to file
	fprintf(file, "%s,%s,%s,%s\n", "wss", "LRU" , "FIFO", "Clock");

	// Output results to file
	for( wss = SET_SIZE_LOWER; wss <= SET_SIZE_UPPER; wss++ ) {
		// Output statistics
		fprintf(file, "%d,", wss);					// wss
		fprintf(file, "%d,", LRUResults[wss]);		// LRU
		fprintf(file, "%d,", FIFOResults[wss]);		// FIFO
		fprintf(file, "%d\n", ClockResults[wss]);	// Clock
	}
	
	// Close file
	fclose(file);
	
	// Exit program
	return 0;
}

/***********************************************************************************
 * int LRU( int wss, int data[] )
 * Author: Justin Hardy
 * Date: 19 November 2021
 * Description: Peforms the Least Recently Used virtual memory replacement algorithm
 * 					on a given data set, with a specified working set size to be
 * 					used. As the algorithm performs, it will count the number of
 * 					page faults that occur, and by the end of its execution,
 * 					return the number of page faults that had occurred throughout
 * 					its execution.
 *
 * Parameters:
 * 	wss		I/P	int			The working set size to be utitilized
 * 	data	I/P	int []		The data to perform the algorithm on
 * 	LRU		O/P	int			The number of page faults that occurred
 *								during the algorithm's execution.
 ***********************************************************************************/
int LRU( int wss, int data[] ) {
	// Create fault variable count, array, and array size.
	// Size keeps track of how much data is filling the array;
	// Empty cells are marked by INT_MIN.
	int faults = 0, size = 0;
	int set[wss];
	
	// Fill array with default values
	int i;
	for( i = 0; i < wss; i++ ) {
		set[i] = INT_MIN;
	}

	// Run LRU Algorithm on the array
	for( i = 0; i < TRACES; i++ ) {
		// Check if data is not present in the set
		if( !arrayContains(set, wss, data[i]) ) {
			// Check if set has room for more pages
			if( size != wss ) {
				// Insert data into set
				set[size] = data[i];

				// Increment size
				size++;
			}
			else {
				// Page fault has occurred
				// Determine least recently used page
				int j, c, lru;
				int ru[wss];

				// fill recently used array with default values
				for( j = 0; j < wss; j++ ) {
					ru[j] = INT_MIN;
				}

				for( j = i, c = 0; j >= 0; j-- ) {
					// Check if this element of the data exists in the set
					if( arrayContains(set, wss, data[j]) ) {
						// This element was recently used
						// If this page wasn't already accounted for
						if( !arrayContains(ru, wss, data[j]) ) {
							// add the data to ru
							ru[c] = data[j];
							
							// increment c 
							c++;
						}
						
						// Check if c is equal to wss
						if( c == wss ) {
							// This element is LRU.
							lru = data[j];
							// Break out of loop
							break;
						}
					}
					// At the worst case, the loop will go to the beginning of the array,
					// but will always get c to equal wss at some point. Break will always
					// be called at some point in this loop.
				}
				
				// Replace least recently used page with new data value
				c = getIndex(set, wss, lru);
				set[c] = data[i];
			
				// Increment page faults counter
				faults++;
			}
		}
	}

	// Return fault count
	return faults;
}

/***********************************************************************************
 * int FIFO( int wss, int data[] )
 * Author: Justin Hardy
 * Date: 19 November 2021
 * Description: Performs the First-In-First-Out virtual memory replacement algorithm
 * 					on a given data set, with a specified working set size to be
 * 					used. As the algorithm performs, it will count the number of
 * 					page faults that occur, and by the end of its execution,
 * 					return the number of page faults that had occurred throughout
 * 					its execution.
 *
 * Parameters:
 * 	wss		I/P	int		The working set size to be utitilized
 * 	data	I/P	int []	The data to perform the algorithm on
 * 	FIFO	O/P	int		The number of page faults that occurred
 *							during the algorithm's execution.
 ***********************************************************************************/
int FIFO( int wss, int data[] ) {
	// Create fault count variable & array
	int faults = 0, size = 0;
	int set[wss];

	//  Fill array with default values
	int i;
	for( i = 0; i < wss; i++ ) {
		set[i] = INT_MIN;
	}

	// Run FIFO Algorithm on the array
	int fifoIndex = 0;
	for( i = 0; i < TRACES; i++ ) {
		// Check if data is not present in the set 
		if( !arrayContains(set, wss, data[i]) ) {
			// Check if set has room for more pages
			if( size != wss ) {
				// Add data to set
				set[size] = data[i];

				// Increment size
				size++;
			}
			else {
				// Page fault has occurred
				// Replace using first-in-first-out index
				set[fifoIndex] = data[i];

				// Increment first-in-first-out index
				fifoIndex++;

				// Wrap first-in-first-out index around wss if applicable
				if( fifoIndex == wss ) {
					fifoIndex = 0;
				}

				// Increment page faults counter
				faults++;
			}
		}
	}

	// Return fault count
	return faults;
}

/***********************************************************************************
 * int Clock( int wss, int data[] )
 * Author: Justin Hardy
 * Date: 19 November 2021
 * Description: Performs the Clock virtual memory replacement algorithm on a given
 * 					data set, with a specified working set size to be used. As
 * 					the algorithm performs, it will count the number of page
 * 					faults that occur, and by the end of its execution, return
 * 					the number of page faults that had occurred throughout its
 * 					execution.
 *
 * Parameters:
 * 	wss		I/P	int		The working set size to be utitilized
 * 	data	I/P	int []	The data to perform the algorithm on
 * 	Clock	O/P	int		The number of page faults that occurred
 *						during the algorithm's execution.
 ***********************************************************************************/
int Clock( int wss, int data[] ) {
	// Create faults count variable & array
	int faults = 0, size = 0;
	int set[wss], secondChance[wss];

	// Fill arrays with default values
	int i;
	for( i = 0; i < wss; i++ ) {
		set[i] = INT_MIN;
		secondChance[i] = 0; // second chance bits start at 0 
	}

	// Run Clock Algorithm on the array
	int fifoIndex = 0;
	for( i = 0; i < TRACES; i++ ) {
		// Check if data is not present in the set 
		if( !arrayContains(set, wss, data[i]) ) {
			// Check if set has room for more pages
			if( size != wss ) {
				// Add data to set
				set[size] = data[i];

				// Increment size
				size++;
			}
			else {
				// Page fault has occurred
				// Determine which index needs to be replaced
				while( secondChance[fifoIndex] != 0 ) {
					// Remove the element's second chance use bit
					secondChance[fifoIndex] = 0;
					
					// Increment first-in-first-out index
					fifoIndex++;

					// Wrap first-in-first-out index around wss if applicable
					if( fifoIndex == wss ) {
						fifoIndex = 0;
					}
				}					

				// Replace using first-in-first-out index
				set[fifoIndex] = data[i];

				// Increment first-in-first-out index			
				fifoIndex++;
				
				// Wrap first-in-first-out index around wss if applicable
				if( fifoIndex == wss ) {
					fifoIndex = 0;
				}

				// Increment page faults counter
				faults++;
			}
		}
		else {
			// Set second chance bit of the data in the set to 1.
			secondChance[ getIndex(set, wss, data[i]) ] = 1;
		}
	}
	
	// Return fault count
	return faults;
}

/***********************************************************************************
 * int normal( int mean, int sd )
 * Author: Justin Hardy
 * Date: 19 November 2021
 * Description: Generates a normal, random number distribution with a specified mean
 * 					and standard deviation.
 *
 * Parameters:
 * 	mean	I/P	int		The mean of the normal distribution.
 * 	sd		I/P	int		The standard deviation of the normal
 *							distribution.
 * 	normal	O/P	int		A randomly generated number within the
 *							normal distribution.
 ***********************************************************************************/
int normal( int mean, int sd ) {
	// Generate random number 1
	double r1;
	do {
		// Make sure the random number is not 0!!
		// Otherwise the program crashes :(
		r1 = ((double)rand()) / ((double)RAND_MAX);
	} while(r1 == 0.0);
	
	// Generate random number 2
	double r2 = ((double)rand()) / ((double)RAND_MAX);
	
	// Use normal distribution formula to generate random number
	return (int) ((( sqrt(-2.0 * log(r1) ) * cos(2.0 * M_PI * r2 )) * sd) + mean);
}

/***********************************************************************************
 * int arrayContains( int array[], int size, int value )
 * Author: Justin Hardy
 * Date: 19 November 2021
 * Description: Determines if an array of a particular size contains a specified
 * 					value. Returns 1 (for true) if the array contains the said
 * 					value, and 0 (for false) if the array does not contain the
 * 					value.
 *
 * Parameters:
 * 	array			I/P	int []	The array to be searched through.
 * 	size			I/P	int		The size of the array.
 * 	value			I/P	int		The value to be found in the array.
 * 	arrayContains	O/P	int		1 if array contains value, 0 if not.
 ***********************************************************************************/
int arrayContains( int array[], int size, int value ) {
	// Basic array contains() function, since C doesn't offer one for arrays..
	int i;
	for( i = 0; i < size; i++ ) {
		if( value == array[i] ) {
			return 1;
		}
	}
	return 0;
}

/***********************************************************************************
 * int arrayContains( int array[], int size, int value )
 * Author: Justin Hardy
 * Date: 19 November 2021
 * Description: Determines the index at which an array contains a specified value. 
 * 					Returns the index at which the value can be first found in
 * 					the array, or -1 if the value does not exist within the
 * 					array.
 *
 * Parameters:
 * 	array		I/P	int []	The array to be searched through.
 * 	size		I/P	int		The size of the array.
 * 	value		I/P	int		The value to be found in the array.
 * 	getIndex	O/P	int		The index of value, -1 if value does
 *							not exist in array.
 ***********************************************************************************/
int getIndex( int array[], int size, int value ) {
	// Basic array find() function, since C doesn't offer one for arrays..
	int i;
	for( i = 0; i < size; i++ ) {
		if( array[i] == value ) {
			return i;
		}
	}
	return -1;
}
