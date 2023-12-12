build:
	gcc clk.c -o clk.out 
	gcc process_generator.c -o process_generator.out 
	gcc scheduler.c -o scheduler.out -lm
	gcc process.c -o process.out 
	gcc test_generator.c -o test_generator.out 

clean:
	# rm -f core.*
	# rm -f core
	# ./killprocess.sh;
	rm -f *.out
	# ipcrm -a  
all: clean build
run:
	./process_generator.out
testgenerate: 
	./test_generator.out 

