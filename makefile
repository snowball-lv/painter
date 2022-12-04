
BIN = bin/paint
SRCS = $(wildcard src/*.cpp)
OBJS = $(SRCS:src/%.cpp=out/%.o)
DEPS = $(SRCS:src/%.cpp=out/%.d)

CFLAGS = -c -MMD -I inc -Wall -O2

all: $(BIN)

-include $(DEPS)

out:
	mkdir out

out/%.o: src/%.cpp | out
	$(CXX) $(CFLAGS) $< -o $@

bin:
	mkdir bin

$(BIN): $(OBJS) | bin
	$(CXX) $^ -o $@

clean:
	rm -rf out bin

test: all
	$(BIN)
	# $(BIN) -g 8 in.ppm
	# $(BIN) -k 16 in.ppm out.ppm