linux多线程服务端编程

####scoped_ptr

scoped_ptr是一个比较简单的智能指针，它能保证离开作用域之后自动被释放掉。这是一个boost语法不是一个c++语法，所以在c++ primer中没有找到
 一个简单的demo

 

	#include <iostream>
	#include <boost/scoped_ptr.hpp>

	using namespace std;

	class Book
	{
	public:
	    Book()
	    {
		cout << "Creating book ..." << endl;
	    }

	    ~Book()
	    {
		cout << "Destroying book ..." << endl;
	    }
	};

	int main()
	{   
	    cout << "=====Main Begin=====" << endl;
	    {
		boost::scoped_ptr<Book> myBook(new Book());
	    }
	    cout << "===== Main End =====" << endl;

	    return 0;
	}
 

我们需要注意的是scoped_ptr/shared_ptr/weak_ptr一般是值成员，应用到Observer上。我们使用c++ 编写一个多线程的观察者模式