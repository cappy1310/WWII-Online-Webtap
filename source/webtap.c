#include "webtap.h"

// Constant Globals
char const filepath_image_list[MAX_CHAR] = "./data/image_url";
char const wwiio_image_url[MAX_CHAR] = "http://map.wwiionline.com/";

int main(int argc, char ** argv)
{
	struct OPTIONS option_list;
	DPRINTF("Args: %d\n", argc);
	Process_Arguments(argc, argv, &option_list);
	curl_global_init(CURL_GLOBAL_DEFAULT);
	DPRINTF("start_while\n");
	while (true) {
		if (option_list.enable_timer)
			Wait_Specific_Interval(option_list);
		Fetch_Images(option_list);
		Process_Images(option_list);
		DPRINTF("continue_while\n");
	}
	DPRINTF("end_while\n");
	curl_global_init(CURL_GLOBAL_DEFAULT);
}

void Process_Arguments(int argc, char ** argv, struct OPTIONS * option_list)
{
	int i = 1;

	// Set defaults.
	option_list->enable_timer = true;
	strncpy(option_list->image_download_path, "./data/image_data/", MAX_CHAR);
	strncpy(option_list->output_path, "./expose/", MAX_CHAR);
	option_list->wait_interval = 15; // minutes
	option_list->wait_tolerance = 5; // seconds
	option_list->xml_output = false;
	option_list->json_output = false;

	// Pull any arguments into the variables.
	while (i < argc) {
		if (strcmp(argv[i], "--xml-output") == 0 || strcmp(argv[i], "-xml") == 0) {
			if (i >= argc)	{
				return;// INSERT ERROR HERE!
			}
			option_list->xml_output = true;
			DPRINTF("XML Output is%s enabled.\n", option_list->xml_output ? "" : "not");
		}
		if (strcmp(argv[i], "--json-output") == 0 || strcmp(argv[i], "-json") == 0) {
			if (i >= argc)	{
				return;// INSERT ERROR HERE!
			}
			option_list->json_output = true;
			DPRINTF("JSON Output is%s enabled.\n", option_list->json_output ? "" : "not");
		}
		if (strcmp(argv[i], "--enable-timer") == 0 || strcmp(argv[i], "-e") == 0) {
			if (i + 1 >= argc)	{
				return;// INSERT ERROR HERE!
			}
			option_list->enable_timer = atoi(argv[i + 1]);
			DPRINTF("Timer is%s enabled.\n", option_list->enable_timer ? "" : "not");
			++i;
		}
		if (strcmp(argv[i], "--output-path") == 0 || strcmp(argv[i], "-o") == 0) {
			if (i + 1 >= argc) {
				return;// INSERT ERROR HERE!
			}
			strncpy(option_list->output_path, argv[i + 1], MAX_CHAR);
			DPRINTF("Output path set to: %s.\n", option_list->output_path);
			++i;
		}
		if (strcmp(argv[i], "--wait-tolerance") == 0 || strcmp(argv[i], "-T") == 0) {
			if (i + 1 >= argc)	{
				return;// INSERT ERROR HERE!
			}
			option_list->wait_tolerance = atoi(argv[i + 1]);
			DPRINTF("Wait tolerance set to: %d.\n", option_list->wait_tolerance);
			++i;
		}
		if (strcmp(argv[i], "--wait-interval") == 0 || strcmp(argv[i], "-i") == 0) {
			if (i + 1 >= argc) {
				return;// INSERT ERROR HERE!
			}
			option_list->wait_interval = atoi(argv[i + 1]);
			DPRINTF("Wait interval set to: %d.\n", option_list->wait_interval);
			++i;
		}
		++i;
	}
}

static size_t my_fwrite(void *buffer, size_t size, size_t nmemb, void *stream)
{
  struct FTPFile *out = (struct FTPFile *) stream;
  if (out && !out->stream) {
    /* open file for writing */
    out->stream = fopen(out->filename, "wb");
    if (!out->stream)
      return -1; /* failure, can't open file to write */
  }
  return fwrite(buffer, size, nmemb, out->stream);
}

