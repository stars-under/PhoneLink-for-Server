CC = g++
LD = g++
SRCS = $(wildcard *.cpp tools/src/*.cpp tools/src/*/*.cpp)
OBJS = $(patsubst %cpp, $(BUILD)/%o, $(SRCS))
RELY_ON = $(patsubst %cpp, $(BUILD)/%d, $(SRCS))
# -I指定头文件目录
INCLUDE = -I./tools/include -I./tools/include/SyncTools
# -L指定库文件目录，-l指定静态库名字(去掉文件名中的lib前缀和.a后缀)
LIB = -L./tools/scr -lpthread
# 开启编译warning和设置优化等级
CFLAGS = -O0 -g3

CWARNIG = -Wno-conversion-null -Wno-write-strings -Wno-pointer-arith -Wno-format-security

BUILD = build

TARGET = PhoneLink

.PHONY:all clean

all:  $(TARGET)

$(TARGET): $(OBJS)
	$(LD) -o $@ $^ $(LIB)

$(BUILD)/%.d:%.cpp
	echo -n "$(BUILD)/" > $@ & $(CC) -MM $^ $(INCLUDE) >> $@

# 编译时候指定头文件目录
include $(RELY_ON)
$(BUILD)/%.o:%.cpp
	$(CC) -c $< -o $@ $(INCLUDE) $(CFLAGS) $(CWARNIG)

clean:
	rm -f $(OBJS) $(RELY_ON) $(TARGET)