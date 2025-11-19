TARGET = bin/dbview
SRC = $(wildcard src/*.c)
OBJ = $(patsubst src/%.c, obj/%.o, $(SRC))
CFLAGS = -g -Iinclude

run: clean default
	./$(TARGET) -f ./newdb.db -n
	./$(TARGET) -f ./newdb.db
	./$(TARGET) -f ./newdb.db -a "Thach Nguyen,F3 Me Coc,88"
	./$(TARGET) -f ./newdb.db -a "Thach Nguyen 2,F3 Me Coc,88"
	./$(TARGET) -f ./newdb.db -a "Thach Nguyen 3,F3 Me Coc,88" -l
	./$(TARGET) -f ./newdb.db -r "Thach Nguyen"
	./$(TARGET) -f ./newdb.db -l
	./$(TARGET) -f ./newdb.db -a "Thach Nguyen,F3 Me Coc,88"
	./$(TARGET) -f ./newdb.db -l

default: $(TARGET)

clean:
	rm -f obj/*.o
	rm -f bin/*
	rm -f *.db

$(TARGET): $(OBJ)
	gcc -o $@ $?

obj/%.o: src/%.c
	gcc $(CFLAGS) -c $< -o $@
