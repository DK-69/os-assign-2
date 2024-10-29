# Part 1
build-part1: 
	g++ -g main.cpp -o a.out

part1: build-part1
	./a.out 4096 100 fifo local combined.trace

clean_part1:
	rm -f a.out

# Part 2
build-part2: 
	g++ -g main.cpp -o lru.out

part2: build-part2
	./lru.out 4 3 lru local combined_short.trace

clean_part2:
	rm -f lru.out

