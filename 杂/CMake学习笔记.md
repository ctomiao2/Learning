**Hello World：**

1) 新建HelloWorld.cpp: 
	
	#include <iostream>
	int main() {
	    std::cout << "hello cmake" << std::endl;
	    return 0;
	}

2) 新建CMakeLists.txt

	add_executable(hello hello.cpp)

现在目录结构如下：

	–test/
	—- hello.cpp
	—- CMakeLists.txt

3）创建一个build目录

	–test/
	
	—-hello.cpp
	
	—-CMakeLists.txt
	
	—-build/

4）cd到build目录执行cmake

	➜  /Users/parrot/build cmake ../

	-- The C compiler identification is AppleClang 7.3.0.7030029
	-- The CXX compiler identification is AppleClang 7.3.0.7030029
	-- Check for working C compiler: /usr/bin/cc
	-- Check for working C compiler: /usr/bin/cc -- works
	-- Detecting C compiler ABI info
	-- Detecting C compiler ABI info - done
	-- Check for working CXX compiler: /usr/bin/c++
	-- Check for working CXX compiler: /usr/bin/c++ -- works
	-- Detecting CXX compiler ABI info
	-- Detecting CXX compiler ABI info - done
	-- Configuring done
	-- Generating done
	-- Build files have been written to: /Users/sunyongjian1/codes/local_codes/cmake_test/build

5）执行make，运行./hello