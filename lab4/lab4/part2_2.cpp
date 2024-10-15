#include <iostream>
#include "libppm.h"
#include <cstdint>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
using namespace std;

struct image_t* S1_smoothen(struct image_t *input_image)
{
	// TODO
	// remember to allocate space for smoothened_image. See read_ppm_file() in libppm.c for some help.
	int w1 = input_image->width;
	int h1 = input_image->height;
	struct image_t* new_image = new struct image_t;
	new_image->height = h1;
	new_image->width = w1;

	// Allocate memory for the image pixel data
	new_image->image_pixels = new uint8_t**[h1];
	for (int i = 0; i < h1; i++) {
		new_image->image_pixels[i] = new uint8_t*[w1];
		for (int j = 0; j < w1; j++) {
			new_image->image_pixels[i][j] = new uint8_t[3];
		}
	}

	// Perform the smoothing operation
	for (int i = 0; i < h1; i++) {
		for (int j = 0; j < w1; j++) {
			for (int k = 0; k < 3; k++) {  

				int cnt = 0;
				int val = 0;

				for (int di = -1; di <= 1; di++) {
					for (int dj = -1; dj <= 1; dj++) {
						int ni = i + di;
						int nj = j + dj;
						if (ni >= 0 && ni < h1 && nj >= 0 && nj < w1) {
							cnt++;
							val += input_image->image_pixels[ni][nj][k];
						}
					}
				}
				new_image->image_pixels[i][j][k] = val / cnt;
			}
		}
	}
	return new_image;
}

struct image_t* S2_find_details(struct image_t *input_image, struct image_t *smoothened_image)
{

	// TODO
	int w1 = input_image->width;
	int h1 = input_image->height;
	struct image_t* new_image = new struct image_t;
	new_image->height = h1;
	new_image->width = w1;

	// Allocate memory for the image pixel data
	new_image->image_pixels = new uint8_t**[h1];
	for (int i = 0; i < h1; i++) {
		new_image->image_pixels[i] = new uint8_t*[w1];
		for (int j = 0; j < w1; j++) {
			new_image->image_pixels[i][j] = new uint8_t[3];
		}
	}
	for (int i = 0; i < h1; i++) {
		for (int j = 0; j < w1; j++) {
			for (int k = 0; k < 3; k++) {  
				int detail_value = input_image->image_pixels[i][j][k]- smoothened_image->image_pixels[i][j][k];
				if (detail_value < 0) detail_value = 0;
				if (detail_value > 255) detail_value = 255;
				new_image->image_pixels[i][j][k] = detail_value;
			}
		}
	}
	return new_image;
}

struct image_t* S3_sharpen(struct image_t *input_image, struct image_t *details_image)
{
	// TODO
	int w1 = input_image->width;
	int h1 = input_image->height;
	struct image_t* new_image = new struct image_t;
	new_image->height = h1;
	new_image->width = w1;

	// Allocate memory for the image pixel data
	new_image->image_pixels = new uint8_t**[h1];
	for (int i = 0; i < h1; i++) {
		new_image->image_pixels[i] = new uint8_t*[w1];
		for (int j = 0; j < w1; j++) {
			new_image->image_pixels[i][j] = new uint8_t[3];
		}
	}
	for (int i = 0; i < h1; i++) {
		for (int j = 0; j < w1; j++) {
			for (int k = 0; k < 3; k++) {  
				int sharp_value = input_image->image_pixels[i][j][k]+ details_image->image_pixels[i][j][k];
				if (sharp_value < 0) sharp_value = 0;
				if (sharp_value > 255) sharp_value = 255;
				new_image->image_pixels[i][j][k] = sharp_value;
				
			}
		}
	}
	return new_image;
}

int main(int argc, char **argv)
{
    struct image_t *input_image1 = read_ppm_file(argv[1]);
    int pipefd1[2], pipefd2[2];  // Two pipes for communication
    struct image_t *message1, *message2;
    pid_t c1, c2, c3;

    // Create the pipes
    if (pipe(pipefd1) == -1 || pipe(pipefd2) == -1) {
        perror("pipe");
        exit(EXIT_FAILURE);
    }

    // First child (smoothen process)
    c1 = fork();
    if (c1 == -1) {
        perror("fork");
        exit(EXIT_FAILURE);
    }

    if (c1 == 0) {
        // Close unused pipe ends
        close(pipefd1[0]);  // Close read end of pipefd1

        // Perform smoothing
        struct image_t *smoothened_image = S1_smoothen(input_image1);
        write(pipefd1[1], smoothened_image, sizeof(struct image_t));  // Send smoothened image
        close(pipefd1[1]);  // Close write end after sending

        // Clear smoothened image memory
        memset(smoothened_image, 0, sizeof(struct image_t));
        free(smoothened_image);  // Free dynamically allocated memory (if applicable)

        exit(EXIT_SUCCESS);
    }

    // Second child (details process)
    c2 = fork();
    if (c2 == -1) {
        perror("fork");
        exit(EXIT_FAILURE);
    }

    if (c2 == 0) {
        // Close unused pipe ends
        close(pipefd1[1]);  // Close write end of pipefd1
        close(pipefd2[0]);  // Close read end of pipefd2

        // Receive smoothened image from pipefd1
        struct image_t smoothened_image;
        read(pipefd1[0], &smoothened_image, sizeof(struct image_t));  
        close(pipefd1[0]);  // Close read end of pipefd1 after reading

        // Perform detail finding
        struct image_t *details_image = S2_find_details(input_image1, &smoothened_image);
        write(pipefd2[1], details_image, sizeof(struct image_t));  // Send detailed image
        close(pipefd2[1]);  // Close write end of pipefd2

        // Clear memory for both smoothened and detailed images
        memset(&smoothened_image, 0, sizeof(struct image_t));  // Clear smoothened image
        memset(details_image, 0, sizeof(struct image_t));      // Clear detailed image
        exit(EXIT_SUCCESS);
    }

    // Third child (sharpen process)
    c3 = fork();
    if (c3 == -1) {
        perror("fork");
        exit(EXIT_FAILURE);
    }

    if (c3 == 0) {
        // Close unused pipe ends
        close(pipefd1[0]); 
        close(pipefd1[1]);  // Close both ends of pipefd1
        close(pipefd2[1]);  // Close write end of pipefd2

        // Receive detailed image from pipefd2
        struct image_t details_image;
        read(pipefd2[0], &details_image, sizeof(struct image_t));  
        close(pipefd2[0]);  // Close read end of pipefd2

        // Perform sharpening
        struct image_t *sharpened_image = S3_sharpen(input_image1, &details_image);
        write_ppm_file(argv[2], sharpened_image);

        // Clear memory for detailed and sharpened images
        memset(&details_image, 0, sizeof(struct image_t));  // Clear detailed image
        memset(sharpened_image, 0, sizeof(struct image_t));  // Clear sharpened image

        exit(EXIT_SUCCESS);
    }

    // Parent process
    // Close all pipe ends in the parent
    close(pipefd1[0]);
    close(pipefd1[1]);
    close(pipefd2[0]);
    close(pipefd2[1]);

    // Wait for all children to finish
    wait(NULL);  // Wait for c1
    wait(NULL);  // Wait for c2
    wait(NULL);  // Wait for c3

    // Clear input image memory
    memset(input_image1, 0, sizeof(struct image_t));
    free(input_image1);  // Free dynamically allocated memory (if applicable)

    return 0;
}