void Fetch_Images(struct OPTIONS option_list)
{
	int err = 0;
	char target_url[MAX_CHAR];
	CURL *curl;
	CURLcode result;
	FILE* imageListFileHandle;
	char  currentImageName[MAX_CHAR];
	char  ftpOutputPath[MAX_CHAR];

	imageListFileHandle = fopen(filepath_image_list, "r");
	printf("File handle: %p.\n", imageListFileHandle);
	for (int i = 0; i < IMAGE_NUM; ++i)	{
		err = Get_Next_Image_Name(imageListFileHandle, currentImageName);
		DPRINTF("***Image name: %s.\n", currentImageName);
		strncpy(target_url, wwiio_image_url, MAX_CHAR);
		DPRINTF("***Starting out with: %s.\n", target_url);
		strncat(target_url, currentImageName, MAX_CHAR);
		DPRINTF("***Appending the above yields: %s.\n", target_url);
		strncpy(ftpOutputPath, option_list.image_download_path, MAX_CHAR);
		strncat(ftpOutputPath, currentImageName, MAX_CHAR);
		struct FTPFile curl_input = { ftpOutputPath, NULL };
		curl = curl_easy_init();
		if (curl) {
			DPRINTF("***Fetching from: %s.\n", target_url);
			curl_easy_setopt(curl, CURLOPT_URL, target_url);
			curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, my_fwrite);
			curl_easy_setopt(curl, CURLOPT_WRITEDATA, &curl_input);
			curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
			result = curl_easy_perform(curl);
			curl_easy_cleanup(curl);
			if (CURLE_OK != result) {
				printf("curl failed unexpectedly; result: %d\n", result);
				//return 1;
			}
			if (curl_input.stream)
    			fclose(curl_input.stream); 
		}
	}
	fclose(imageListFileHandle);
}

int Process_Images(struct OPTIONS option_list)
{
	FILE 				* imageListFileHandle;
	char 				  currentImageName[MAX_CHAR];
	char 				  currentImagePath[MAX_CHAR];
	struct CITY_DATA 	* webmapDataList;
	struct CITY 		* xmlMetadataList;
	struct CP_STATE 	* cpstateDataList;
	uchar 				* imageData[IMAGE_NUM];
	int 				  cpstateNumberOfEntries;
	int err, i;
	
	char const filepath_city_data[MAX_CHAR] = "./data/city_data";
	char const filepath_xml_metadata[MAX_CHAR] = "./data/xml_data/cplist.citys.xml";
	char const filepath_xml_output[MAX_CHAR] = "./expose/webtap.cpstates.citys.xml";
	char       filepath_json_output[MAX_CHAR] = "./expose/webtap.cpstates.citys.json";

	// Allocate space for data storage
	webmapDataList = (struct CITY_DATA *)malloc(WEB_CITY_NUM * sizeof(struct CITY_DATA));
	xmlMetadataList = (struct CITY *)malloc(CITY_NUM * sizeof(struct CITY));
	
	// Make sure we can hold all of the cities in cpstateDataList
	if (WEB_CITY_NUM > CITY_NUM)
		cpstateDataList = (struct CP_STATE *)malloc(WEB_CITY_NUM * sizeof(struct CP_STATE));
	else
		cpstateDataList = (struct CP_STATE *)malloc(CITY_NUM * sizeof(struct CP_STATE));

	// Recursively allocate space for imageData
	for (i = 0; i < IMAGE_NUM; ++i)	
		imageData[i] = (uchar *)malloc(IMAGE_X * IMAGE_Y * 3 * sizeof(uchar));
	
	// Load in the list of images to load
	imageListFileHandle = fopen(filepath_image_list,"r");
	
	// If the file couldn't be opened, quit.
	if (imageListFileHandle == NULL)
		FILE_ERROR("imageListFileHandle")

	// Load all the CITY data in from the HTML source	
	err = Load_Webmap_Data_List(filepath_city_data, webmapDataList); 
	if (err != 0)
		FILE_ERROR("city_data")
	
	// Load all the XML Metadata from the website
	err = Load_Xml_Metadata_List(filepath_xml_metadata, xmlMetadataList); 
	if (err != 0)
		FILE_ERROR("cplist.citys.xml")
	
	// Loop through each image
	for (i = 0; i < IMAGE_NUM; ++i)	{
		err = Get_Next_Image_Name(imageListFileHandle, currentImageName);
		strncpy(currentImagePath, option_list.image_download_path, MAX_CHAR);
		strncat(currentImagePath, currentImageName, MAX_CHAR);
		printf("Grabbing an image from: %s.\n", currentImagePath);
		if (err != 0)
			FILE_ERROR("imageListFileHandle")

		err = Load_Image(currentImagePath, i, imageData);
		if (err != 0)
			FILE_ERROR("load_image")
	}
	DPRINTF("Finished loading images!\n");
	// Matches RGB color points with sides
	err = Decide_City_Side(imageData, webmapDataList);
	DPRINTF("Decided the sides!\n");
	// Compares city names to get ID #s
	cpstateNumberOfEntries = Match_City_To_Metadata(webmapDataList, xmlMetadataList, cpstateDataList);
	DPRINTF("Matched the cities!\n");
	// Export to an xml file
	if (option_list.xml_output)
		err = Append_To_Xml(filepath_xml_output, cpstateDataList, cpstateNumberOfEntries);
	DPRINTF("XML Appended!\n");
	if (option_list.json_output)
		err = Append_To_Json(filepath_json_output, cpstateDataList, cpstateNumberOfEntries);
	// Free up mallocated memory
	for (i = 0; i < IMAGE_NUM; ++i)
		stbi_image_free(imageData[i]);  // Special free() for stbi images
	free(webmapDataList);
	free(xmlMetadataList);
	free(cpstateDataList);

	// Successfully executed; return 0.
	return 0;
};

