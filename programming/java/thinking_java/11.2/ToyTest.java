package c11;

interface HasBatteries {}
interface Waterproof {}
interface ShootsThings {}

class Toy {
 //Comment out the following default
 //constructor to see
 //NoSuchMethodError from (*1*)
Toy() {}
Toy(int i) {}
}
class FancyToy extends Toy implements HasBatteries,Waterproof, ShootsThings {
	FancyToy() 
	{ super(1); }
}
public class ToyTest {
public static void main(String[] args) {
Class c = null;
try {
	c = Class.forName("c11.FancyToy");
	System.out.println("Class name: " + c.getName() +" is interface? [" + c.isInterface() + "]");
} catch(ClassNotFoundException e) {
	System.out.println("What happen" + e.toString());
}
//printInfo(c);
Class[] faces = c.getInterfaces();
for(int i = 0; i < faces.length; i++)
printInfo(faces[i]);
Class cy = c.getSuperclass();
Object o = null;
try {
 //Requires default constructor:
o = cy.newInstance(); 
} catch(InstantiationException e) {}
catch(IllegalAccessException e) {}

printInfo(o.getClass());
}
static void printInfo(Class cc) {
System.out.println("Class name: " + cc.getName() +" is interface? [" + cc.isInterface() + "]");
}
}
