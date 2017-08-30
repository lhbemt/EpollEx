currdir = $(shell pwd)
clientdir = $(currdir)/Client
serverdir = $(currdir)/Server

all : client server

client :
	$(MAKE) -C $(clientdir)

server :
	$(MAKE) -C $(serverdir)

clean  :
	$(MAKE) -C $(clientdir) clean
	$(MAKE) -C $(serverdir) clean
