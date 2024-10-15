build-sharpen: ./a.out

./a.out: part1.cpp libppm.cpp
	g++ -g part1.cpp libppm.cpp -o a.out

run-sharpen: ./a.out
	./a.out images/$(INPUT).ppm images/$(OUTPUT).ppm

build-part2_1: part2_1.cpp libppm.cpp
	g++ -g part2_1.cpp libppm.cpp -o part2_1.out

run-part2_1: build-part2_1
	./part2_1.out images/1.ppm images/dk1.ppm

clean:
	rm -f part2_1.out


# part2_2: build-part2_2 run-part2_2

# build-part2_2: part2_2.cpp libppm.cpp
# 	g++ -g part2_2.cpp libppm.cpp -o part2_2.out

# run-part2_2: part2_2.out
# 	./part2_2.out images/1.ppm images/output_part2_2.ppm

# part2_3: build-part2_3 run-part2_3

# build-part2_3: part2_3.cpp libppm.cpp
# 	g++ -g part2_3.cpp libppm.cpp -o part2_3.out -lpthread

# run-part2_3: part2_3.out
# 	./part2_3.out images/1.ppm images/output_part2_3.ppm

# clean:
# 	rm -f part2_1.out part2_2.out part2_3.out
