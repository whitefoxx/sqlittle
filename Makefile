db: main.c db.h shell.h btree.h result.h statement.h
	gcc main.c -o db

test: test.c db.h shell.h btree.h result.h statement.h
	gcc test.c -o test

run: db
	./db

clean:
	rm -f db *.db

format: *.c *.h
	clang-format-3.9 -i *.c *.h

git: *.c *.h Makefile
	git add *.c *.h Makefile
