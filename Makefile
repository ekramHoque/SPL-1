CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -pthread
LDFLAGS = -pthread

TARGET = picodb
SERVER_TARGET = picodb_server
CLIENT_TARGET = picodb_client

SRC_DIR = src
CLIENT_DIR = client

INCLUDES = \
	-I$(SRC_DIR) \
	-I$(SRC_DIR)/commands \
	-I$(SRC_DIR)/index \
	-I$(SRC_DIR)/parser \
	-I$(SRC_DIR)/storage \
	-I$(SRC_DIR)/server

SOURCES = \
	$(SRC_DIR)/main.cpp \
	$(SRC_DIR)/commands/commands.cpp \
	$(SRC_DIR)/commands/create.cpp \
	$(SRC_DIR)/commands/insert.cpp \
	$(SRC_DIR)/commands/select.cpp \
	$(SRC_DIR)/commands/show.cpp \
	$(SRC_DIR)/commands/delete.cpp \
	$(SRC_DIR)/commands/update.cpp \
	$(SRC_DIR)/commands/utils.cpp \
	$(SRC_DIR)/index/bplusTree_index.cpp \
	$(SRC_DIR)/index/hash_index.cpp \
	$(SRC_DIR)/parser/parser.cpp \
	$(SRC_DIR)/storage/bitfield.cpp \
	$(SRC_DIR)/storage/file_manager.cpp \
	$(SRC_DIR)/storage/varint.cpp

SERVER_SOURCES = \
	$(SRC_DIR)/main_server.cpp \
	$(SRC_DIR)/server/server_socket.cpp \
	$(SRC_DIR)/server/message_protocol.cpp \
	$(SRC_DIR)/server/lock_manager.cpp \
	$(SRC_DIR)/server/client_handler.cpp \
	$(SRC_DIR)/commands/commands.cpp \
	$(SRC_DIR)/commands/create.cpp \
	$(SRC_DIR)/commands/insert.cpp \
	$(SRC_DIR)/commands/select.cpp \
	$(SRC_DIR)/commands/show.cpp \
	$(SRC_DIR)/commands/delete.cpp \
	$(SRC_DIR)/commands/update.cpp \
	$(SRC_DIR)/commands/utils.cpp \
	$(SRC_DIR)/index/bplusTree_index.cpp \
	$(SRC_DIR)/index/hash_index.cpp \
	$(SRC_DIR)/parser/parser.cpp \
	$(SRC_DIR)/storage/bitfield.cpp \
	$(SRC_DIR)/storage/file_manager.cpp \
	$(SRC_DIR)/storage/varint.cpp

CLIENT_SOURCES = \
	$(CLIENT_DIR)/client_main.cpp \
	$(SRC_DIR)/server/message_protocol.cpp

.PHONY: all cli server client

all: cli server client

cli:
	$(CXX) $(CXXFLAGS) $(INCLUDES) $(SOURCES) -o $(TARGET) $(LDFLAGS)

server:
	$(CXX) $(CXXFLAGS) $(INCLUDES) $(SERVER_SOURCES) -o $(SERVER_TARGET) $(LDFLAGS)

client:
	$(CXX) $(CXXFLAGS) $(INCLUDES) $(CLIENT_SOURCES) -o $(CLIENT_TARGET) $(LDFLAGS)

