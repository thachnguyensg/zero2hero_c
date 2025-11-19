TARGET = bin/dbview
SRC = $(wildcard src/*.c)
OBJ = $(patsubst src/%.c, obj/%.o, $(SRC))
CFLAGS = -g -Iinclude

run: clean default
	./$(TARGET) -f ./newdb.db -n
	./$(TARGET) -f ./newdb.db

default: $(TARGET)

clean:
	rm -f obj/*.o
	rm -f bin/*
	rm -f *.db

$(TARGET): $(OBJ)
	gcc -o $@ $?

obj/%.o: src/%.c
	gcc $(CFLAGS) -c $< -o $@
