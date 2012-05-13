#include "webtap.h"

void Wait_Specific_Interval(struct OPTIONS optionList)
{
	// The different between the current and next processing times (seconds).
	unsigned int difference;
	time_t currentTime;
	// Specifies the next time to run the image processing.
	time_t nextProcessTime;		
	// Allows us to determine the next value for nextProcessTime.
	struct tm * timestruct;

	/* Determine the value of nextProcessTime */
	// Baseline is the current time.
	time(&currentTime);
	// Only way to access the minutes, seconds, hours, etc.
	timestruct = localtime(&currentTime);

	DPRINTF("Current time is: Hours: %d\n", timestruct->tm_hour);
	DPRINTF("Minutes: %d\n", timestruct->tm_min);
	DPRINTF("Seconds: %d\n", timestruct->tm_sec);

	/* Analyze the current time so that we accurately choose nextProcessTime. */
	// Adjust the hours properly to account for large waitIntervals
	//for( int i = 0; i < (timestruct->tm_min + optionList.waitInterval) / 60; i++) // Integer division
	//	timestruct->tm_hour++;
	// Round to the nearest interval.
	// Note: Interval must be evenly divisble by 60.
	// Adding the wait tolerance means we can compare 0 to 0, instead of 60 - waitTolerance.
	int outputTimeMin = 0;
	int outputTimeHour = timestruct->tm_hour;
	int compareTimeMin = timestruct->tm_min + optionList.waitTolerance / 60;
	int compareTimeSec = timestruct->tm_sec + optionList.waitTolerance % 60;
	if( compareTimeSec >= 60 )
		compareTimeMin++;
	DPRINTF("Original comparative time: %d\n", compareTimeMin);
	if( compareTimeMin >= 60 ) {
		outputTimeHour = ( outputTimeHour + compareTimeMin / 60 ) % 24;
		printf("Output time (hour): %d.\n", outputTimeHour);
		compareTimeMin = compareTimeMin % 60;
	}

	DPRINTF("Post->60 Check comparative time: %d\n", compareTimeMin);
	// Calculate the next time.
	// time/itv*itv = last interval time.
	// last interval time + itv = next interval time.
	outputTimeMin = ( compareTimeMin / optionList.waitInterval ) * 
					  optionList.waitInterval + optionList.waitInterval;
	if( outputTimeMin >= 60 ) {
		//outputTimeHour++;
		printf("Output time (hour) post->60 check: %d.\n", outputTimeHour);
	}
	DPRINTF("Output time (minutes) is: %d\n", outputTimeMin);
	timestruct->tm_hour = outputTimeHour;
	timestruct->tm_min = outputTimeMin;
	timestruct->tm_sec = 0;

	nextProcessTime = mktime(timestruct);
	DPRINTF("Target time is: Hours: %d\n", timestruct->tm_hour);
	DPRINTF("Minutes: %d\n", timestruct->tm_min);
	DPRINTF("Seconds: %d\n", timestruct->tm_sec);
	/* Sleep until we need to run the image processing. */
	do {
		time(&currentTime);
		difference = (unsigned int)difftime(nextProcessTime, currentTime);
		DPRINTF("Planning to sleep for %u.\n", difference - optionList.waitTolerance);
		DPRINTF("Difference: %u.\n", difference);
		// The - tolerance is so we don't accidentally go over.
		sleep(difference - optionList.waitTolerance); 
	} while (difference > optionList.waitTolerance);

	// When this process returns, the next image processing function runs.
	time(&currentTime);
	timestruct = localtime(&currentTime);
	DPRINTF("Exited loop at:\n");
	DPRINTF("Hours: %d\n", timestruct->tm_hour);
	DPRINTF("Minutes: %d\n", timestruct->tm_min);
	DPRINTF("Seconds: %d\n", timestruct->tm_sec);
	return;
}