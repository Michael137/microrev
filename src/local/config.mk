ROOT_DIR ?= .
CXXFLAGS ?= # -fsanitize=address # -fsanitize=thread 
CXX = g++
SHELL = /bin/bash
UNAME := $(shell uname)
PC_TYPE ?= WITH_PAPI_LL
PAPI_DIR := /home/gardei/papi/install
CXX_VERSION ?= c++11
