build:
	gcc clk.c -o clk.out -g
	gcc process_generator.c -o process_generator.out -g
	gcc scheduler.c -o scheduler.out -g
	gcc process.c -o process.out -g
	gcc test_generator.c -o test_generator.out -g

clean:
	rm -f core.*
	rm -f core
	./killprocess.sh;
	rm -f *.out
	ipcrm -a  
all: clean build
run:
	./process_generator.out
testgenerate: 
	./test_generator.out 

