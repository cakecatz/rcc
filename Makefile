rcc: rcc.c

test: rcc
	./rcc -test
	./test.sh

clean:
	rm -rf rcc *.o *~ tmp*