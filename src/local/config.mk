ROOT_DIR ?= .
CXXFLAGS ?=  -O0 #-fsanitize=thread # -fsanitize=address
CXX = g++
SHELL = /bin/bash
UNAME := $(shell uname)
PC_TYPE ?= WITH_PAPI_LL
PAPI_DIR := /home/yeongilk/papi/install
CXX_VERSION ?= c++11
