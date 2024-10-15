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
int main(int argc, char **argv) {
    struct image_t *input_image = read_ppm_file(argv[1]);
    int pipefd1[2], pipefd2[2];  // Two pipes for communication
    pid_t c1, c2;
	double total_read_time = 0.0;
	auto read_start = std::chrono::high_resolution_clock::now();
    // Create the pipes
    if (pipe(pipefd1) == -1 || pipe(pipefd2) == -1) {
        perror("pipe");
        exit(EXIT_FAILURE);
    }
    if (argc != 3) {
        std::cout << "usage: ./a.out <path-to-original-image> <path-to-transformed-image>\n\n";
        exit(0);
    }

    double total_read_time = 0.0;
    double total_smoothen_time = 0.0;
    double total_details_time = 0.0;
    double total_sharpen_time = 0.0;
	double total_free_time = 0.0;
    const int iterations = 1000;
	image_t *sharpened_image;
    // Measure read time

    c1 = fork();
    if (c1 == -1) {
        perror("fork");
        exit(EXIT_FAILURE);
    }
	for (int i = 0; i < iterations; i++) {
		if (c1 > 0) {  // Parent (c1 PARENT)
			close(pipefd1[0]);  // Close unused read end of pipefd1
			
			// Smoothening image
			auto smoothen_start = std::chrono::high_resolution_clock::now();
			image_t *smoothened_image = S1_smoothen(input_image);
			auto smoothen_end = std::chrono::high_resolution_clock::now();
			total_smoothen_time += std::chrono::duration<double>(smoothen_end - smoothen_start).count();
			
			// Write smoothened image to pipe
			write(pipefd1[1], smoothened_image, sizeof(struct image_t));  
			close(pipefd1[1]);  // Close the write end of pipefd1 after writing

		} 
		else {  // Child (c1 CHILD)
			close(pipefd1[1]);  // Close unused write end of pipefd1
			
			// Receive smoothened image from parent
			struct image_t smoothened_image;
			read(pipefd1[0], &smoothened_image, sizeof(struct image_t));  
			close(pipefd1[0]);  // Close read end of pipefd1 after reading
			
			// Finding details
			auto details_start = std::chrono::high_resolution_clock::now();
			image_t *details_image = S2_find_details(input_image, smoothened_image);
			auto details_end = std::chrono::high_resolution_clock::now();
			total_details_time += std::chrono::duration<double>(details_end - details_start).count();

			c2 = fork();
			if (c2 == -1) {
				perror("fork");
				exit(EXIT_FAILURE);
			}

			if (c2 > 0) {  // Parent (c2 PARENT)
				close(pipefd2[0]);  // Close unused read end of pipefd2
				
				// Write detailed image to pipe
				write(pipefd2[1], details_image, sizeof(struct image_t));  
				close(pipefd2[1]);  // Close the write end of pipefd2 after writing

			} 
			else {  // Child (c2 CHILD)
				close(pipefd2[1]);  // Close unused write end of pipefd2
				
				// Receive detailed image from parent
				struct image_t detailed_image;
				read(pipefd2[0], &detailed_image, sizeof(struct image_t));  
				close(pipefd2[0]);  // Close read end of pipefd2 after reading
				
				// Sharpening
				auto sharpen_start = std::chrono::high_resolution_clock::now();
				sharpened_image = S3_sharpen(input_image, details_image);
				auto sharpen_end = std::chrono::high_resolution_clock::now();
				total_sharpen_time += std::chrono::duration<double>(sharpen_end - sharpen_start).count();
				
				// Write the sharpened image to the output file
				
			}

		}
		auto free_start = std::chrono::high_resolution_clock::now();
		// Free the images to prevent memory leaks
		if(i != iterations-1) {
		free_image(smoothened_image);
		free_image(details_image);
		free_image(sharpened_image);
		}
		auto free_end = std::chrono::high_resolution_clock::now();
		total_free_time += std::chrono::duration<double>(free_end - free_start).count();
	}
	write_ppm_file(argv[2], sharpened_image);
    // Free input image memory after all iterations
    free_image(input_image1);
	auto read_end = std::chrono::high_resolution_clock::now();
    total_read_time += std::chrono::duration<double>(read_end - read_start).count();
    // Output timing statistics
    std::cout << "Total Read Time: " << total_read_time << "s\n";
	std::cout << "Total Free Time: " << total_free_time << "s\n";
    std::cout << "Total Smoothen Time: " << total_smoothen_time << "s\n";
    std::cout << "Total Find Details Time: " << total_details_time << "s\n";
    std::cout << "Total Sharpen Time: " << total_sharpen_time << "s\n";

    return 0;
}


