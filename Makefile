CC=gcc
CXX=g++
RM=rm -f
INCLS=-I/usr/lib/arm-linux-gnueabihf/
CPPFLAGS= -std=c++14 -g
LDFLAGS=
LIB_OPENCV = -L/usr/lib/arm-linux-gnueabihf \
	-lopencv_calib3d \
	-lopencv_contrib \
	-lopencv_core \
	-lopencv_features2d \
	-lopencv_flann \
	-lopencv_highgui \
	-lopencv_imgproc \
	-lopencv_ml \
	-lopencv_objdetect \
	-lopencv_photo \
	-lopencv_video \
    -lpthread

LDLIBS=  $(LIB_OPENCV)

SRCS=videorecordcapture.cpp main.cpp 
OBJS=$(subst .cpp,.o,$(SRCS))

all: tool

tool: $(OBJS)
	$(CXX) $(CPPFLAGS) $(LDFLAGS)  -o  tool  $(OBJS)  $(LDLIBS) 

depend: .depend

.depend: $(SRCS)
	$(RM) ./.depend
	$(CXX) $(CPPFLAGS) -MM $^>>./.depend;

clean:
	$(RM) $(OBJS)

distclean: clean
	$(RM) *~ .depend

include .depend
