package c11.petcount3;
import java.util.*;
class Pet {}
class Dog extends Pet {}
class Pug extends Dog {}
class Cat extends Pet {}
class Rodent extends Pet {}
class Gerbil extends Rodent {}
class Hamster extends Rodent {}
class Counter { int i; }
public class PetCount3 {
public static void main(String[] args) {
	String s = null;
	Pet sn = new Pet();
try {
	s = Pet.class.newInstance().toString();
} catch(InstantiationException e) {}
catch(IllegalAccessException e) {}

System.out.println("class obj (meta class) " + s);
System.out.println("class obj " + sn);
System.out.println("class obj " + sn.getClass());
System.out.println("class obj " + Pet.class);
System.out.println("class obj " + Pet.class.getName());
	Vector pets = new Vector();
	Class[] petTypes = {
		Pet.class,Dog.class,Pug.class,Cat.class,Rodent.class,Gerbil.class,Hamster.class,
	};
try {
	for(int i = 0; i < 15; i++) {
		int rnd = 1 + (int)(
			Math.random() * (petTypes.length - 1));
			pets.addElement(
			petTypes[rnd].newInstance());
	}
} catch(InstantiationException e) {}
catch(IllegalAccessException e) {}
	Hashtable h = new Hashtable();
	for(int i = 0; i < petTypes.length; i++)
		h.put(petTypes[i].toString(),new Counter());
	for(int i = 0; i < pets.size(); i++) {
		Object o = pets.elementAt(i);

		for (int j = 0; j < petTypes.length; ++j)
			if (petTypes[j].isInstance(o)) {
				String key = petTypes[j].toString();
				((Counter)h.get(key)).i++;
		}
	}
for(int i = 0; i < pets.size(); i++)
System.out.println(
pets.elementAt(i).getClass().toString());
Enumeration keys = h.keys();
while(keys.hasMoreElements()) {
String nm = (String)keys.nextElement();
Counter cnt = (Counter)h.get(nm);
System.out.println(
nm.substring(nm.lastIndexOf('.') + 1) +
" quantity: " + cnt.i);
}
}
}