int Load_Image(char * image_path, int index, uchar ** image_data)
{
	int x,y,n; 		// X-Dim, Y-Dim, and N components of JPEG

	// Fetch image data
	image_data[index] = stbi_load(image_path, &x, &y, &n, 0);
	printf("Image path is: %s.\n", image_path);
	if (image_data[index] == NULL)
		FILE_ERROR("stbi_load_image")
	
	// Success	
	return 0;
};

int Get_Next_Image_Name(FILE * imageListFileHandle, char * image_name)
{
	// Pull the next image location from the file
	// Should be in the form of: http://map.wwiionline.com/<image_name>.jpg
	// This pulls off the http:// part
	if (fscanf(imageListFileHandle, "%s\n", image_name) == EOF)
		FILE_ERROR("imageListFileHandle_loop")

	return 0;
};

int Load_Webmap_Data_List(char const * filename, struct CITY_DATA * city_data)
{
	int i = 0;				// Loop var
	FILE * city_data_file;	// File to open
	DPRINTF("Load_Webmap_Data_List!\n");
	DPRINTF("Accessing file: %s\n", filename);
	// Load the file
	city_data_file = fopen(filename, "r");
	
	if (city_data_file == NULL)
		FILE_ERROR("Load_Webmap_Data_List\n");

	// Capture the data from each city
	while (fscanf(city_data_file, "%u;(%u,%u);%20[^\n\r]",
				  &(city_data[i].area), &(city_data[i].x),
				  &(city_data[i].y), &(city_data[i].name)) != EOF) {
		// Set the side to the default 0, which will be marked as "Unknown" if the colors don't match anything.
		city_data[i].side = 0;

		//DPRINTF("Loading Webmap City Data: #%d, Name: %s.\n", i, city_data[i].name);
		++i;
	}

	fclose(city_data_file);

	return 0;
};

