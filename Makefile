# NAME: Kuan Xiang Wen
# EMAIL: kuanxw@g.ucla.edu
# ID: 004461554

default:
	gcc lab2_add.c -o lab2_add -pthread -lrt -Wall -Wextra
	gcc -c lab2_list.c
	gcc -c SortedList.c
	gcc lab2_list.o SortedList.o -o lab2_list -pthread -lrt -Wall -Wextra
	rm -f lab2_list.o SortedList.o
clean:
	rm -f lab2a-004461554.tar.gz lab2_add lab2_list
	
dist: default tests graphs
	-tar -cvzf lab2a-004461554.tar.gz lab2_add.c lab2_list.c SortedList.h SortedList.c Makefile README \
	lab2_add.csv lab2_add-1.png lab2_add-2.png lab2_add-3.png lab2_add-4.png lab2_add-5.png \
	lab2_list.csv lab2_list-1.png lab2_list-2.png lab2_list-3.png lab2_list-4.png \
	lab2_list.gp lab2_add.gp
check: dist
	./P2A_check.sh 004461554