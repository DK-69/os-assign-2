# Compiler and flags
CC = g++
CFLAGS = -g -Wall

# Targets
all: build-part1 build-part2 build-part3

# Part 1 build
build-part1: part1.out

part1.out: part1_searcher.cpp
	$(CC) $(CFLAGS) part1_searcher.cpp -o part1.out

# Part 1 run
run-part1: part1.out
	./part1.out file.txt NGTNIJGK 0 67108863

# Part 2 build
build-part2: part2_searcher.out part2_partitioner.out

part2_searcher.out: part2_searcher.cpp
	$(CC) $(CFLAGS) part2_searcher.cpp -o part2_searcher.out

part2_partitioner.out: part2_partitioner.cpp
	$(CC) $(CFLAGS) part2_partitioner.cpp -o part2_partitioner.out

# Part 2 run example
run-part2: part2_partitioner.out
	./part2_partitioner.out file.txt NGTNIJGK 0 67108863 32

# Part 3 build
build-part3: part3_searcher.out part3_partitioner.out

part3_searcher.out: part3_searcher.cpp
	$(CC) $(CFLAGS) part3_searcher.cpp -o part3_searcher.out

part3_partitioner.out: part3_partitioner.cpp
	$(CC) $(CFLAGS) part3_partitioner.cpp -o part3_partitioner.out

# Part 3 run example
run-part3: part3_partitioner.out
	./part3_partitioner.out file.txt NGTNIJGK 0 67108863 8388608

# Cleaning targets
clean-part1:
	rm -f part1.out

clean-part2:
	rm -f part2_searcher.out part2_partitioner.out

clean-part3:
	rm -f part3_searcher.out part3_partitioner.out

clean: clean-part1 clean-part2 clean-part3