int Load_Xml_Metadata_List(char const * xmlMetadataList_filename, struct CITY * xml_data)
{
	FILE * xmlMetadataList_file;	// File to open
	int j;						// Loop var
	char temp[MAX_CHAR];		// Temporary string to hold the first 3 lines
	char format[MAX_CHAR];		// Parsing format

	DPRINTF("Load_XML_Metadata_List!\n");
	// Load the file
	xmlMetadataList_file = fopen(xmlMetadataList_filename, "r");
	
	// The first 3 lines are metadata.
	for (j = 0; j < 3; ++j)
		fgets(temp, MAX_CHAR, xmlMetadataList_file);
	
	// The xml data contains date and time info, which could be used to construct a "Recently Captured Towns" list.
	// Scan in all of the data from the cplist.citys.xml file.
	strncpy(format, "<cp id=\"%d\" name=\"%20[^\"\n\r]\" type=\"%hu\" orig-country=\"%hu\" orig-side=\"%hu\" links=\"%hu\" x=\"%d\" y=\"%d\" absx=\"%d\" absy=\"%d\"/>\n", MAX_CHAR);
	for (j = 0; j < CITY_NUM; ++j) {
		// Scan the entire thing in at once. 
		// Note: [^\"\n\r] is necessary, as %s will grab the rest of the input and store it as a string.
		// 		 Also, the \r will really mess things up if it grabs it.
		fscanf( xmlMetadataList_file, format, 
				&(xml_data[j].id), xml_data[j].name, &(xml_data[j].type), 
				&(xml_data[j].orig_country), &(xml_data[j].orig_side), 
				&(xml_data[j].links), &(xml_data[j].x), &(xml_data[j].y), 
				&(xml_data[j].x_abs), &(xml_data[j].y_abs));
		DPRINTF("%d: Name: %s, ID: %d, AbsX: %d, AbsY: %d, Type: %hu, Links: %hu, OC: %hu, OS: %hu\n", j, xml_data[j].name, xml_data[j].id, xml_data[j].x_abs, xml_data[j].y_abs, xml_data[j].type, xml_data[j].links, xml_data[j].orig_country, xml_data[j].orig_side);
	}

	// Close file
	fclose(xmlMetadataList_file);

	return 0;
};

int Decide_City_Side(uchar ** imageData, struct CITY_DATA * webmapDataList)
{
	int x, y, i, area_index = 0;
	char s[8];

	// Loop through all the cities from the map.wwiionline list.
	for (i = 0; i < WEB_CITY_NUM; ++i) {
		// Store the x and y-coords
		x = webmapDataList[i].x;
		y = webmapDataList[i].y;
		area_index = webmapDataList[i].area - 1;
		// Check the color of the city on the map
		// Blue and White (contested) represent Allied cities.
		// Red and Yellow (contested) represent Axis cities.
		// Potential support for determining if the cities are British/French using the Strat maps in the future.
		// -> area - 1 is used as the webmapDataList sides range from 1-6, while the imageData goes from 0-5
		if (Is_Color_White(imageData[area_index], x, y)) {
			// The color white! This indicates a contested Allied city.
			webmapDataList[i].side = 1;
			webmapDataList[i].contested = true;
			strncpy(s,"Allied\0", 8);
		} else if (Is_Color_Yellow(imageData[area_index], x, y)) {
			// The color yellow! This indicates a contested Axis city.
			webmapDataList[i].side = 2;
			webmapDataList[i].contested = true;			
			strncpy(s,"Allied\0", 8);
		} else if (Is_Color_Red(imageData[area_index], x, y)) {
			// If it is very red, to the AXIS it goes
			webmapDataList[i].side = 2;
			webmapDataList[i].contested = false;
			strncpy(s,"Axis\0", 8);
		} else if (Is_Color_Blue(imageData[area_index], x, y)) {
			// Blue! Another ALLIED town, it is!
			webmapDataList[i].side = 1;
			webmapDataList[i].contested = false;
			strncpy(s,"Allied\0", 8);
		} else {
			// Otherwise, fuck. Uh...mark it as unknown and deal with it. Won't list it in the xml output, at least.
			webmapDataList[i].side = 0;
			webmapDataList[i].contested = false;
			strncpy(s,"Unknown\0", 8);
		}
		DPRINTF("%s {%d,%d}: Color: %s. Result: %s=%d. %sontested.\n", 
			webmapDataList[i].name, webmapDataList[i].x, webmapDataList[i].y,
			Is_Color_Blue(imageData[webmapDataList[i].area-1], x ,y) ? "Blue" : 
			Is_Color_Red(imageData[webmapDataList[i].area-1], x ,y) ? "Red" : 
			Is_Color_Yellow(imageData[webmapDataList[i].area-1], x ,y) ? "Yellow" :
			Is_Color_White(imageData[webmapDataList[i].area-1], x ,y) ? "White" : "Unknown",
			s, webmapDataList[i].side, webmapDataList[i].contested ? "C" : "Unc"); 
	}

	return 0;
}

