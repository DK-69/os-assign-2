#include <iostream>
#include "libppm.h"
#include <cstdint>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <chrono>
#include <stdlib.h>
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

void free_image(image_t* img) {
    if (img) {
        if (img->image_pixels) {
            for (int i = 0; i < img->height; i++) {
                for (int j = 0; j < img->width; j++) {
                    delete[] img->image_pixels[i][j]; // Free each pixel array
                }
                delete[] img->image_pixels[i]; // Free each row
            }
            delete[] img->image_pixels; // Free the height array
        }
        delete img; // Free the image structure itself
    }
}

struct image_t* allocate_image(int width, int height) {
    struct image_t* img = new image_t;
    img->width = width;
    img->height = height;
    uint8_t* data = new uint8_t[height * width * 3];  // Contiguous allocation

    img->image_pixels = new uint8_t**[height];
    for (int i = 0; i < height; ++i) {
        img->image_pixels[i] = new uint8_t*[width];
        for (int j = 0; j < width; ++j) {
            img->image_pixels[i][j] = &data[(i * width + j) * 3];  // Point to the correct location in the contiguous block
        }
    }

    return img;
}

int main(int argc, char **argv) {
    if (argc != 3) {
        cout << "usage: ./a.out <input_file> <output_file>\n";
        return 1;
    }

    // Start timing for reading the image
    auto read_start = std::chrono::high_resolution_clock::now();
    struct image_t *input_image = read_ppm_file(argv[1]);
    if (!input_image) {
        cerr << "Failed to read input image." << endl;
        return 1;
    }

    // Create pipes for communication
    int pipefd[2];  // For smoothing -> details communication
    int pipefd2[2]; // For details -> sharpening communication

    if (pipe(pipefd) == -1 || pipe(pipefd2) == -1) {
        perror("pipe");
        return 1;
    }

    pid_t pid1 = fork();
    if (pid1 < 0) {
        perror("fork failed");
        return 1;
    }

    if (pid1 > 0) {  // Parent process: Smoothing
        close(pipefd[0]); // Close read end of first pipe

        for (int i = 0; i < 1000; i++) {
            // Perform smoothing
            auto smooth_start = std::chrono::high_resolution_clock::now();
            struct image_t *smoothened_image = S1_smoothen(input_image);
            auto smooth_end = std::chrono::high_resolution_clock::now();

            // Write smoothened image data directly to the pipe
            for (int h = 0; h < smoothened_image->height; h++) {
                write(pipefd[1], smoothened_image->image_pixels[h], 
                      smoothened_image->width * 3 * sizeof(unsigned char)); // Assuming 3 channels
            }

            double smooth_time = std::chrono::duration<double>(smooth_end - smooth_start).count();
            // cout << "Smoothing Iteration " << i << " completed in " << smooth_time << " seconds." << endl;

            free_image(smoothened_image); // Free after usage
        }

        close(pipefd[1]); // Close write end after writing
        wait(nullptr); // Wait for child processes to complete

    } else {  // Child process 1: Detail Extraction
        pid_t pid2 = fork();
        if (pid2 < 0) {
            perror("fork failed");
            return 1;
        }

        if (pid2 > 0) { // C1: Detail extraction
            close(pipefd[1]);  // Close write end of pipefd (smooth -> details)
            close(pipefd2[0]); // Close read end of pipefd2 (details -> sharpening)

            struct image_t *smoothened_image = allocate_image(input_image->width, input_image->height);
            for (int i = 0; i < 1000; i++) {
                // Read smoothened image data from the pipe
                for (int h = 0; h < smoothened_image->height; h++) {
                    read(pipefd[0], smoothened_image->image_pixels[h], 
                         smoothened_image->width * 3 * sizeof(unsigned char));
                }

                // Perform detail extraction
                auto details_start = std::chrono::high_resolution_clock::now();
                struct image_t *details_image = S2_find_details(input_image, smoothened_image);
                auto details_end = std::chrono::high_resolution_clock::now();

                // Write details image data directly to the second pipe
                for (int h = 0; h < details_image->height; h++) {
                    write(pipefd2[1], details_image->image_pixels[h], 
                          details_image->width * 3 * sizeof(unsigned char));
                }

                double details_time = std::chrono::duration<double>(details_end - details_start).count();
                // cout << "Detail Extraction Iteration " << i << " completed in " << details_time << " seconds." << endl;

                free_image(details_image); // Free after usage
            }

            free_image(smoothened_image); // Free after usage
            close(pipefd2[1]);
        } else { // C2: Sharpening
            close(pipefd[0]); // Close unused pipe ends
            close(pipefd[1]);
            close(pipefd2[1]); // Close write end of pipefd2 (details -> sharpening)

            struct image_t *details_image = allocate_image(input_image->width, input_image->height);
            for (int i = 0; i < 1000; i++) {
                // Read details image data from the second pipe
                for (int h = 0; h < details_image->height; h++) {
                    read(pipefd2[0], details_image->image_pixels[h], 
                         details_image->width * 3 * sizeof(unsigned char));
                }

                // Perform sharpening
                auto sharpen_start = std::chrono::high_resolution_clock::now();
                struct image_t *sharpened_image = S3_sharpen(input_image, details_image);
                auto sharpen_end = std::chrono::high_resolution_clock::now();

                if (i == 999) { // Final iteration, write the output image to file
                    write_ppm_file(argv[2], sharpened_image);
                }

                double sharpen_time = std::chrono::duration<double>(sharpen_end - sharpen_start).count();
                // cout << "Sharpening Iteration " << i << " completed in " << sharpen_time << " seconds." << endl;

                free_image(sharpened_image); // Free after usage
            }

            free_image(details_image); // Free after usage
            close(pipefd2[0]);
        }
    }

    // Free input image
    free_image(input_image);
    auto read_end = std::chrono::high_resolution_clock::now();
    double total_read_time = std::chrono::duration<double>(read_end - read_start).count();

    // Output timing statistics
    cout << "Total Read Time: " << total_read_time << " seconds." << endl;

    return 0;
}
