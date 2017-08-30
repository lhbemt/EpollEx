cc = g++
flags = -g -Wall -std=c++11 -pthread
objs = main.o ctcpclient.o
target = client

$(target) : $(objs)
	$(cc) $(flags) -o $@ $^

%.o : %.cpp
	$(cc) $(flags) -c $<

clean:
	rm -rf $(target) $(objs)
