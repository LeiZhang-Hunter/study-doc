1 pair的应用

pair是将2个数据组合成一个数据，当需要这样的需求时就可以使用pair，如stl中的map就是将key和value放在一起来保存。另一个应用是，当一个函数需要返回2个数据的时候，可以选择pair。 pair的实现是一个结构体，主要的两个成员变量是first second 因为是使用struct不是class，所以可以直接使用pair的成员变量。

	#include <utility>
	#include <iostream>
	
	int main() {
	    std::pair<int ,int> data = std::make_pair(1, 3);
	    std::cout<<data.first<<std::endl;
	    std::cout<<data.second<<std::endl;
	}

2.有序集合


	
	
	
	#include <utility>
	#include <iostream>
	#include <set>
	
	std::set<int> setData;
	
	int main() {
	    setData.insert(1);
	    setData.insert(1);
	    setData.insert(1);
	    setData.insert(3);
	    setData.insert(5);
	    setData.insert(8);
	
	    std::set<int>::iterator iterator;
	    for (iterator = setData.begin(); iterator != setData.end(); iterator++) {
	        std::cout<<*iterator<<std::endl;
	    }
	
	    //检查元素是否存在
	    std::cout<<setData.count(5)<<std::endl;
	
	    //返回大于某个值的迭代器
	    std::cout<<*setData.upper_bound(1)<<std::endl;
	    
	}