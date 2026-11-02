#include <iostream>
#include <list>
using namespace std;

template<typename T>
void print_l(list<T>& l) {
    static int n = 1;
    cout << n << ": ";
    for (auto e : l) {
        cout << e << ' ';
    }
    cout << endl;
    n++;
}

int main() {

    cout << "It's a simple tutorial of list" << endl;
    cout << "==============================" << endl;

    // construct
    list<int> l1;
    list<int> l2(5, 10);
    list<int> l3 = {1, 2, 3, 4, 5};
    list<int> l4(l3);
    list<int> l5(l3.begin(), l3.end());

    // push / pop / push_front / pop_back
    l3.push_front(0);
    auto f = l3.front(); cout << f << endl;
    l3.pop_front();
    l3.push_back(6);
    auto b = l3.back(); cout << b << endl;
    l3.pop_back();      print_l(l3);

    // reverse
    l3.reverse();   print_l(l3);
    l3.reverse();   print_l(l3);
    
    // insert / erase / advance
    auto it = l3.begin();
    advance(it, 2);
    l3.insert(it, 99);     print_l(l3);
    l3.erase(it);          print_l(l3);

    l3.insert(l3.begin(), 3, 88);   print_l(l3);
    
    list<int> another = {100, 200};
    l3.insert(l3.end(), another.begin(), another.end());    print_l(l3);

    auto first = l3.begin();
    auto last = l3.begin();
    advance(first, 1);
    advance(last, 4);
    l3.erase(first, last);      print_l(l3);

    // 3 iterate methods
    cout << "3 methods of interation" << endl;
    for (auto it = l3.begin(); it != l3.end(); it++) {
        cout << *it << " ";
    }
    cout << endl;
    for (auto it = l3.rbegin(); it != l3.rend(); it++) {
        cout << *it << " ";
    }
    cout << endl;
    for (int x : l3) {
        cout << x << " ";
    }
    cout << endl;

    // remove / remove_if
    l3.remove(100);     print_l(l3);
    l3.remove_if([](int x) { return x > 50; });      print_l(l3);

    // size / resize / empty
    cout << "Size before: " << l3.size() << endl;
    l3.resize(3);
    cout << "Size: " << l3.size() << endl;
    l3.resize(5, 0);    print_l(l3);
    cout << "Empty before? " << l3.empty() << endl;
    cout << "Max size: " << l3.max_size() << endl;

    // clear
    l3.clear();
    cout << "Empty? " << l3.empty() << endl;

    cout << "=========================" << endl;
    cout << "Some advanced featured..." << endl;

    // sort
    list<int> l = {5, 2, 8, 1, 9, 3};
    l.sort();               /* 升序 */      print_l(l);
    l.sort(less<int>());
    l.sort(greater<int>()); /* 降序 */      print_l(l);
    l.sort([](int a, int b) {
        if ((a % 2) != (b % 2)) {
            return a % 2 == 1;
        }
        return a < b;
    });                                     print_l(l);

    // unique
    list<int> lst = {1, 1, 2, 3, 3, 3, 4, 5, 5};
    lst.sort();
    lst.unique();                           print_l(lst);
    list<string> words = {"apple", "apricot", "banana", "blueberry"};
    print_l(words);
    words.unique([](const string& a, const string& b) {
        return a[0] == b[0];
    });                                     print_l(words);

    // merge
    list<int> list1 = {1, 3, 5, 7, 9};
    list<int> list2 = {2, 4, 6, 8, 10};
    list1.merge(list2);                     print_l(list1); print_l(list2);
    list<int> list3 = {9, 7, 5, 3, 1};
    list<int> list4 = {10, 8, 6, 4, 2};
    list3.merge(list4, greater<int>());     print_l(list3); print_l(list4);

    // embrace series and splice omitted
    
    return 0;
}