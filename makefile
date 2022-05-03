CC = g++
LD = g++
SRCS = $(wildcard *.cpp tools/src/*.cpp tools/src/*/*.cpp)
OBJS = $(patsubst %cpp, $(BUILD)/%o, $(SRCS))
RELY_ON = $(patsubst %cpp, $(BUILD)/%d, $(SRCS))
DEFINE = -D SOCKET_PORT=2562
# -I指定头文件目录
INCLUDE = -I./tools/include -I./tools/include/SyncTools
# -L指定库文件目录，-l指定静态库名字(去掉文件名中的lib前缀和.a后缀)
LIB = -L./tools/scr -lpthread
# 开启编译warning和设置优化等级
CFLAGS = -O0 -g3
# 获得编译时间
COMPILE_TIME = $(shell date +"%Y-%M-%d %H:%M:%S")

CWARNIG = -Wno-conversion-null -Wno-write-strings -Wno-pointer-arith -Wno-format-security

BUILD = build

TARGET = PhoneLink

.PHONY:all clean

all:  $(TARGET)

$(TARGET): $(OBJS)
	$(LD) -o $@ $^ $(LIB)
	git add .
	git commit -m "make ok $(COMPILE_TIME)"

$(BUILD)/%.d:%.cpp
	echo -n "$(BUILD)/" > $@ & $(CC) -MM $^ $(INCLUDE) >> $@

# 编译时候指定头文件目录
include $(RELY_ON)
$(BUILD)/%.o:%.cpp
	$(CC) -c $< -o $@ $(INCLUDE) $(CFLAGS) $(CWARNIG) $(DEFINE)

clean:
	rm -f $(OBJS) $(RELY_ON) $(TARGET)