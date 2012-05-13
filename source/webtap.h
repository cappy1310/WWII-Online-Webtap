#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h> // *nix only.
#include <stdbool.h>
#include <curl/curl.h>
#include <mxml.h>

#ifdef LIBTAP
#include <tap.h>
#endif

// Maximum number of characters to a string
// Note: This is rather arbitrary.
#define MAX_CHAR 255
// Number of images to be processed
#define IMAGE_NUM 6
// Size of each image
#define IMAGE_X 579
#define IMAGE_Y 555
// Number of cities processed from cplist.citys.xml
#define CITY_NUM 551
// Number of cities processed from the map.wwiionline.com html files
#define WEB_CITY_NUM 533
// Lower limit for a point to be considered a color.
// I.e., if the R component has a value of 240, and the min value is 220, then
//       there is a red component in the color.
#define MINIMUM_COLOR_VALUE 220

#ifdef DEBUG
#define DPRINTF(x, ...) printf(x, ##__VA_ARGS__)
#else
#define DPRINTF(x, ...)
#endif

// Simple error processing.
#define FILE_ERROR(x) { printf("Error loading file: %s.\n",x); return 1; }

// Typedefs and Structs
typedef unsigned char uchar;

enum CityArea	// Refers to the 6 different map.wwiionline.com maps
{
	Holland = 0,			// Holland Supply
	Channel = 1,			// Channel Supply
	Belgium = 2,			// Belgium Supply
	CentralGerman = 3,		// Central German Supply
	NEFrance = 4,			// NE France Supply
	Maginot = 5				// Maginot Supply
};

struct OPTIONS
{
	bool enable_timer;
	unsigned int waitInterval;  // In minutes.
	unsigned int waitTolerance; // In seconds.
	bool xmlOutput;
	bool jsOutput;
	char imageDownloadPath[MAX_CHAR];
	char outputFilePath[MAX_CHAR];
};

struct CITY_DATA				// Corresponds to the map.wwiionline.com Maps
{
	enum CityArea area;			// Refers to which map the city appears in
	unsigned int x;				// X coordinate of the city on the map
	unsigned int y;				// Y coordinate of the city on the map
	char name[MAX_CHAR];		// Name of the city
	unsigned short int side;	// Which side the city belongs to: 0 - Unknown, 1 - Allied, 2 - Axis
};

struct CITY								// Corresponds to the cplist.citys.xml <cp> tag and values
{
	unsigned int id;					// The internal ID for the city
	char name[MAX_CHAR];				// The name of the city
	unsigned short int type;			// Type (bridge, big city, small city, etc.); unused, but imported
	unsigned short int orig_country;	// The country the city starts a campaign on ()
	unsigned short int orig_side;		// The side the city starts a campaign on ()
	unsigned short int links;			// Number of links to other cities
	signed int x;						// Its x-coordinate
	signed int y;						// Its y-coordinate
	signed int x_abs;					// Its x-coordinate
	signed int y_abs; 					// Its y-coordinate
};

struct CP_STATE	
{
	unsigned int id;					// The internal ID for the city
	unsigned short int owner;			// The side that owns the city
	unsigned short int controller;		// The side that controls the city (slight difference?)
};

// For the curl call-back.
struct FTPFile
{
	const char * filename;
	FILE * stream;
};

// Basic header
void Process_Arguments(int, char **, struct OPTIONS *);
void Wait_Specific_Interval(struct OPTIONS);
void Fetch_Images(struct OPTIONS);
int Process_Images(struct OPTIONS);
int Load_Image(char *, int, uchar **);					// Loads an image
int Get_Next_Image_Name(FILE *, char *);					// Gets the next image path from imageListFileHandle
int Load_Webmap_Data_List(char const *, struct CITY_DATA *);		// Loads the file city_data
int Load_Xml_Metadata_List(char const *, struct CITY *);			// Loads the file cplist.citys.xml
int Decide_City_Side(uchar **, struct CITY_DATA *);		// Assigns allied/axis side to cities from RGB points
int Match_City_To_Metadata(struct CITY_DATA *, struct CITY *, struct CP_STATE *);	// Finds IDs by matching names
void Selection_Sort(int, struct CP_STATE *);					// Standard selection sort
int Append_To_Xml(char const *, struct CP_STATE *, int);		// Write to the XML file
int Get_Red(int, int);
int Get_Green(int, int);
int Get_Blue(int, int);

// STBI Library Declarations
void stbi_image_free(unsigned char *);
unsigned char * stbi_load(char const *, int *, int *, int *, int);

// curl Library Headers
CURLcode curl_global_init(long flags);
void curl_global_cleanup(void);

// Mini-XML Library Headers


// Debug Headers
int Fake_Image(uchar **, int);							// Used to debug the image
int Test_Image_Data(uchar **);							// Used to debug the image
