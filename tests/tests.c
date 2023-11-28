// File to put our test cases or something

// Testing code
// fdDir* testDir;	
// testDir = fs_opendir(NULL);
// testDir = fs_opendir("Life is a journey filled with twists and turns. It's a continuous adventure where we learn, grow, and experience the beauty of the world. Every day presents new opportunities and challenges, and it's up to us to make the most of them. Embrace the unknown, cherish the moments, and strive to be the best version of yourself. In this journey, remember that kindness, empathy, and love are the guiding stars that illuminate the path. So, let's keep moving forward with an open heart and a curious mind, making the most of every step we take.");
// testDir = fs_opendir("");
// testDir = fs_opendir("Home/misc");
// testDir = fs_opendir("/Home/misc");
// testDir = fs_opendir("/Home/Misc");
// free(testDir->directory);
// free(testDir);
	
#include "tests.h"
#include "mfs.h"
#include <assert.h>

void runTestFiles() {
	char* formattedPath;

	char* formattedPath;
	formattedPath = fs_formatPathname("/home/misc", "/home");
	assert(strcmp(formattedPath, "/home") == 0);
	free(formattedPath);

	// Test relative path
	formattedPath = fs_formatPathname("/home/misc", "inside");
	assert(strcmp(formattedPath, "/home/misc/inside") == 0);
	free(formattedPath);

	// Test leading / for first arg
	formattedPath = fs_formatPathname("/home/misc/", "inside");
	assert(strcmp(formattedPath, "/home/misc/inside") == 0);
	free(formattedPath);

	// Test leading / for second arg
	formattedPath = fs_formatPathname("/home/misc/", "inside/");
	assert(strcmp(formattedPath, "/home/misc/inside") == 0);
	free(formattedPath);

	// Test if .. works
	formattedPath = fs_formatPathname("/home/misc/", "..");
	assert(strcmp(formattedPath, "/home") == 0);
	free(formattedPath);

	// Test if .. works on root folder
	formattedPath = fs_formatPathname("/", "..");
	assert(strcmp(formattedPath, "/") == 0);
	free(formattedPath);

	// Test if . works
	formattedPath = fs_formatPathname("/home/misc/", "/home/./misc");
	assert(strcmp(formattedPath, "/home/misc") == 0);
	free(formattedPath);

	// Test if . works on root folder
	formattedPath = fs_formatPathname("/", ".");
	assert(strcmp(formattedPath, "/") == 0);
	free(formattedPath);

	// Test starting .. 
	formattedPath = fs_formatPathname("/home/", "../misc");
	assert(strcmp(formattedPath, "/misc") == 0);
	free(formattedPath);

	formattedPath = fs_formatPathname("/", "home");
	assert(strcmp(formattedPath, "/home") == 0);
	free(formattedPath);

	formattedPath = fs_formatPathname("/", "../../../..");
	assert(strcmp(formattedPath, "/") == 0);
	free(formattedPath);
}