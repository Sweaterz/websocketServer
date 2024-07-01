#!/bin/bash
g++ -O2 -std=c++11 -I./websocketpp/websocketpp -o websocket_server websocketServer.cpp -lboost_system -lpthread