int Match_City_To_Metadata(struct CITY_DATA * webmapDataList, struct CITY * xmlMetadataList, struct CP_STATE * cpstateDataList)
{
	int i,j,cp;
	int willem;		// Deal with an exception involving the city of Willemstad.
	
	willem = 0;
	cp = 0;
	// Looping through every permutation of the data sets. Not the best way, but it'll do for now.
	for (i = 0; i < WEB_CITY_NUM; ++i) {
		for (j = 0; j < CITY_NUM; ++j) {
			if (xmlMetadataList[j].id == 311 && willem == 1) // Willemstad
				continue;	// This fixes an issue where Willemstad is listed twice in the HTML.
			
			// Compare the names of the two cities.
			if (strcmp(webmapDataList[i].name, xmlMetadataList[j].name) == 0) {
				// If they're similar, we have a match!
				// Since cpstate.citys.xml only lists non-default cities, check
				//		if the original side is the same as the current side.
				// Also, make sure it has a side to begin with. Otherwise, we don't want to list it.
				if (xmlMetadataList[j].orig_side != webmapDataList[i].side && 
				    webmapDataList[i].side != 0) {
					// The CP State is non-default! Store all its data in cpstateDataList.
					cpstateDataList[cp].id = xmlMetadataList[j].id;
					cpstateDataList[cp].owner = webmapDataList[i].side;
					cpstateDataList[cp].controller = webmapDataList[i].side;
					cpstateDataList[cp].contested = webmapDataList[i].contested;
					++cp;
					
					if (strcmp(webmapDataList[i].name, "Willemstad") == 0)
						willem = 1;
				}
			}
		}
	}
	
	// Sort the data by ID (Selection sort)
	Selection_Sort(cp, cpstateDataList);

	// DEBUG
#ifdef DEBUG
	printf("CP State Listing:\n");
	for (i = 0; i < cp; ++i)
		printf("ID: %u, Owner: %hu, Controller: %hu, Contested: %s\n", 
			cpstateDataList[i].id, cpstateDataList[i].owner, 
			cpstateDataList[i].controller, cpstateDataList[i].contested ? "y" : "n");
#endif

	// Return the number of cities not in their default state	
	return cp;
}

/* Helper functions for determining the color of the cities from the image color data. */
bool Is_Color_Blue(uchar * image_data, int x, int y)
{
	return (image_data[Get_Red(x,y)] < MINIMUM_COLOR_VALUE &&
			image_data[Get_Green(x,y)] < MINIMUM_COLOR_VALUE &&
			image_data[Get_Blue(x,y)] > MINIMUM_COLOR_VALUE);
}

bool Is_Color_Red(uchar * image_data, int x, int y)
{
	return (image_data[Get_Red(x,y)] > MINIMUM_COLOR_VALUE &&
			image_data[Get_Green(x,y)] < MINIMUM_COLOR_VALUE &&
			image_data[Get_Blue(x,y)] < MINIMUM_COLOR_VALUE);
}

bool Is_Color_Yellow(uchar * image_data, int x, int y)
{
	return (image_data[Get_Red(x,y)] > MINIMUM_COLOR_VALUE &&
			image_data[Get_Green(x,y)] > MINIMUM_COLOR_VALUE &&
			image_data[Get_Blue(x,y)] < MINIMUM_COLOR_VALUE);
}

bool Is_Color_White(uchar * image_data, int x, int y)
{
	return (image_data[Get_Red(x,y)] > MINIMUM_COLOR_VALUE &&
			image_data[Get_Green(x,y)] > MINIMUM_COLOR_VALUE &&
			image_data[Get_Blue(x,y)] > MINIMUM_COLOR_VALUE);
}

/* Helper functions for getting colored points from the stb_image */
int Get_Red(int x, int y)
{
	return (y * IMAGE_X * 3) + x * 3;
}
int Get_Green(int x, int y)
{
	return Get_Red(x,y) + 1;
}
int Get_Blue(int x, int y)
{
	return Get_Red(x,y) + 2;
}