// #include <iostream>
// #include "libppm.h"
// #include <cstdint>
// #include <string.h>
// #include <chrono>
// using namespace std;

// struct image_t* S1_smoothen(struct image_t *input_image)
// {
// 	// TODO
// 	// remember to allocate space for smoothened_image. See read_ppm_file() in libppm.c for some help.
// 	int w1 = input_image->width;
// 	int h1 = input_image->height;
// 	struct image_t* new_image = new struct image_t;
// 	new_image->height = h1;
// 	new_image->width = w1;

// 	// Allocate memory for the image pixel data
// 	new_image->image_pixels = new uint8_t**[h1];
// 	for (int i = 0; i < h1; i++) {
// 		new_image->image_pixels[i] = new uint8_t*[w1];
// 		for (int j = 0; j < w1; j++) {
// 			new_image->image_pixels[i][j] = new uint8_t[3];
// 		}
// 	}

// 	// Perform the smoothing operation
// 	for (int i = 0; i < h1; i++) {
// 		for (int j = 0; j < w1; j++) {
// 			for (int k = 0; k < 3; k++) {  

// 				int cnt = 0;
// 				int val = 0;

// 				for (int di = -1; di <= 1; di++) {
// 					for (int dj = -1; dj <= 1; dj++) {
// 						int ni = i + di;
// 						int nj = j + dj;
// 						if (ni >= 0 && ni < h1 && nj >= 0 && nj < w1) {
// 							cnt++;
// 							val += input_image->image_pixels[ni][nj][k];
// 						}
// 					}
// 				}
// 				new_image->image_pixels[i][j][k] = val / cnt;
// 			}
// 		}
// 	}
// 	return new_image;
// }

// struct image_t* S2_find_details(struct image_t *input_image, struct image_t *smoothened_image)
// {
// 	// TODO
// 	int w1 = input_image->width;
// 	int h1 = input_image->height;
// 	struct image_t* new_image = new struct image_t;
// 	new_image->height = h1;
// 	new_image->width = w1;

// 	// Allocate memory for the image pixel data
// 	new_image->image_pixels = new uint8_t**[h1];
// 	for (int i = 0; i < h1; i++) {
// 		new_image->image_pixels[i] = new uint8_t*[w1];
// 		for (int j = 0; j < w1; j++) {
// 			new_image->image_pixels[i][j] = new uint8_t[3];
// 		}
// 	}
// 	for (int i = 0; i < h1; i++) {
// 		for (int j = 0; j < w1; j++) {
// 			for (int k = 0; k < 3; k++) {  
// 				int detail_value = input_image->image_pixels[i][j][k]- smoothened_image->image_pixels[i][j][k];
// 				if (detail_value < 0) detail_value = 0;
// 				if (detail_value > 255) detail_value = 255;
// 				new_image->image_pixels[i][j][k] = detail_value;
// 			}
// 		}
// 	}
// 	return new_image;
// }

// struct image_t* S3_sharpen(struct image_t *input_image, struct image_t *details_image)
// {
// 	// TODO
// 	int w1 = input_image->width;
// 	int h1 = input_image->height;
// 	struct image_t* new_image = new struct image_t;
// 	new_image->height = h1;
// 	new_image->width = w1;

