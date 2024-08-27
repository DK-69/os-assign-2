#include <iostream>
#include <fstream>
#include <cstring>
#include <unistd.h>
#include <signal.h>
#include <cstdlib> // For atoi

using namespace std;

int main(int argc, char **argv)
{
	if(argc != 5)
	{
		cout << "usage: ./searcher.out <path-to-file> <pattern> <search-start-position> <search-end-position>\n";
		cout << "provided arguments:\n";
		for(int i = 0; i < argc; i++)
			cout << argv[i] << "\n";
		return -1;
	}
	
	char *file_to_search_in = argv[1];
	char *pattern_to_search_for = argv[2];
	int search_start_position = atoi(argv[3]);
	int search_end_position = atoi(argv[4]);
	
	ifstream givenfile(file_to_search_in, ios::binary);
	if (!givenfile.is_open()) {
		cout << "Error opening file: " << file_to_search_in << endl;
		return -1;
	}
	
	size_t length = search_end_position - search_start_position + 1;
	if (length <= 0) {
		cout << "Invalid range: end position must be greater than or equal to start position." << endl;
		return -1;
	}

	int len_pattern = strlen(pattern_to_search_for);
	char* buffer = new char[length];
	givenfile.seekg(search_start_position, ios::beg);
	givenfile.read(buffer, length);
	givenfile.close();

	size_t i = 0;
	while (i + len_pattern <= length) {
		int j = 0;
		while (j < len_pattern && buffer[i + j] == pattern_to_search_for[j]) {
			j++;
		}
		if (j == len_pattern) {
			cout << getpid() << " found at " << (search_start_position + i) << endl;
			delete[] buffer;
			return 1;
		}
		i++;
	}

	cout << "[-1] didn't find\n";
	delete[] buffer;
	return 0;
}
