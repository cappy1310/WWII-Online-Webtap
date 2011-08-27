#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mxml.h>
#include <time.h>

// Maximum number of characters to a string
// Note: This is rather arbitrary.
#define MAX_CHAR 255
// Number of images to be processed
#define IMAGE_NUM 6
// Size of each image
#define IMAGE_X 555
#define IMAGE_Y 579
// Number of cities processed from cplist.citys.xml
#define CITY_NUM 551
// Number of cities processed from the map.wwiionline.com html files
#define WEB_CITY_NUM 533
// These defines make it easier to access the datapoints in the uchar * array from image_load
// For row-major form of a matrix: X is the row, Y is the column, Z is the total number of columns
#define R(x,y,z) (x*z+y)*3
#define G(x,y,z) (x*z+y)*3+1
#define B(x,y,z) (x*z+y)*3+2

// Files that need to be opened
#define FILE_IMAGE_LIST "./data/image_url"
#define FILE_CITY_DATA "./data/html_data/city_data"
#define FILE_XML_METADATA "./data/xml_data/cplist.citys.xml"
#define FILE_XML_OUTPUT "./webtap.cpstates.citys.xml"

// Simple error processing.
#define LOAD_ERROR(x) { printf("Error loading file: %s.\n",x); return 1; }

// Typedefs and Structs
typedef unsigned char uchar;

