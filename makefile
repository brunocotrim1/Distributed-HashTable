PROTOC_DIR=/usr/local/
OBJ_dir = object
OBJECTOS = data.o entry.o table.o list.o  serialization.o message-private.o 
data.o = data.h
entry.o = entry.h 
list.o = list.h list-private.h
table.o = table.h table-private.h
serialization.o = serialization.h
CC = gcc
CLEAN = data.o entry.o table.o list.o  serialization.o message-private.o client_stub.o network_client.o message-private.o table_skel.o network_server.o table_server.o table_client.o sdmessage.pb-c.o

client_stub.o = client_stub.h client_stub-private.h network_client.h serialization.h

network_client.o = network_client.h client_stub-private.h message-private.h

message-private.o = message-private.h

table_skel.o = table_skel.h serialization.h
network_server.o = network_server.h message-private.h 



table_server.o = network_server.h
table_client.o = client_stub.h
sdmessage.pb-c.o = sdmessage.pb-c.h


all: table_client table_server

client-lib.o: sdmessage.pb-c.o client_stub.o network_client.o
		ld -r ./object/sdmessage.pb-c.o ./object/client_stub.o ./object/network_client.o -o ./lib/client-lib.o 



table_client: $(OBJECTOS) table_client.o client-lib.o
		$(CC) -D THREADED $(addprefix $(OBJ_dir)/,$(OBJECTOS)) ./lib/client-lib.o object/table_client.o /usr/local/lib/libprotobuf-c.a -o binary/table_client -lzookeeper_mt



table_server: $(OBJECTOS) table_server.o table_skel.o network_server.o
		$(CC) -D THREADED  $(addprefix $(OBJ_dir)/,$(OBJECTOS)) object/table_skel.o ./object/network_client.o object/network_server.o object/table_server.o object/sdmessage.pb-c.o /usr/local/lib/libprotobuf-c.a -o binary/table_server -lpthread -lzookeeper_mt


%.pb-c.c:%.proto
	${PROTOC_DIR}bin/protoc-c ./sdmessage.proto --c_out=./include


sdmessage.pb-c.o: sdmessage.pb-c.c $($@)
		$(CC) -I include -o object/$@ -c ./include/$<


%.o: source/%.c $($@)
		$(CC) -I include -o $(OBJ_dir)/$@ -c $< -lzookeeper_mt -D THREADED 


clean:
		rm -f $(addprefix $(OBJ_dir)/,$(CLEAN)) binary/table_client binary/table_server lib/client-lib.o

