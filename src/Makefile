all: init monlog

init: 
	mkdir -p build
	touch build/results.txt

monlog: 
	gcc -o build/monlog monlog.c $(DFLAGS) -lpthread
	
%.o: %.c
	gcc -c -o $@ $< $(DFLAGS) -lpthread	

clean:
	rm build/*

.PHONY: debug
debug: DFLAGS = -g
debug: clean monlog