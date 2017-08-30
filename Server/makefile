cc = g++
flags = -g -Wall -std=c++11 -pthread
target = server
objs = mainserver.o ctcpserver.o

$(target) : $(objs)
	$(cc) $(flags) -o $@ $^

%.o : %.cpp
	$(cc) $(flags) -c $<

clean:
	rm -rf $(target) $(objs)
