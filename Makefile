rcc: rcc.c

test: rcc
	./test.sh

clean:
	rm -rf rcc *.o *~ tmp*