struct CITY_DATA				// Corresponds to the map.wwiionline.com Maps
{
	unsigned short int area;	// [0-5], refers to which map the city appears in
								/*	0 - Holland Supply
								*	1 - Channel Supply
								*	2 - Belgium Supply
								*	3 - Central German Supply
								*	4 - NE France Supply
								*	5 - Maginot Supply
								*/	
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

// Basic header
int load_image(char *, int, uchar **);					// Loads an image
int get_new_image_path(FILE *, char *);					// Gets the next image path from image_list
int load_webmap_data(char *, struct CITY_DATA *);		// Loads the file city_data
int load_xml_metadata(char *, struct CITY *);			// Loads the file cplist.citys.xml
int decide_city_side(uchar **, struct CITY_DATA *);		// Assigns allied/axis side to cities from RGB points
int match_city_to_metadata(struct CITY_DATA *, struct CITY *, struct CP_STATE *);	// Finds IDs by matching names
int append_to_xml(char *, struct CP_STATE *, int);		// Write to the XML file

// Debug Headers
int fake_image(uchar **, int);							// Used to debug the image
int test_image_data(uchar **);							// Used to debug the image


int main(int argc, char ** argv)
{
	FILE * image_list;
	FILE * xml_file;
	char cur_image[MAX_CHAR];
	struct CITY_DATA * webmap_data;
	struct CITY * xml_metadata;
	struct CP_STATE * cpstate_data;
	uchar * image_data[IMAGE_NUM];
	int i;
	int cp_state_entries;
	
	// Allocate space for data storage
	webmap_data = (struct CITY_DATA *)malloc(WEB_CITY_NUM * sizeof(struct CITY_DATA));
	xml_metadata = (struct CITY *)malloc(CITY_NUM * sizeof(struct CITY));
	
	// Make sure we can hold all of the cities in cpstate_data
	if (WEB_CITY_NUM > CITY_NUM)
	{
		cpstate_data = (struct CP_STATE *)malloc(WEB_CITY_NUM * sizeof(struct CP_STATE));
	} else {
		cpstate_data = (struct CP_STATE *)malloc(CITY_NUM * sizeof(struct CP_STATE));
	}

	// Recursively allocate space for image_data
	for(i = 0; i < IMAGE_NUM; i++)	
		image_data[i] = (uchar *)malloc(IMAGE_X * IMAGE_Y * 3 * sizeof(uchar));
	
	// Load in the list of images to load
	image_list = fopen(FILE_IMAGE_LIST,"r");
	
	// If the file couldn't be opened, quit.
	if(image_list == NULL)
		LOAD_ERROR("image_list")

	// Load all the CITY data in from the HTML source
	if(load_webmap_data(FILE_CITY_DATA, webmap_data) != 0)
		LOAD_ERROR("city_data")
	
	// Load all the XML Metadata from the website
	if(load_xml_metadata(FILE_XML_METADATA, xml_metadata) != 0)
		LOAD_ERROR("cplist.citys.xml")
	
	// Loop through each image
	for (i = 0; i < IMAGE_NUM; i++)
	{
		if(get_next_image_path(image_list, cur_image) != 0)
			LOAD_ERROR("image_list")
		if(load_image(cur_image, i, image_data) != 0)
			LOAD_ERROR("load_image")
	}
	
	// Matches RGB color points with sides
	decide_city_side(image_data, webmap_data);
	
	// Compares city names to get ID #s
	cp_state_entries = match_city_to_metadata(webmap_data, xml_metadata, cpstate_data);
	
	// Export to an xml file
	append_to_xml(FILE_XML_OUTPUT, cpstate_data, cp_state_entries);
	
	// Free up mallocated memory
	for(i = 0; i < IMAGE_NUM; i++)
		stbi_image_free(image_data[i]);  // Special free() for stbi images
	free(webmap_data);
	free(xml_metadata);
	free(cpstate_data);

	// Successfully executed; return 0.
	return 0;
};

int load_image(char * img_name, int index, uchar ** data)
{
	int x,y,n; 		// X-Dim, Y-Dim, and N components of JPEG

	// Fetch image data
	data[index] = stbi_load(img_name, &x, &y, &n, 0);

	if(data[index] == NULL)
		LOAD_ERROR("load_image")
	
	// Success	
	return 0;
};

int get_next_image_path(FILE * image_list, char * image_name)
{
	char temp[MAX_CHAR] = "./data/";
	
	// Pull the next image location from the file
	// Should be in the form of: http://map.wwiionline.com/<image_name>.jpg
	// This pulls off the http:// part
	if(fscanf(image_list, "http://%s\n", image_name) == EOF)
		LOAD_ERROR("image_list_loop")

	// Now we add the ./data/ onto the beginning of the string
	strncat(temp, image_name, MAX_CHAR);

	// Copy the temp string back into the original
	strncpy(image_name, temp, MAX_CHAR);
	
	return 0;
};

int load_webmap_data(char * filename, struct CITY_DATA * data)
{
	int j;					// Loop var
	FILE * city_data_file;	// File to open
	
	// Load the file
	city_data_file = fopen(filename, "r");
	
	j = 0;
	// Loop through the file
	while (fscanf(city_data_file, "%hu;(%d,%d);%20[^\n\r]", &(data[j].area), &(data[j].y), &(data[j].x), data[j].name) != EOF)
	{
		// Set the side to the default 0, which will be marked as "Unknown" if the colors don't match anything.
		data[j].side = 0;
		j++;
	}

	return 0;
};

int load_xml_metadata(char * xml_metadata_filename, struct CITY * xml_data)
{
	FILE * xml_metadata_file;	// File to open
	int j;						// Loop var
	char temp[MAX_CHAR];		// Temporary string to hold the first 3 lines
	
	// Load the file
	xml_metadata_file = fopen(xml_metadata_filename, "r");
	
	// The first 3 lines are unnecessary
	for(j = 0; j < 3; j++)
		fgets(temp, MAX_CHAR, xml_metadata_file);
	
	// The xml data contains date and time info, which could be used to construct a "Recently Captured Towns" list.
	// Scan in all of the data from the cplist.citys.xml file.
	for(j = 0; j < CITY_NUM; j++)
	{
		// Scan the entire thing in at once. 
		// Note: [^\"\n\r] is necessary, as %s will grab the rest of the input and store it as a string.
		// 		 Also, the \r will really mess things up if it grabs it.
		fscanf(xml_metadata_file, "<cp id=\"%d\" name=\"%20[^\"\n\r]\" type=\"%hu\" orig-country=\"%hu\" orig-side=\"%hu\" links=\"%hu\" x=\"%d\" y=\"%d\" absx=\"%d\" absy=\"%d\"/>\n", &(xml_data[j].id), xml_data[j].name, &(xml_data[j].type), &(xml_data[j].orig_country), &(xml_data[j].orig_side), &(xml_data[j].links), &(xml_data[j].x), &(xml_data[j].y), &(xml_data[j].x_abs), &(xml_data[j].y_abs));
		// DEBUG
		//printf("%d: Name: %s, ID: %d, AbsX: %d, AbsY: %d, Type: %hu, Links: %hu, OC: %hu, OS: %hu\n", j, xml_data[j].name, xml_data[j].id, xml_data[j].x_abs, xml_data[j].y_abs, xml_data[j].type, xml_data[j].links, xml_data[j].orig_country, xml_data[j].orig_side);
	}

	return 0;
};

int decide_city_side(uchar ** image_data, struct CITY_DATA * webmap_data)
{
	int x, y, i;
	char s[8];
	
	// Loop through all the cities from the map.wwiionline list.
	for(i = 0; i < WEB_CITY_NUM; i++)
	{
		// Store the x and y-coords
		x = webmap_data[i].x;
		y = webmap_data[i].y;
		
		// Check the color of the city on the map
		// Blue and White (contested) represent Allied cities.
		// Red and Yellow (contested) represent Axis cities.
		// Potential support for determining if the cities are British/French using the Strat maps in the future.
		// -> area - 1 is used as the webmap_data sides range from 1-6, while the image_data goes from 0-5
		if((image_data[webmap_data[i].area - 1])[R(x,y,IMAGE_Y)] > 220 &&
		   (image_data[webmap_data[i].area - 1])[G(x,y,IMAGE_Y)] > 220 &&
		   (image_data[webmap_data[i].area - 1])[B(x,y,IMAGE_Y)] > 220) // White
		{
			// The color white! This indicates a contested Allied city.
			webmap_data[i].side = 1;
			strncpy(s,"Allied\0", 8);
		}
		else if((image_data[webmap_data[i].area - 1])[R(x,y,IMAGE_Y)] > 220 &&
		   		(image_data[webmap_data[i].area - 1])[G(x,y,IMAGE_Y)] > 220 &&
		   		(image_data[webmap_data[i].area - 1])[B(x,y,IMAGE_Y)] < 220) // Yellow
		{
			// The color yellow! This indicates a contested Axis city.
			webmap_data[i].side = 2;
			strncpy(s,"Allied\0", 8);
		}
		else if((image_data[webmap_data[i].area - 1])[R(x,y,IMAGE_Y)] > 220 &&
		   		(image_data[webmap_data[i].area - 1])[G(x,y,IMAGE_Y)] < 220 &&
		   		(image_data[webmap_data[i].area - 1])[B(x,y,IMAGE_Y)] < 220) // Red
		{
			// If it is very red, to the AXIS it goes
			webmap_data[i].side = 2;
			strncpy(s,"Axis\0", 8);
		}
		else if((image_data[webmap_data[i].area - 1])[R(x,y,IMAGE_Y)] < 220 &&
		   		(image_data[webmap_data[i].area - 1])[G(x,y,IMAGE_Y)] < 220 &&
		   		(image_data[webmap_data[i].area - 1])[B(x,y,IMAGE_Y)] > 220) // Blue
		{
			// Blue! Another ALLIED town, it is!
			webmap_data[i].side = 1;
			strncpy(s,"Allied\0", 8);
		}
		else
		{
			// Otherwise, fuck. Uh...mark it as unknown and deal with it. Won't list it in the xml output, at least.
			webmap_data[i].side = 0;
			strncpy(s,"Unknown\0", 8);
		}
		// DEBUG
		//printf("The city %s at {%d,%d} has %u red, %u green, and %u blu and is %s=%d.\n", webmap_data[i].name, webmap_data[i].x, webmap_data[i].y, (image_data[webmap_data[i].area-1])[R(x,y,IMAGE_Y)], (image_data[webmap_data[i].area-1])[G(x,y,IMAGE_Y)], (image_data[webmap_data[i].area-1])[B(x,y,IMAGE_Y)], s, webmap_data[i].side); 
	}

	return 0;
}

int match_city_to_metadata(struct CITY_DATA * webmap_data, struct CITY * xml_metadata, struct CP_STATE * cpstate_data)
{
	int i,j,cp;
	int willem;
	int key;		// For sort algorith
	
	willem = 0;
	cp = 0;
	// Looping through every permutation of the data sets. Not the best way, but it'll do for now.
	for(i = 0; i < WEB_CITY_NUM; i++)
	{
		for(j = 0; j < CITY_NUM; j++)
		{
			if (xml_metadata[j].id == 311 && willem == 1) // Willemstad
				continue;	// This fixes an issue where Willemstad is listed twice in the webmap.
			
			// Compare the names of the two cities.
			if(strcmp(webmap_data[i].name, xml_metadata[j].name) == 0)
			{
				// If they're similar, we have a match!
				// Since cpstate.citys.xml only lists non-default cities, check
				//		if the original side is the same as the current side.
				// Also, make sure it has a side to begin with. Otherwise, we don't want to list it.
				if(xml_metadata[j].orig_side != webmap_data[i].side && webmap_data[i].side != 0)
				{
					// The CP State is non-default! Store all its data in cpstate_data.
					cpstate_data[cp].id = xml_metadata[j].id;
					cpstate_data[cp].owner = webmap_data[i].side;
					cpstate_data[cp].controller = webmap_data[i].side;
					cp++;
					
					if(strcmp(webmap_data[i].name, "Willemstad") == 0)
						willem = 1;
				}
			}
		}
	}
	
	// Sort the data by ID (Selection sort)
	for(j = 0; j < cp; j++)
	{
		key = cpstate_data[j].id;
		i = j - 1;
		while(i >= 0 && cpstate_data[i].id > key)
		{
			cpstate_data[i + 1].id = cpstate_data[i].id;
			i = i - 1;
		}
		cpstate_data[i + 1].id = key;
		cpstate_data[i + 1].owner = cpstate_data[j].owner;
		cpstate_data[i + 1].controller = cpstate_data[j].controller;
	}

	// DEBUG
	//printf("CP State Listing:\n");
	//for(i = 0; i < cp; i++)
	//	printf("ID: %u, Owner: %hu, Controller: %hu\n",cpstate_data[i].id, cpstate_data[i].owner, cpstate_data[i].controller);

	// Return the number of cities not in their default state	
	return cp;
}

int append_to_xml(char * xml_filename, struct CP_STATE * cp_data, int cp_state_num)
{
	FILE * xml_file;
	int i;
	// For printing the time
	char time_str[100];
	const struct tm * time_info;
	time_t cur_time;

	// MXML Nodes, for creating the xml document
	mxml_node_t * cpstates;
	mxml_node_t * generated;
	mxml_node_t * description;
	mxml_node_t * defaults;
	mxml_node_t * def;
	mxml_node_t * cp;

	// Get the time
	time(&cur_time);
	time_info = localtime(&cur_time);
	strftime(time_str, 100, "%Y-%m-%d %H:%M:%S", time_info); // 2011-06-25 00:15:29, year-month-day hour:minute:second

	// MXML time
	// Default wrap is 75; We don't want wraparound, though.
	mxmlSetWrapMargin(0);
	
	// Build the tree

	// First, the static headers
	// <cpstates version="1.3" copyright="(C) Playnet Inc 2007-2010, All rights reserved.">
	cpstates = mxmlNewElement(MXML_NO_PARENT, "cpstates");
	mxmlElementSetAttr(cpstates, "version", "1.3");
	mxmlElementSetAttr(cpstates, "copyright", "(C) Playnet Inc 2007-2010, All rights reserved.");
	// <generated date="[year]-[month]-[day] [24-hour hour]:[minute]:[second]" timestamp="[current linux timestamp]"/>
	generated = mxmlNewElement(cpstates, "generated");
	mxmlElementSetAttrf(generated, "date", "%s", time_str);
	mxmlElementSetAttrf(generated, "timestamp", "%d", cur_time);
	// <description>
	// 	City-only list of *non-default* CP states
	// </description>
	description = mxmlNewElement(cpstates, "description");
	mxmlNewText(description, 0, "\tCity-only list of *non-default* CP states");
	// <defaults>
	// 	<def att="contested" value="n"/>
	//</defaults>
	defaults = mxmlNewElement(cpstates, "defaults");
	def = mxmlNewElement(defaults, "def");
	mxmlElementSetAttr(def, "att", "contested");
	mxmlElementSetAttr(def, "value", "n");

	// Now, to loop through the CP_STATE struct for the dynamic data
	// <cp id="##" owner="##" controller="##" />
	for(i = 0; i < cp_state_num; i++)
	{
		cp = mxmlNewElement(cpstates, "cp");
		mxmlElementSetAttrf(cp, "id", "%u", cp_data[i].id);
		mxmlElementSetAttrf(cp, "owner", "%hu", cp_data[i].owner);
		mxmlElementSetAttrf(cp, "controller", "%hu", cp_data[i].controller);
	}
	
	// MXML handles the rest! Pretty nifty.

	// Open the file for writing
	xml_file = fopen(xml_filename, "w");
    
	// Save the XML to the file
	mxmlSaveFile(cpstates, xml_file, MXML_NO_CALLBACK);
    
	// Close the file pointer
	fclose(xml_file);

	// Free the memory from the nodes
	mxmlRelease(cpstates);

	return 0;
}

// DEBUG - Prints out all the values from a loaded image (creates a very large file).
int test_image_data(uchar ** image_data)
{
	int x,y,i;
	for(i = 0; i < IMAGE_NUM; i++)
		for(x = 0; x < IMAGE_X; x++)
			for(y = 0; y < IMAGE_Y; y++)
				printf("%d{%d,%d}: {%u,%u,%u}\n",i,x,y,(image_data[i])[R(x,y,IMAGE_Y)], (image_data[i])[G(x,y,IMAGE_Y)], (image_data[i])[B(x,y,IMAGE_Y)]);
	
	return 0;
}

// DEBUG - Creates a fake image with predicatable and overflowing values for testing purposes.
int fake_image(uchar ** image, int i)
{
	int x,y,j;
	for(x = 0; x < IMAGE_X; x++)
	{
		for(y = 0; y < IMAGE_Y; y++)
		{
			(image[i])[R(x,y,IMAGE_Y)] = (uchar) x;
			(image[i])[G(x,y,IMAGE_Y)] = (uchar) y;
			(image[i])[B(x,y,IMAGE_Y)] = (uchar) ((x*y) % 255);
		}
	}
	return 0;
}
