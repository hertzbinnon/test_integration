package c11.petcount2;
import java.util.*;
class Pet {}
class Dog extends Pet {}
class Pug extends Dog {}
class Cat extends Pet {}
class Rodent extends Pet {}
class Gerbil extends Rodent {}
class Hamster extends Rodent {}
class Counter { int i; }
public class PetCount2 {
public static void main(String[] args) {
	Vector pets = new Vector();
	Class[] petTypes = {
		Pet.class,
		Dog.class,
		Pug.class,
		Cat.class,
		Rodent.class,
		Gerbil.class,
		Hamster.class,
		};
	try {
		for(int i = 0; i < 15; i++) {

			int rnd = 1 + (int)(Math.random() * (petTypes.length - 1));
			pets.addElement(
				petTypes[rnd].newInstance());
		}
	} catch(InstantiationException e) {}
	catch(IllegalAccessException e) {}
	Hashtable h = new Hashtable();
	for(int i = 0; i < petTypes.length; i++)
		h.put(petTypes[i].toString(),
		new Counter());
	for(int i = 0; i < pets.size(); i++) {
		Object o = pets.elementAt(i);
		if(o instanceof Pet)
			((Counter)h.get(
			"class c11.petcount2.Pet")).i++;
		if(o instanceof Dog)
			((Counter)h.get(
			"class c11.petcount2.Dog")).i++;
		if(o instanceof Pug)
			((Counter)h.get(
			"class c11.petcount2.Pug")).i++;
		if(o instanceof Cat)
			((Counter)h.get(
			"class c11.petcount2.Cat")).i++;
		if(o instanceof Rodent)
			((Counter)h.get(
		"class c11.petcount2.Rodent")).i++;
if(o instanceof Gerbil)
((Counter)h.get(
"class c11.petcount2.Gerbil")).i++;
if(o instanceof Hamster)
((Counter)h.get(
"class c11.petcount2.Hamster")).i++;
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
