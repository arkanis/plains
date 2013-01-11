test: test.c syscall-reporter.o
	gcc -g test.c syscall-reporter.o -o test

syscall-reporter.o: syscall-reporter.c syscall-reporter.h syscall-names.h
	gcc -g -c syscall-reporter.c

syscall-names.h: /usr/include/sys/syscall.h
	echo "static const char *syscall_names[] = {" > $@ ;\
	echo "#include <sys/syscall.h>" | cpp -dM | grep '^#define __NR_' | \
		LC_ALL=C sed -r -n -e 's/^\#define[ \t]+__NR_([a-z0-9_]+)[ \t]+([0-9]+)(.*)/ [\2] = "\1",/p' >> $@ ;\
	echo "};" >> $@

clean:
	rm -f syscall-names.h core test *.o