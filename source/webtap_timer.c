#include "webtap.h"

void Wait_Specific_Interval(struct OPTIONS option_list)
{
	// The different between the current and next processing times (seconds).
	unsigned int difference;
	time_t current_time;
	// Specifies the next time to run the image processing.
	time_t next_process_time;		
	// Allows us to determine the next value for next_process_time.
	struct tm * timestruct;

	/* Determine the value of next_process_time */
	// Baseline is the current time.
	time(&current_time);
	// Only way to access the minutes, seconds, hours, etc.
	timestruct = localtime(&current_time);

	DPRINTF("Current time is: Hours: %d\n", timestruct->tm_hour);
	DPRINTF("Minutes: %d\n", timestruct->tm_min);
	DPRINTF("Seconds: %d\n", timestruct->tm_sec);

	/* Analyze the current time so that we accurately choose next_process_time. */
	// Round to the nearest interval.
	// Note: Interval must be evenly divisble by 60.
	// Adding the wait tolerance means we can compare 0 to 0, instead of 60 - wait_tolerance.
	int output_time_min = 0;
	int output_time_hour = timestruct->tm_hour;
	int compare_time_min = timestruct->tm_min + option_list.wait_tolerance / 60;
	int compare_time_sec = timestruct->tm_sec + option_list.wait_tolerance % 60;
	if (compare_time_sec >= 60)
		compare_time_min++;
	DPRINTF("Original comparative time: %d\n", compare_time_min);
	if (compare_time_min >= 60) {
		output_time_hour = ( output_time_hour + compare_time_min / 60 ) % 24;
		printf("Output time (hour): %d.\n", output_time_hour);
		compare_time_min = compare_time_min % 60;
	}

	DPRINTF("Post->60 Check comparative time: %d\n", compare_time_min);
	// Calculate the next time.
	// time/itv*itv = last interval time.
	// last interval time + itv = next interval time.
	output_time_min = ( compare_time_min / option_list.wait_interval ) * 
					  option_list.wait_interval + option_list.wait_interval;
	if (output_time_min >= 60) {
		DPRINTF("Output time (hour) post->60 check: %d.\n", output_time_hour);
	}
	DPRINTF("Output time (minutes) is: %d\n", output_time_min);
	timestruct->tm_hour = output_time_hour;
	timestruct->tm_min = output_time_min;
	timestruct->tm_sec = 0;

	next_process_time = mktime(timestruct);
	DPRINTF("Target time is: Hours: %d\n", timestruct->tm_hour);
	DPRINTF("Minutes: %d\n", timestruct->tm_min);
	DPRINTF("Seconds: %d\n", timestruct->tm_sec);
	/* Sleep until we need to run the image processing. */
	do {
		time(&current_time);
		difference = (unsigned int)difftime(next_process_time, current_time);
		DPRINTF("Planning to sleep for %u.\n", difference - option_list.wait_tolerance);
		DPRINTF("Difference: %u.\n", difference);
		// The - tolerance is so we don't accidentally go over.
		sleep(difference - option_list.wait_tolerance); 
	} while (difference > option_list.wait_tolerance);

	// When this process returns, the next image processing function runs.
	time(&current_time);
	timestruct = localtime(&current_time);
	DPRINTF("Exited loop at:\n");
	DPRINTF("Hours: %d\n", timestruct->tm_hour);
	DPRINTF("Minutes: %d\n", timestruct->tm_min);
	DPRINTF("Seconds: %d\n", timestruct->tm_sec);
	return;
}