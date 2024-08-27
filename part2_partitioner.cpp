#include <iostream>
#include <fstream>
#include <cstring>  // Required for strlen
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include <cstdlib>  // Required for atoi

using namespace std;

int main(int argc, char **argv)
{
    if(argc != 6)
    {
        cout <<"usage: ./partitioner.out <path-to-file> <pattern> <search-start-position> <search-end-position> <max-chunk-size>\nprovided arguments:\n";
        for(int i = 0; i < argc; i++)
            cout << argv[i] << "\n";
        return -1;
    }
    
    char *file_to_search_in = argv[1];
    char *pattern_to_search_for = argv[2];
    int search_start_position = atoi(argv[3]);
    int search_end_position = atoi(argv[4]);
    int max_chunk_size = atoi(argv[5]);
    
    ifstream givenfile(file_to_search_in, ios::binary);
    if (!givenfile.is_open()) {
        cout << "Error opening file: " << file_to_search_in << endl;
        return -1;
    }
    
    size_t length = search_end_position - search_start_position + 1;
    char* buffer = new char[length];
    givenfile.seekg(search_start_position, ios::beg);
    givenfile.read(buffer, length);
    givenfile.close(); 

    pid_t my_pid = getpid(); 
    long int mid = (search_start_position + search_end_position) / 2;

    cout << "[" << my_pid << "] start position = " << search_start_position << " ; end position = " << search_end_position << "\n"; // Log start and end positions

    if(length > static_cast<size_t>(max_chunk_size))
    {
        pid_t left_child = fork();
        if (left_child == 0)
        {
            // Left child process
            string rightstr = to_string(search_start_position);
            string rightstr_end = to_string(mid); 
            const char* str2[] = {"part2_partitioner.out", file_to_search_in, pattern_to_search_for, rightstr.c_str(), rightstr_end.c_str(), to_string(max_chunk_size).c_str(), nullptr}; // Fixed the conversion
            execv("./part2_partitioner.out", const_cast<char* const*>(str2));

            perror("execv failed for left child");
            exit(EXIT_FAILURE);
        }
        else
        {
            cout << "[" << my_pid << "] forked left child " << left_child << "\n"; // Log the PID of the left child

            pid_t right_child = fork();
            if(right_child == 0)
            {
                // Right child process
                string rightstr = to_string(mid + 1);
                string rightstr_end = to_string(search_end_position);
                const char* str2[] = {"part2_partitioner.out", file_to_search_in, pattern_to_search_for, rightstr.c_str(), rightstr_end.c_str(), to_string(max_chunk_size).c_str(), nullptr}; // Fixed the conversion
                execv("./part2_partitioner.out", const_cast<char* const*>(str2));

                perror("execv failed for right child");
                exit(EXIT_FAILURE);
            }
            else
            {
                cout << "[" << my_pid << "] forked right child " << right_child << "\n"; // Log the PID of the right child

                int status;
                waitpid(left_child, &status, 0); 
                cout << "[" << my_pid << "] left child returned\n";
                
                waitpid(right_child, &status, 0); 
                cout << "[" << my_pid << "] right child returned\n";
            }
        }
    }
    else
    {
        string rightstr = to_string(search_start_position);
        string rightstr_end = to_string(search_end_position);
        const char* str2[] = {"part2_searcher.out", file_to_search_in, pattern_to_search_for, rightstr.c_str(), rightstr_end.c_str(), nullptr};

        pid_t searcher_pid = fork(); 
        if (searcher_pid == 0)
        {
            // In the child process
            execv("./part2_searcher.out", const_cast<char* const*>(str2));

            // If execv fails:
            perror("execv failed for searcher child");
            exit(EXIT_FAILURE);
        }
        else
        {
            cout << "[" << my_pid << "] forked searcher child " << searcher_pid << "\n"; // Log the PID of the searcher child

            int status;
            waitpid(searcher_pid, &status, 0); 
            cout << "[" << my_pid << "] searcher child returned \n";
        }
    }

    delete[] buffer; // Clean up buffer memory
    return 0;
}
