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

void testBio() {
		printf("\n----------------------------------\nTesting...\n----------------------------------\n");
    char* testFilename = "/testfile.txt";
    char* writeData = "1-2-3-4-5-6-7-8-9-10-11-12-13-14-15-16-17-18-19-20-21-22-23-24-25-26-27-28-29-30-31-32-33-34-35-36-37-38-39-40-41-42-43-44-45-46-47-48-49-50-51-52-53-54-55-56-57-58-59-60-61-62-63-64-65-66-67-68-69-70-71-72-73-74-75-76-77-78-79-80-81-82-83-84-85-86-87-88-89-90-91-92-93-94-95-96-97-98-99-100-101-102-103-104-105-106-107-108-109-110-111-112-113-114-115-116-117-118-119-120-121-122-123-124-125-126-127-128-129-130-131-132-133-134-135-136-137-138-139-140-141-142-143-144-145-146-147-148-149-150-151-152-153-154-155-156-157-158-159-160-161-162-163-164-165-166-167-168-169-170-171-172-173-174-175-176-177-178-179-180-181-182-183-184-185-186-187-188-189-190-191-192-193-194-195-196-197-198-199-200-201-202-203-204-205-206-207-208-209-210-211-212-213-214\0";
    char readData[1000]; // Buffer to read data into

    // Open the file for writing (and create it if it doesn't exist)
		printf("Opening file (create and write)...\n");
    b_io_fd fd = b_open(testFilename, O_WRONLY | O_CREAT);
    if (fd < 0) {
        printf("Error opening file for writing.\n");
        return;
    }
		
    // Write data to the file
		printf("\nWriting to file...\n");
    int bytesWritten = b_write(fd, writeData, strlen(writeData));
    if (bytesWritten < 0) {
        printf("Error writing to file.\n");
        b_close(fd);
        return;
    }

    // Open the file for reading
		printf("\nOpening file (read)...\n");
    fd = b_open(testFilename, O_RDONLY);
    if (fd < 0) {
        printf("Error opening file for reading.\n");
        return;
    }

    // Read data from the file
		printf("Reading file...\n");
    int bytesRead = b_read(fd, readData, strlen(writeData)); // Leave space for null terminator
    if (bytesRead < 0) {
        printf("Error reading from file.\n");
        //b_close(fd);
        return;
    }

  

	
  // Null terminate the string read
    readData[bytesRead] = '\0';
    // Output the read data
    printf("Data read from file: %s\n", readData);
}