// 	// Allocate memory for the image pixel data
// 	new_image->image_pixels = new uint8_t**[h1];
// 	for (int i = 0; i < h1; i++) {
// 		new_image->image_pixels[i] = new uint8_t*[w1];
// 		for (int j = 0; j < w1; j++) {
// 			new_image->image_pixels[i][j] = new uint8_t[3];
// 		}
// 	}
// 	for (int i = 0; i < h1; i++) {
// 		for (int j = 0; j < w1; j++) {
// 			for (int k = 0; k < 3; k++) {  
// 				int sharp_value = input_image->image_pixels[i][j][k]+ details_image->image_pixels[i][j][k];
// 				if (sharp_value < 0) sharp_value = 0;
// 				if (sharp_value > 255) sharp_value = 255;
// 				new_image->image_pixels[i][j][k] = sharp_value;
				
// 			}
// 		}
// 	}
// 	return new_image;
// }

// #include <iostream>
// #include <chrono>
// #include <cstdlib>
// #include <cstring>

// using namespace std;

// // Assuming these functions and struct are defined elsewhere
// struct image_t;
// struct image_t* read_ppm_file(const char* path);
// struct image_t* S1_smoothen(struct image_t* img);
// struct image_t* S2_find_details(struct image_t* input, struct image_t* smooth);
// struct image_t* S3_sharpen(struct image_t* input, struct image_t* details);
// void write_ppm_file(const char* path, struct image_t* img);
// void free(struct image_t* img); // Assuming there's a free function for image_t

// int main(int argc, char **argv) {
//     if (argc != 3) {
//         cout << "usage: ./a.out <path-to-original-image> <path-to-transformed-image>\n\n";
//         exit(0);
//     }

//     double total_read_time = 0.0;
//     double total_smoothen_time = 0.0;
//     double total_details_time = 0.0;
//     double total_sharpen_time = 0.0;
//     const int iterations = 1000;

//     // Measure read time
//     auto read_start = chrono::high_resolution_clock::now();
//     struct image_t *input_image1 = read_ppm_file(argv[1]);
//     auto read_end = chrono::high_resolution_clock::now();
//     total_read_time += chrono::duration<double>(read_end - read_start).count();

//     struct image_t *input_image = input_image1;

//     // Loop for repeated processing
//     for (int i = 0; i < iterations; i++) {
//         auto start_time69 = chrono::high_resolution_clock::now();
//         input_image = input_image1; // Reset to original for each iteration

//         // Smoothening
//         auto smoothen_start = chrono::high_resolution_clock::now();
//         struct image_t *smoothened_image = S1_smoothen(input_image);
//         auto smoothen_end = chrono::high_resolution_clock::now();
//         total_smoothen_time += chrono::duration<double>(smoothen_end - smoothen_start).count();

//         // Finding details
//         auto details_start = chrono::high_resolution_clock::now();
//         struct image_t *details_image = S2_find_details(input_image, smoothened_image);
//         auto details_end = chrono::high_resolution_clock::now();
//         total_details_time += chrono::duration<double>(details_end - details_start).count();

//         // Sharpening
//         auto sharpen_start = chrono::high_resolution_clock::now();
//         struct image_t *sharpened_image = S3_sharpen(input_image, details_image);
//         auto sharpen_end = chrono::high_resolution_clock::now();
//         total_sharpen_time += chrono::duration<double>(sharpen_end - sharpen_start).count();

//         // Write output file
//         write_ppm_file(argv[2], sharpened_image);

//         // Clear the structs
//         memset(input_image, 0, sizeof(struct image_t));
//         memset(smoothened_image, 0, sizeof(struct image_t));
//         memset(details_image, 0, sizeof(struct image_t));
//         memset(sharpened_image, 0, sizeof(struct image_t));
//         auto end_time69 = chrono::high_resolution_clock::now();
//         total_read_time += chrono::duration<double>(end_time69 - start_time69).count();
//     }

//     // Output timing statistics
//     cout << "Total Read Time: " << total_read_time << "s\n";
//     cout << "Total Smoothen Time: " << total_smoothen_time << "s\n";
//     cout << "Total Find Details Time: " << total_details_time << "s\n";
//     cout << "Total Sharpen Time: " << total_sharpen_time << "s\n";


//     return 0;

// }
