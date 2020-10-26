VPATH = . ./objective_c
CC = gcc
CXX= g++
CFLAGS = -Wall -Wextra  -O2
CXXFLAGS = -Wall -Wextra  -O2
.PHONY: clean
target: radio-proxy

class.o: class.c class.h

my_string.o: my_string.c class.h my_string.h

net_utils.o: net_utils.c

my_error.o: my_error.c

shout-client.o: shout-client.c my_string.h class.h shout_request.h \
http.h shout_response.h net_utils.h my_error.h

shout_receive.o: shout_receive.c http.h shout_response.h net_utils.h \
my_string.h class.h my_error.h

shout_request.o: shout_request.c shout_request.h my_string.h \
class.h http.h my_error.h

shout_response.o: shout_response.c shout_response.h net_utils.h \
my_string.h class.h http.h my_error.h

http.o: http.c http.h

proxy-client.o: proxy-client.c proxy-client.h shout_response.h net_utils.h \
my_string.h http.h udp_sockets.h client-set.h udp_message.h

udp_sockets.o: udp_sockets.c

udp_message.o: udp_message.c

client-set.o: client-set.cc

multicast.o: multicast.c

ifi.o: ifi.c

sons.o: sons.c

rp_command_line.o: rp_command_line.c

SHOUT = shout-client.o shout_request.o shout_receive.o shout_response.o http.o

PROXY = proxy-client.o udp_sockets.o client-set.o udp_message.o \
multicast.o ifi.o

RADIO-PROXY = radio-proxy.o net_utils.o my_string.o class.o my_error.o sons.o\
rp_command_line.o

radio-proxy.o: radio-proxy.c net_utils.h my_string.h my_error.h
	$(CC) $^ $(CFLAGS) -c

radio-proxy: $(RADIO-PROXY) $(SHOUT) $(PROXY)
	$(CXX) $^ -o $@

clean:
	- rm *.o radio-proxy *.gch objective_c/*.gch
