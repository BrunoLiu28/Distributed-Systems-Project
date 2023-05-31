CC = gcc
LD= ld
INC_DIR = include
OBJ_DIR = object
BIN_DIR = binary
SRC_DIR = source
LIB_DIR = lib

tree-server = tree_server.o data.o entry.o tree.o message.o tree_skel.o network_server.o sdmessage.pb-c.o client_stub.o network_client.o
tree-client = tree_client.o data.o entry.o message.o client_stub.o network_client.o sdmessage.pb-c.o 
client-lib = client_stub.o network_client.o data.o entry.o message.o sdmessage.pb-c.o 

data.o = data.h
entry.o = data.h entry.h
tree.o = data.h tree.h tree-private.h
tree_skel.o = message-private.h sdmessage.pb-c.h tree.h tree_skel.h network_server.h client_stub.h client_stub-private.h
tree_server.o = network_server.h tree_skel.h
tree_client.o = entry.h data.h client_stub.h client_stub-private.h
network_client.o = client_stub.h client_stub-private.h sdmessage.pb-c.h network_client.h inet.h message-private.h
network_server.o = inet.h message-private.h tree_skel.h network_server.h
message.o = inet.h sdmessage.pb-c.h message-private.h
client_stub.o = client_stub-private.h data.h entry.h client_stub.h network_client.h message-private.h sdmessage.pb-c.h

all: client-lib.o tree-client tree-server

%.o: $(SRC_DIR)/%.c
	$(CC) -g -Wall -I $(INC_DIR) -o $(OBJ_DIR)/$@ -c $<

client-lib.o: $(client-lib)
	ld -r $(addprefix $(OBJ_DIR)/,$^) -o $(LIB_DIR)/$@

tree-client: $(tree-client)
	$(CC)  -D THREADED $(addprefix $(OBJ_DIR)/,$^) $(addprefix -I/,$(INC_DIR)) $(addprefix -L/,$(LIB_DIR)) -lprotobuf-c -o $(BIN_DIR)/$@ -lpthread -lzookeeper_mt

tree-server: $(tree-server)
	$(CC) -D THREADED $(addprefix $(OBJ_DIR)/,$^) $(addprefix -I/,$(INC_DIR)) $(addprefix -L/,$(LIB_DIR)) -lprotobuf-c -o $(BIN_DIR)/$@ -lpthread -lzookeeper_mt

clean:
	rm -f $(OBJ_DIR)/*.o
	rm -f $(BIN_DIR)/*
	rm -f $(LIB_DIR)/*