/* Standard selection sort algorithm for sorting the cities by city number */
void Selection_Sort(int cp_number, struct CP_STATE * cpstateDataList)
{
	int i, j;
	unsigned int key;
	// Sort the data by ID (Selection sort)
	for (j = 0; j < cp_number; ++j) {
		key = cpstateDataList[j].id;
		i = j - 1;
		while (i >= 0 && cpstateDataList[i].id > key) {
			cpstateDataList[i + 1].id = cpstateDataList[i].id;
			i = i - 1;
		}
		cpstateDataList[i + 1].id = key;
		cpstateDataList[i + 1].owner = cpstateDataList[j].owner;
		cpstateDataList[i + 1].controller = cpstateDataList[j].controller;
	}
}

int Append_To_Xml(const char * xml_filename, struct CP_STATE * cp_data, int cp_state_num)
{
#ifdef XML_OUTPUT
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
	mxmlElementSetAttrf(generated, "timestamp", "%ld", (unsigned long)cur_time);
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
	for (i = 0; i < cp_state_num; ++i) {
		cp = mxmlNewElement(cpstates, "cp");
		mxmlElementSetAttrf(cp, "id", "%u", cp_data[i].id);
		mxmlElementSetAttrf(cp, "owner", "%hu", cp_data[i].owner);
		mxmlElementSetAttrf(cp, "controller", "%hu", cp_data[i].controller);
	}
	
	// MXML handles the rest! Pretty nifty.

	// Write the data to file
	xml_file = fopen(xml_filename, "w");
	mxmlSaveFile(cpstates, xml_file, MXML_NO_CALLBACK);
	fclose(xml_file);

	// Clean up memory
	mxmlRelease(cpstates);

	return 0;
#else // ifndef XML_OUTPUT
	return 1;
#endif // XML_OUTPUT
}

int Append_To_Json(char * json_filename, struct CP_STATE * cp_data, int cp_state_num)
{
#ifdef JSON_OUTPUT
	json_object *my_root, *my_array, *cp_entry;

	// Create root object.
	my_root = json_object_new_object();
	// Add version information to root object.
	json_object_object_add(my_root, "ver", json_object_new_string("1.3"));
	// Generate enormous "data" hash key array.
	my_array = json_object_new_array();
	for (int i = 0; i < cp_state_num; ++i) {
		cp_entry = json_object_new_object();
		json_object_object_add(cp_entry, "id", json_object_new_int(cp_data[i].id));
		json_object_object_add(cp_entry, "owner", json_object_new_int(cp_data[i].owner));
		json_object_object_add(cp_entry, "controller", json_object_new_int(cp_data[i].controller));
		json_object_object_add(cp_entry, "contested", json_object_new_string(cp_data[i].contested ? "y" : "n"));
		json_object_array_add(my_array, cp_entry);
		//json_object_put(cp_entry);
	}
	DPRINTF("Finished adding in JSON array.\n");
	json_object_object_add(my_root, "data", my_array);
	//json_object_put(my_array);
	DPRINTF("Trying to print to file...\n");
	json_object_to_file(json_filename, my_root);
	DPRINTF("Print successful.\n");
	
	//json_object_put(my_array);
	json_object_put(my_root);

	return 0;
#else // #ifndef JSON_OUTPUT
	return 1;
#endif // JSON_OUTPUT
}

// DEBUG - Prints out all the values from a loaded image (creates a very large file).
int Test_Image_Data(uchar ** imageData)
{
	int x,y,i;
	for (i = 0; i < IMAGE_NUM; ++i)
		for (x = 0; x < IMAGE_X; ++x)
			for (y = 0; y < IMAGE_Y; ++y)
				printf("%d{%d,%d}: {%u,%u,%u}\n",i,x,y,(imageData[i])[Get_Red(x,y)], (imageData[i])[Get_Green(x,y)], (imageData[i])[Get_Blue(x,y)]);
	
	return 0;
}

// DEBUG - Creates a fake image with predicatable and overflowing values for testing purposes.
int Fake_Image(uchar ** image, int i)
{
	int x,y;
	for (x = 0; x < IMAGE_X; ++x) {
		for (y = 0; y < IMAGE_Y; ++y) {
			(image[i])[Get_Red(x,y)] = (uchar) x;
			(image[i])[Get_Green(x,y)] = (uchar) y;
			(image[i])[Get_Blue(x,y)] = (uchar) ((x*y) % 255);
		}
	}
	return 0;
}
