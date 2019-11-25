ROOT_DIR ?= .
CXXFLAGS ?=
CXX = g++
SHELL = /bin/bash
UNAME := $(shell uname)
PC_TYPE ?= WITH_PAPI_LL
PAPI_DIR := /home/mbuch/papi/install
CXX_VERSION ?= c++11
