#NAME: JIAXUAN XUE,Felix Gan
#EMAIL: nimbusxue1015@g.ucla.edu,felixgan@g.ucla.edu
#ID: 705142227,205127385

default:
	gcc -Wall -Wextra lab3a.c -o lab3a

dist:
	tar -cvzf lab3a-705142227.tar.gz lab3a.c Makefile README ext2_fs.h

clean:
	rm -f lab3a-705142227.tar.gz lab3a