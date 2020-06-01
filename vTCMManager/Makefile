all: manager.o
	gcc manager.o -omanager -lsqlite3 -lpthread

manager.o: manager.c
	gcc -c manager.c

.PHONY: clean
clean:
	rm -rf manager.o manager
	rm -rf vtcm_db.sqlite3
