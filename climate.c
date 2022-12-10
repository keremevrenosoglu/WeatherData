/**
 * climate.c
 *
 * Performs analysis on climate data provided by the
 * National Oceanic and Atmospheric Administration (NOAA).
 *
 * Input:    Tab-delimited file(s) to analyze.
 * Output:   Summary information about the data.
 *
 *
 * Example Run:      ./climate data_tn.tdv data_wa.tdv
 *
 *
 * Opening file: data_tn.tdv
 * Opening file: data_wa.tdv
 * States found: TN WA
 * -- State: TN --
 * Number of Records: 17097
 * Average Humidity: 49.4%
 * Average Temperature: 58.3F
 * Max Temperature: 110.4F 
 * Max Temperatuer on: Mon Aug  3 11:00:00 2015
 * Min Temperature: -11.1F
 * Min Temperature on: Fri Feb 20 04:00:00 2015
 * Lightning Strikes: 781
 * Records with Snow Cover: 107
 * Average Cloud Cover: 53.0%
 * -- State: WA --
 * Number of Records: 48357
 * Average Humidity: 61.3%
 * Average Temperature: 52.9F
 * Max Temperature: 125.7F
 * Max Temperature on: Sun Jun 28 17:00:00 2015
 * Min Temperature: -18.7F 
 * Min Temperature on: Wed Dec 30 04:00:00 2015
 * Lightning Strikes: 1190
 * Records with Snow Cover: 1383
 * Average Cloud Cover: 54.5%
 *
 * TDV format:
 *
 * CA» 1428300000000»  9prcjqk3yc80»   93.0»   0.0»100.0»  0.0»95644.0»277.58716
 * CA» 1430308800000»  9prc9sgwvw80»   4.0»0.0»100.0»  0.0»99226.0»282.63037
 * CA» 1428559200000»  9prrremmdqxb»   61.0»   0.0»0.0»0.0»102112.0»   285.07513
 * CA» 1428192000000»  9prkzkcdypgz»   57.0»   0.0»100.0»  0.0»101765.0» 285.21332
 * CA» 1428170400000»  9prdd41tbzeb»   73.0»   0.0»22.0»   0.0»102074.0» 285.10425
 * CA» 1429768800000»  9pr60tz83r2p»   38.0»   0.0»0.0»0.0»101679.0»   283.9342
 * CA» 1428127200000»  9prj93myxe80»   98.0»   0.0»100.0»  0.0»102343.0» 285.75
 * CA» 1428408000000»  9pr49b49zs7z»   93.0»   0.0»100.0»  0.0»100645.0» 285.82413
 *
 * Each field is separated by a tab character \t and ends with a newline \n.
 *
 * Fields:
 *      state code (e.g., CA, TX, etc),
 *      timestamp (time of observation as a UNIX timestamp),
 *      geolocation (geohash string),
 *      humidity (0 - 100%),
 *      snow (1 = snow present, 0 = no snow),
 *      cloud cover (0 - 100%),
 *      lightning strikes (1 = lightning strike, 0 = no lightning),
 *      pressure (Pa),
 *      surface temperature (Kelvin)
 */

#include <float.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define NUM_STATES 50

/**
 * climate_info structs set up to hold values that will be needed to 
 * for the report. Types dependent on what is necessary to hold their
 * respective data.
 */
struct climate_info {
    char code[3];
    unsigned long num_records;
    long double sum_temperature;
    long double sum_humidity;
    unsigned long snow_records;
    long double sum_cloud_cover;
    unsigned long lightning_strikes; 
    float max_temperature;
    float min_temperature;
    long max_timestamp;
    long min_timestamp;
};

void analyze_file(FILE *file, struct climate_info *states[], int num_states);
void print_report(struct climate_info *states[], int num_states);

/**
 * main is meant to take arguments of file names.
 */
int main(int argc, char *argv[]) {

    /**
     * If no arguments were passed beyond the name of the program, a message
     * will be provided and the program will terminate.
     */

    if (argc < 2) {
        printf("Usage: %s tdv_file1 tdv_file2 ... tdv_fileN \n", argv[0]);
        return EXIT_FAILURE;
    }

    /* Let's create an array to store our state data in. As we know, there are
     * 50 US states. */
    struct climate_info *states[NUM_STATES] = { NULL };
    int i;

    /**
     * Loop will run for each file name that was found. Attempts to open the file,
     * if it is not found, an error message is outputted and it moves on to the 
     * next one. Otherwise, the file is sent through analyze_value to collect
     * its data and closed afterward.
     */
    for (i = 1; i < argc; ++i) {
	FILE *file = fopen(argv[i], "r");
	if (file == NULL) {
	    printf("ERROR: %s does not exist\n", argv[i]);
	    continue;
	}
	printf("Opening file: %s\n", argv[i]);
        analyze_file(file, states, NUM_STATES); 
	fclose(file);
    }

    /**
     * If no files were read from (first entry of the array is still null), 
     * nothing further needs to be done. In most cases though, print_report 
     * will be called in order to output the statistics.
     */
    if (*states != NULL) {
        print_report(states, NUM_STATES);
    }

    return 0;
}

