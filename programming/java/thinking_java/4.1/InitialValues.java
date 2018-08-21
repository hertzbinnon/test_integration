package c04;
class Measurement {
	boolean t;
	char c;
	byte b;
	short s;
	int i;
	long l;
	float f;
	double d;
	void print() {
		System.out.println(
		"Data type Inital value\n" +
		"boolean " + t + "\n" +
		"char " + c + "\n" +
		"byte " + b + "\n" +
		"short " + s + "\n" +
		"int " + i + "\n" +
		"long " + l + "\n" +
		"float " + f + "\n" +
		"double " + d);
	}
}
public class InitialValues {
public static void main(String[] args) {
Measurement d = new Measurement();
d.print();
/* In this case you could also say:
 * new Measurement().print();
 * */
}
} ///:~
