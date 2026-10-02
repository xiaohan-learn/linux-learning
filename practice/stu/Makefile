student: stu.o main.o
	gcc stu.o main.o -o student

stu.o: stu.c stu.h
	gcc -Wall -Wextra -c stu.c

main.o: main.c stu.h
	gcc -Wall -Wextra -c main.c

clean:
	rm -f *.o student
