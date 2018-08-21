#include <iostream>
#include <memory> // std::shared_ptr
#include <thread> // std::thread

class X
{
	int x=2;
	std::string *Str;
	const char*  Str2 = "this string 2";
	std::string *Str3 = new std::string("this string 3");
	static std::string *Str4 ; // static data member cant be init in in-class
	std::string Str5; // string cant be init in general like it Str5("something")
public:
	X():x{1}{
		Str = new std::string("default string");
		std::cout<< x << " " <<  Str->c_str()<< std::endl;
	}
	X(const char* str):Str5{"this string 5"}{
		Str = new std::string(str);
		std::cout<< x << " " << str << " " <<  this << std::endl;
	}
	~X(){
		std::cout<<"X destructor"<<std::endl;
		delete(Str);
	}
	int getx() const{ return x;}
	int setx() const{ return x;}
};
std::string *Str4 = new std::string("this string 4");


//typedef std::string(int x, char y, double z) newtype;
std::string fn(int x, char y, double z){
	std::cout<< "that is it" << std::endl;
	return std::string("yes");
}

X fn1(int x, char y, double z){
	X xobj("X constructor");
	std::cout<< &xobj << std::endl;
	return xobj;
}

X fn2(int x, char y, double z){
	X xobj("X1 constructor");
	return xobj;
}
int main(){
	X obj1; // dont call destructor when  exit
	X obj("string 1"); // dont call destructor when  exit
	std::cout<< "x = " << obj1.getx() << std::endl;
	X obj2("local obj");
	X* pobj = new X();
	delete(pobj);
	std::string Str5("this is string 5");
	std::shared_ptr<X> obj3  ( new X() ); // class std::shared_ptr<X> constructor: (T* )
	X* tmp = new X();
	std::shared_ptr<X> obj4(tmp)   ;
	std::function<std::string(int x,char y,double z)> fun(fn);
	std::string Strobj = fun(1,2,3);
	std::cout<< Strobj.c_str();

	std::function<X(int x,char y,double z)> fun1(fn1);
	X Strobj1 = fun1(1,2,3);
	std::cout<< &Strobj1 << std::endl;
	std::function<X(int x,char y,double z)> fun2; // std::function is function  pointer
	fun2 = fn2;
	fun2(1,2,3);
	
	std::shared_ptr<X> pX;
	pX = std::shared_ptr <X> (new X()); // pointer cast 
	std::cout<< "x = " << pX->getx() << std::endl; // !!! Note: operator `->' overload by shared_ptr


	std::cout << "pyhsical core " << std::thread::hardware_concurrency() << std::endl;

	std::unique_ptr<X> pX1;
	pX1.reset(new X());
	std::cout<< "x = " << pX1->getx() << std::endl;
	return 0;
}
