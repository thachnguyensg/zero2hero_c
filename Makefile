TARGET_SRV = bin/dbserver

SRC_SVR = $(wildcard src/srv/*.c)
OBJ_SRV = $(patsubst src/srv/%.c, obj/srv/%.o, $(SRC_SVR))

CFLAGS = -g -Iinclude

run: clean default
	./$(TARGET_SRV) -f ./newdb.db -n -p 8080

default: $(TARGET_SRV)

clean:
	rm -f obj/srv/*.o
	rm -f bin/*
	rm -f *.db

$(TARGET_SRV): $(OBJ_SRV)
	gcc -o $@ $?

$(OBJ_SRV): obj/srv/%.o: src/srv/%.c
	@mkdir -p obj/srv
	gcc $(CFLAGS) -c $< -o $@
