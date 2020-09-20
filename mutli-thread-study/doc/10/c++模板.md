c++ 代码膨胀 是什么

我们知道使用模板时， 同一模板生成不同的模板实类后会是多份代码 ，比如 vector<int>， vector<char>, vector<double>, 这里总共会生成3份不同的vector代码，这就是我们平时所说的代码膨胀。

模板编译链接的不同之处在于，以上具有external linkage的对象通常会在多个编译单元被定义。连接器必须进行重复的代码取消，才能正确生成可执行文件。

templete 和 inline 不会导致代码的膨胀

假设有一个定长的buffer类，其内置buffer长度是在编译期间确定的，我们可以把它实现为非模板类模板
	
	#include <cstring>
	
	template<int Size>
	class Buffer {
	public:
	    Buffer() : index_(0) {};
	
	    void clear() { index_ = 0; }
	
	    void append(const void* data, int len){
	        ::memcpy(buffer + index_, data, len);
	        index_ += len;
	    }
	private:
	    char index_;
	    char buffer[Size];
	};
	
	int main() {
	    Buffer<256> b1;
	    b1.append("hello", 5);
	    b1.clear();
	
	    Buffer<1024> b2;
	    b2.append("template", 8);
	    b2.clear();
	}

g++结果

	zhanglei@zhanglei-virtual-machine:~/ourc/test$ nm a.out 
	0000000000004010 B __bss_start
	0000000000004010 b completed.8059
	                 w __cxa_finalize@@GLIBC_2.2.5
	0000000000004000 D __data_start
	0000000000004000 W data_start
	00000000000010b0 t deregister_tm_clones
	0000000000001120 t __do_global_dtors_aux
	0000000000003db8 d __do_global_dtors_aux_fini_array_entry
	0000000000004008 D __dso_handle
	0000000000003dc0 d _DYNAMIC
	0000000000004010 D _edata
	0000000000004018 B _end
	00000000000013a8 T _fini
	0000000000001160 t frame_dummy
	0000000000003db0 d __frame_dummy_init_array_entry
	000000000000224c r __FRAME_END__
	0000000000003fb0 d _GLOBAL_OFFSET_TABLE_
	                 w __gmon_start__
	0000000000002014 r __GNU_EH_FRAME_HDR
	0000000000001000 t _init
	0000000000003db8 d __init_array_end
	0000000000003db0 d __init_array_start
	0000000000002000 R _IO_stdin_used
	                 w _ITM_deregisterTMCloneTable
	                 w _ITM_registerTMCloneTable
	00000000000013a0 T __libc_csu_fini
	0000000000001330 T __libc_csu_init
	                 U __libc_start_main@@GLIBC_2.2.5
	0000000000001169 T main
	                 U memcpy@@GLIBC_2.14
	00000000000010e0 t register_tm_clones
	                 U __stack_chk_fail@@GLIBC_2.4
	0000000000001080 T _start
	0000000000004010 D __TMC_END__
	000000000000130e W _ZN6BufferILi1024EE5clearEv
	00000000000012b2 W _ZN6BufferILi1024EE6appendEPKvi
	000000000000129c W _ZN6BufferILi1024EEC1Ev
	000000000000129c W _ZN6BufferILi1024EEC2Ev
	0000000000001286 W _ZN6BufferILi256EE5clearEv
	000000000000122a W _ZN6BufferILi256EE6appendEPKvi
	0000000000001214 W _ZN6BufferILi256EEC1Ev
	0000000000001214 W _ZN6BufferILi256EEC2Ev
	zhanglei@zhanglei-virtual-machine:~/ourc/test$ 

我们可以看到，类模板会为每一个用到的类模板成员函数具现化一份实体

这样看来很可能出现代码膨胀的问题，但是实际情况不会这样，我们用-O2会发现模板被inline展开了。

	zhanglei@zhanglei-virtual-machine:~/ourc/test$ nm a.out 
	0000000000004010 B __bss_start
	0000000000004010 b completed.8059
	                 w __cxa_finalize@@GLIBC_2.2.5
	0000000000004000 D __data_start
	0000000000004000 W data_start
	0000000000001080 t deregister_tm_clones
	00000000000010f0 t __do_global_dtors_aux
	0000000000003df8 d __do_global_dtors_aux_fini_array_entry
	0000000000004008 D __dso_handle
	0000000000003e00 d _DYNAMIC
	0000000000004010 D _edata
	0000000000004018 B _end
	00000000000011b8 T _fini
	0000000000001130 t frame_dummy
	0000000000003df0 d __frame_dummy_init_array_entry
	0000000000002124 r __FRAME_END__
	0000000000003fc0 d _GLOBAL_OFFSET_TABLE_
	                 w __gmon_start__
	0000000000002004 r __GNU_EH_FRAME_HDR
	0000000000001000 t _init
	0000000000003df8 d __init_array_end
	0000000000003df0 d __init_array_start
	0000000000002000 R _IO_stdin_used
	                 w _ITM_deregisterTMCloneTable
	                 w _ITM_registerTMCloneTable
	00000000000011b0 T __libc_csu_fini
	0000000000001140 T __libc_csu_init
	                 U __libc_start_main@@GLIBC_2.2.5
	0000000000001040 T main
	00000000000010b0 t register_tm_clones
	0000000000001050 T _start
	0000000000004010 D __TMC_END__
	zhanglei@zhanglei-virtual-machine:~/ourc/test$ 

一般c++教材会告诉你，模板定义要放到头文件中，否则会出现编译错误。如果读者细心 会发现编译错误其实就是链接错误。

	template <typename T>
	void foo(const T&);
	
	template <typename T>
	T bar(const T&);
	
	int main()
	{
	    foo(0);
	    foo(1.0);
	    bar('c');
	}

错误信息:
	
	/usr/bin/ld: CMakeFiles/test.dir/main.cpp.o: in function `main':
	/home/zhanglei/ourc/test/main.cpp:9: undefined reference to `void foo<int>(int const&)'
	/usr/bin/ld: /home/zhanglei/ourc/test/main.cpp:10: undefined reference to `void foo<double>(double const&)'
	/usr/bin/ld: /home/zhanglei/ourc/test/main.cpp:11: undefined reference to `char bar<char>(char const&)'
	collect2: error: ld returned 1 exit status
	make[3]: *** [CMakeFiles/test.dir/build.make:84：test] 错误 1
	make[2]: *** [CMakeFiles/Makefile2:76：CMakeFiles/test.dir/all] 错误 2
	make[1]: *** [CMakeFiles/Makefile2:83：CMakeFiles/test.dir/rule] 错误 2
	make: *** [Makefile:118：test] 错误 2

那么有办法把模板的实现放到库里，头文件只放声明吗？其实是可以的，前提是你知道模板会有那些具体化类型，并且先显式具体化出来。

	zhanglei@zhanglei-virtual-machine:~/ourc/test$ g++ -c main.cpp 
	zhanglei@zhanglei-virtual-machine:~/ourc/test$ nm main.o
	                 U _GLOBAL_OFFSET_TABLE_
	0000000000000000 T main
	                 U __stack_chk_fail
	                 U _Z3barIcET_RKS0_
	                 U _Z3fooIdEvRKT_
	                 U _Z3fooIiEvRKT_
