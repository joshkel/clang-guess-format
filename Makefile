CXX := clang++-3.9

clang-guess-format: ClangGuessFormat.o
	$(CXX) -o $@ $^ -lclangFormat -lclangLex -lclangToolingCore -lclangRewrite -lclangBasic -lLLVM-3.9 -L/usr/lib/llvm-3.9/lib

ClangGuessFormat.o: ClangGuessFormat.cpp
	$(CXX) -o $@ -c $^ -I/usr/lib/llvm-3.9/include -Wall -std=c++14
