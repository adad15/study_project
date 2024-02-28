// map容器构造
#include <iostream>
#include <map>
#include <string.h>
using namespace std;

int main()
{
	// 1）map();  // 创建一个空的map容器。
	map<int, string> m1;

	// 2）map(initializer_list<pair<K, V>> il); // 使用统一初始化列表。
	map<int, string> m2({ {8, "冰冰"}, {3, "西施"}, {1, "幂幂"}, {7, "金莲"}, {5, "西瓜"} });
	// map<int, string> m2={ { 8,"冰冰" }, { 3,"西施" }, { 1,"幂幂" }, { 7,"金莲" }, { 5,"西瓜" } };
	// map<int, string> m2   { { 8,"冰冰" }, { 3,"西施" }, { 1,"幂幂" }, { 7,"金莲" }, { 5,"西瓜" } };
	for (auto& val : m2)
		cout << val.first << "," << val.second << " ";
	cout << endl;

	// 3）map(const map<K, V>&m);  // 拷贝构造函数。
	map<int, string> m3 = m2;
	for (auto& val : m3)
		cout << val.first << "," << val.second << " ";
	cout << endl;

	// 4）map(Iterator first, Iterator last);  // 用迭代器创建map容器。
	auto first = m3.begin();
	first++;
	auto last = m3.end();
	last--;
	map<int, string> m4(first, last);
	for (auto& val : m4)
		cout << val.first << "," << val.second << " ";
	cout << endl;

	// 5）map(map<K, V> && m);  // 移动构造函数（C++11标准）。
	system("pause");
}