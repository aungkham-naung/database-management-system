CC := gcc
SRC := dberror.c storage_mgr.c buffer_mgr.c buffer_mgr_stat.c record_mgr.c btree_mgr.c expr.c rm_serializer.c test_assign4_1.c
OBJ := dberror.o storage_mgr.o buffer_mgr.o buffer_mgr_stat.o record_mgr.o btree_mgr.o expr.o rm_serializer.o test_assign4_group18.o
assignment4_f24_group18: $(OBJ)
	$(CC) -o test_assign4_group18 $(SRC)
$(OBJ): $(SRC)
	$(CC) -g -c $(SRC)
run: test_assign4_group18
	./test_assign4_group18
clean:
	rm -rf test_table_t test_assign4_group18 *.o