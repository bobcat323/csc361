all: \
lazysend

compile: sws.c
	gcc -o sws.out sws.c

lazysend: 
	gcc -o sws.out sws.c
	./sws.out 8082 sws/www

v2:
	gcc -o sws.out sws.c
	./sws.out $(PORT) $(DIR)


clean:
	rm sws.out
