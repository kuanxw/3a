# NAME: Kuan Xiang Wen, Amir Saad
# EMAIL: kuanxw@g.ucla.edu, arsaad@g.ucla.edu
# ID: 004461554, 604840359

default:
	gcc lab3a.c -o lab3a -Wall -Wextra -std=gnu11
clean:
	rm -f lab3a-004461554-604840359.tar.gz lab3a out.csv
dist:
	tar -cvzf lab3a-004461554-604840359.tar.gz lab3a.c Makefile README ext2_fs.h
check: dist
	./P3A_check.sh 004461554 604840359