/**
 * analyze_file works with the passed file to fill appropriate members of
 * the structs of the array. *token will be used to initially tokenize the
 * current line of the file, timestamp will hold the time value of the 
 * current line, and temperature carries the current temperature. delim has
 * the points strtok() will need to split each line.
 */
void analyze_file(FILE *file, struct climate_info **states, int num_states) {
    const int line_sz = 100;
    char line[line_sz];
    char *token;
    char delim[] = "\t\n";
    long timestamp;
    float temperature;
	
    /**
     * Loop here stores retrieves and stores each line one at a time 
     * until the end of the file has been reached.
     */
    while (fgets(line, line_sz, file) != NULL) {
        /* 
         * For the process for each line, to start, the line is tokenized,
	 * getting the state code first. A for loop is then ran to check
	 * through the state of the matching code already has data stored
	 * in a struct. If not, memory is allocated for one. Program exits
	 * if this fails. This struct then has all its members set to 
	 * the ones of the current line and the function can move on to the
	 * next line. If there was already a struct for the current state,
	 * values are updated including max and mins if it makes sense in the 
	 * given case. Can then move on to the next line.
         */
	token = strtok(line, delim);
	for (int i = 0; i < num_states; ++i) {
	    if (*(states + i) == NULL) {
	        *(states + i)  = malloc(sizeof(struct climate_info));

		if (*(states + i) == NULL) {
		    printf("ERROR: Memory could not be allocated\n");
		    exit(EXIT_FAILURE);
		}

		strcpy((*(states + i))->code, token);
		(*(states + i))->num_records = 1; 
		timestamp = atoll(strtok(NULL, delim)) / 1000;
		(*(states + i))->max_timestamp = timestamp;
                (*(states + i))->min_timestamp = timestamp;
		strtok(NULL, delim);
		(*(states + i))->sum_humidity = atof(strtok(NULL, delim));
		(*(states + i))->snow_records = atoi(strtok(NULL, delim));		
	    	(*(states + i))->sum_cloud_cover = atof(strtok(NULL, delim));
		(*(states + i))->lightning_strikes = atoi(strtok(NULL, delim));	
	    	strtok(NULL, delim);
		temperature = atof(strtok(NULL, delim)) * 1.8 - 459.67;
		(*(states + i))->sum_temperature = temperature;
		(*(states + i))->max_temperature = temperature;
		(*(states + i))->min_temperature = temperature;
		break;
			
	    }
	
	    if (strcmp((*(states + i))->code, token) == 0) {
		((*(states + i))->num_records)++;
		timestamp = atoll(strtok(NULL, delim)) / 1000;
		strtok(NULL, delim);
		(*(states + i))->sum_humidity += atof(strtok(NULL, delim));
		(*(states + i))->snow_records += atoi(strtok(NULL, delim));
		(*(states + i))->sum_cloud_cover += atof(strtok(NULL, delim));
		(*(states + i))->lightning_strikes += atoi(strtok(NULL, delim));
		strtok(NULL, delim);
		temperature = atof(strtok(NULL, delim)) * 1.8 - 459.67;
		(*(states + i))->sum_temperature += temperature;
		
		if (temperature > (*(states + i))->max_temperature) {
		    (*(states + i))->max_temperature = temperature;
	    	    (*(states + i))->max_timestamp = timestamp;
		}
		if (temperature < (*(states + i))->min_temperature) {
		    (*(states + i))->min_temperature = temperature;
                    (*(states + i))->min_timestamp = timestamp;
		}
		
		break;
	    }
	}

    }
}

/**
 * print_report handles all of the ouput for the program starting by
 * giving the codes of the states that were found in the files. A for
 * loop that goes through non-NULL members of the array to is utilized
 * for this.
 */
void print_report(struct climate_info *states[], int num_states) {
    printf("States found: ");
    int i;
    for (i = 0; i < num_states; ++i) {
        if (states[i] != NULL) {
            struct climate_info *info = states[i];
            printf("%s ", info->code);
        }
    }
    printf("\n");

    /**
     * Another similarly incorporated for loop is used here but to
     * to print the all of the information for each state that was collected.
     * Averages are calculated using totals divided by number of records.
     * Floats/doubles go to one decimal point
     */
    for (i = 0; i < num_states; ++i) {
        if (states[i] != NULL) {
	    printf("-- State: %s --\n", states[i]->code);
    	    printf("Number of Records: %ld\n", states[i]->num_records);
	    printf("Average Humidity: %0.1f%%\n",(double) (states[i]->sum_humidity / states[i]->num_records));
	    printf("Average Temperature: %0.1fF\n",(double) (states[i]->sum_temperature / states[i]->num_records));
	    printf("Max Temperature: %0.1fF\n",(double) states[i]->max_temperature);
	    printf("Max Temperature on: %s", ctime(&(states[i]->max_timestamp)));
	    printf("Min Temperature: %0.1fF\n",(double) states[i]->min_temperature);
            printf("Min Temperature on: %s", ctime(&(states[i]->min_timestamp)));	    
	    printf("Lightning Strikes: %ld\n", states[i]->lightning_strikes);
	    printf("Records with Snow Cover: %ld\n", states[i]->snow_records); 
	    printf("Average Cloud Cover: %0.1f%%\n",(double) (states[i]->sum_cloud_cover / states[i]->num_records)); 
	}
    }
}
