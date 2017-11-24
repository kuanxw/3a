# NAME: Kuan Xiang Wen
# EMAIL: kuanxw@g.ucla.edu
# ID: 004461554

# NAME: Amir Saad
# EMAIL: 
# ID: 604840359

default:
	gcc lab3a.c -o lab3a -Wall -Wextra
clean:
	rm -f lab3a-004461554-604840359.tar.gz lab3a
dist:
	tar -cvzf lab3a-004461554-604840359.tar.gz lab3a.c Makefile README
check: dist
	./P3A_check.sh 004461554 604840359